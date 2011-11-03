#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"
#include "R_ext/Parse.h"
#include "yaml.h"
#include "yaml_private.h"

static SEXP R_KeysSymbol = NULL;
static SEXP R_IdenticalFunc = NULL;
static SEXP R_FormatFunc = NULL;
static char error_msg[255];

yaml_char_t *find_implicit_tag(yaml_char_t *value, size_t size);

typedef struct {
  int refcount;
  SEXP obj;

  /* For storing sequence types */
  int seq_type;

  /* This is for tracking whether or not this object has a parent.
   * If there is no parent, that means this object should be UNPROTECT'd
   * when assigned to a parent SEXP object */
  int orphan;
} s_prot_object;

typedef struct {
  s_prot_object *obj;
  int placeholder;
  char *tag;
  void *prev;
} s_stack_entry;

typedef struct {
  yaml_char_t *name;
  s_prot_object *obj;
  void *prev;
} s_alias_entry;

static int
R_cmp(x, y)
  SEXP x;
  SEXP y;
{
  int i, retval = 0, *arr;
  SEXP call, result, t;

  PROTECT(t = allocVector(LGLSXP, 1));
  LOGICAL(t)[0] = 1;
  PROTECT(call = lang5(R_IdenticalFunc, x, y, t, t));
  PROTECT(result = eval(call, R_GlobalEnv));

  arr = LOGICAL(result);
  for(i = 0; i < LENGTH(result); i++) {
    if (!arr[i]) {
      retval = 1;
      break;
    }
  }
  UNPROTECT(3);
  return retval;
}

static int
R_index(haystack, needle, character, upper_bound)
  SEXP haystack;
  SEXP needle;
  int character;
  int upper_bound;
{
  int i;

  if (character) {
    for (i = 0; i < upper_bound; i++) {
      if (strcmp(CHAR(needle), CHAR(STRING_ELT(haystack, i))) == 0) {
        return i;
      }
    }
  }
  else {
    for (i = 0; i < upper_bound; i++) {
      if (R_cmp(needle, VECTOR_ELT(haystack, i)) == 0) {
        return i;
      }
    }
  }

  return -1;
}

static int
R_rindex(haystack, needle, character, lower_bound)
  SEXP haystack;
  SEXP needle;
  int character;
  int lower_bound;
{
  int i;

  if (character) {
    for (i = LENGTH(haystack) - 1; i > lower_bound; i--) {
      if (strcmp(CHAR(needle), CHAR(STRING_ELT(haystack, i))) == 0) {
        return i;
      }
    }
  }
  else {
    for (i = LENGTH(haystack) - 1; i > lower_bound; i--) {
      if (R_cmp(needle, VECTOR_ELT(haystack, i)) == 0) {
        return i;
      }
    }
  }

  return -1;
}

static int
R_is_named_list(obj)
  SEXP obj;
{
  SEXP names;
  if (TYPEOF(obj) != VECSXP)
    return 0;

  names = GET_NAMES(obj);
  return (TYPEOF(names) == STRSXP && LENGTH(names) == LENGTH(obj));
}

static int
R_is_pseudo_hash(obj)
  SEXP obj;
{
  SEXP keys;
  if (TYPEOF(obj) != VECSXP)
    return 0;

  keys = getAttrib(obj, R_KeysSymbol);
  return (keys != R_NilValue && TYPEOF(keys) == VECSXP);
}

static const char *
R_format(x)
  SEXP x;
{
  SEXP call, result;

  PROTECT(call = lang2(R_FormatFunc, x));
  result = eval(call, R_GlobalEnv);
  UNPROTECT(1);

  return CHAR(STRING_ELT(result, 0));
}

static void
R_set_str_attrib( obj, sym, str )
  SEXP obj;
  SEXP sym;
  char *str;
{
  SEXP val;
  PROTECT(val = NEW_STRING(1));
  SET_STRING_ELT(val, 0, mkChar(str));
  setAttrib(obj, sym, val);
  UNPROTECT(1);
}

static void
R_set_class( obj, name )
  SEXP obj;
  char *name;
{
  R_set_str_attrib(obj, R_ClassSymbol, name);
}

static int
R_class_of( obj, name )
  SEXP obj;
  char *name;
{
  int i;
  SEXP class = GET_CLASS(obj);
  if (TYPEOF(class) == STRSXP) {
    for (i = 0; i < length(class); i++) {
      if (strcmp(CHAR(STRING_ELT(GET_CLASS(obj), i)), name) == 0) {
        return 1;
      }
    }
  }

  return 0;
}

static s_prot_object *
new_prot_object(obj)
  SEXP obj;
{
  s_prot_object *result;

  result = (s_prot_object *)malloc(sizeof(s_prot_object));
  result->refcount = 0;
  result->obj = obj;
  result->orphan = 1;
  result->seq_type = -1;

  return result;
}

static void
prune_prot_object(obj)
  s_prot_object *obj;
{
  if (obj == NULL)
    return;

  if (obj->obj != NULL && obj->orphan == 1) {
    /* obj is now part of another object and is therefore protected */
    UNPROTECT_PTR(obj->obj);
    obj->orphan = 0;
  }

  if (obj->refcount == 0) {
    /* Don't need this object wrapper anymore */
    free(obj);
  }
}

static void
stack_push(stack, placeholder, tag, obj)
  s_stack_entry **stack;
  int placeholder;
  const char *tag;
  s_prot_object *obj;
{
  s_stack_entry *result;

  result = (s_stack_entry *)malloc(sizeof(s_stack_entry));
  result->placeholder = placeholder;
  if (tag != NULL) {
    result->tag = strdup(tag);
  }
  else {
    result->tag = NULL;
  }
  result->obj = obj;
  obj->refcount++;
  result->prev = *stack;

  *stack = result;
}

static void
stack_pop(stack, obj)
  s_stack_entry **stack;
  s_prot_object **obj;
{
  s_stack_entry *result, *top;

  top = *stack;
  if (obj) {
    *obj = top->obj;
  }
  top->obj->refcount--;
  result = (s_stack_entry *)top->prev;

  if (top->tag != NULL) {
    free(top->tag);
  }
  free(top);

  *stack = result;
}

static char *
process_tag(tag)
  char *tag;
{
  char *retval = tag;

  if (strncmp(retval, "tag:yaml.org,2002:", 18) == 0) {
    retval = retval + 18;
  }
  else {
    while (*retval == '!') {
      retval++;
    }
  }
  return retval;
}

static SEXP
find_handler(handlers, name)
  SEXP handlers;
  const char *name;
{
  SEXP names;
  int i;

  /* Look for a custom R handler */
  if (handlers != R_NilValue) {
    names = GET_NAMES(handlers);
    for (i = 0; i < length(names); i++) {
      if (STRING_ELT(names, i) != NA_STRING) {
        if (strcmp(translateChar(STRING_ELT(names, i)), name) == 0) {
          /* Found custom handler */
          return VECTOR_ELT(handlers, i);
        }
      }
    }
  }

  return R_NilValue;
}

static int
run_handler(handler, arg, result)
  SEXP handler;
  SEXP arg;
  SEXP *result;
{
  SEXP cmd;
  int errorOccurred;

  PROTECT(cmd = allocVector(LANGSXP, 2));
  SETCAR(cmd, handler);
  SETCADR(cmd, arg);
  *result = R_tryEval(cmd, R_GlobalEnv, &errorOccurred);
  UNPROTECT(1);

  return errorOccurred;
}

static int
handle_alias(event, stack, aliases)
  yaml_event_t *event;
  s_stack_entry **stack;
  s_alias_entry *aliases;
{
  s_alias_entry *alias = aliases;
  while (alias) {
    if (strcmp((char *)alias->name, event->data.alias.anchor) == 0) {
      stack_push(stack, 0, NULL, alias->obj);
      break;
    }
  }
  return 0;
}

static int
handle_start_event(tag, stack)
  const char *tag;
  s_stack_entry **stack;
{
  stack_push(stack, 1, tag, new_prot_object(NULL));
  return 0;
}

/* Call this on a just created object. */
static int
convert_object(event_type, s_obj, tag, s_handlers, coerce_keys)
  yaml_event_type_t event_type;
  s_prot_object *s_obj;
  yaml_char_t *tag;
  SEXP s_handlers;
  int coerce_keys;
{
  SEXP handler, obj, new_obj, elt, keys, key, expr, text, call, pcall, Rfn;
  int handled, coercionError, base, i, len, total_len, idx, elt_len, j, k, dup_key;
  const char *nptr;
  char *endptr;
  double f;
  ParseStatus parseStatus;

  /* Look for a custom R handler */
  handler = find_handler(s_handlers, tag);
  handled = 0;
  obj = s_obj->obj;
  new_obj = NULL;
  if (handler != R_NilValue) {
    if (run_handler(handler, obj, &new_obj) != 0) {
      warning("an error occurred when handling type '%s'; using default handler", tag);
    }
    else {
      handled = 1;
      if (new_obj != R_NilValue) {
        PROTECT(new_obj);
      }
    }
  }

  coercionError = 0;
  if (!handled) {
    /* default handlers, ordered by most-used */

    if (strcmp(tag, "str") == 0) {
      /* if this is a scalar, then it's already a string */
      coercionError = event_type != YAML_SCALAR_EVENT;
    }
    else if (strcmp(tag, "seq") == 0) {
      /* Let's try to coerce this list! */
      switch (s_obj->seq_type) {
        case LGLSXP:
        case INTSXP:
        case REALSXP:
        case STRSXP:
          PROTECT(new_obj = coerceVector(obj, s_obj->seq_type));
          break;
      }
    }
    else if (strcmp(tag, "int") == 0 || strncmp(tag, "int#", 4) == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        base = -1;
        if (strcmp(tag, "int") == 0) {
          base = 10;
        }
        else if (strcmp(tag, "int#hex") == 0) {
          base = 16;
        }
        else if (strcmp(tag, "int#oct") == 0) {
          base = 8;
        }

        if (base >= 0) {
          nptr = CHAR(STRING_ELT(obj, 0));
          i = (int)strtol(nptr, &endptr, base);
          if (*endptr != 0) {
            /* strtol is perfectly happy converting partial strings to
             * integers, but R isn't. If you call as.integer() on a
             * string that isn't completely an integer, you get back
             * an NA. So I'm reproducing that behavior here. */

            printf("Original: (%p), Leftover: (%p)\n", nptr, endptr);
            warning("NAs introduced by coercion: %s is not an integer", nptr);
            i = NA_INTEGER;
          }

          PROTECT(new_obj = NEW_INTEGER(1));
          INTEGER(new_obj)[0] = i;
        }
        else {
          /* Don't do anything, we don't know how to handle this type */
        }
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "float") == 0 || strcmp(tag, "float#fix") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        nptr = CHAR(STRING_ELT(obj, 0));
        f = strtod(nptr, &endptr);
        if (*endptr != 0) {
          /* No valid floats found (see note above about integers) */
          warning("NAs introduced by coercion: %s is not a real", nptr);
          f = NA_REAL;
        }

        PROTECT(new_obj = NEW_NUMERIC(1));
        REAL(new_obj)[0] = f;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "bool#yes") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_LOGICAL(1));
        LOGICAL(new_obj)[0] = 1;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "bool#no") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_LOGICAL(1));
        LOGICAL(new_obj)[0] = 0;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "omap") == 0) {
      /* NOTE: This is here mostly because of backwards compatibility
       * with R yaml 1.x package. All maps are ordered in 2.x, so there's
       * no real need to use omap */

      if (event_type == YAML_SEQUENCE_END_EVENT) {
        len = length(obj);
        total_len = 0;
        for (i = 0; i < len; i++) {
          elt = VECTOR_ELT(obj, i);
          if ((coerce_keys && !R_is_named_list(elt)) || (!coerce_keys && !R_is_pseudo_hash(elt))) {
            sprintf(error_msg, "omap must be a sequence of maps");
            return 1;
          }
          total_len += length(elt);
        }

        /* Construct the list! */
        PROTECT(new_obj = allocVector(VECSXP, total_len));
        if (coerce_keys) {
          keys = allocVector(STRSXP, total_len);
          SET_NAMES(new_obj, keys);
        }
        else {
          keys = allocVector(VECSXP, total_len);
          setAttrib(new_obj, R_KeysSymbol, keys);
        }

        dup_key = 0;
        for (i = 0, idx = 0; i < len && dup_key == 0; i++) {
          elt = VECTOR_ELT(obj, i);
          elt_len = length(elt);
          for (j = 0; j < elt_len && dup_key == 0; j++) {
            SET_VECTOR_ELT(new_obj, idx, VECTOR_ELT(elt, j));

            if (coerce_keys) {
              key = STRING_ELT(GET_NAMES(elt), j);
              SET_STRING_ELT(keys, idx, key);

              if (R_index(keys, key, 1, idx) >= 0) {
                dup_key = 1;
                sprintf(error_msg, "Duplicate omap key: '%s'", CHAR(key));
              }
            }
            else {
              key = VECTOR_ELT(getAttrib(elt, R_KeysSymbol), j);
              SET_VECTOR_ELT(keys, idx, key);

              if (R_index(keys, key, 0, idx) >= 0) {
                dup_key = 1;
                sprintf(error_msg, "Duplicate omap key: %s", R_format(key));
              }
            }
            idx++;
          }
        }

        if (dup_key == 1) {
          UNPROTECT(1); // new_obj
          coercionError = 1;
        }
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "merge") == 0) {
      /* see http://yaml.org/type/merge.html */
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_STRING(1));
        SET_STRING_ELT(new_obj, 0, mkChar("_yaml.merge_"));
        R_set_class(new_obj, "_yaml.merge_");
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "float#nan") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_NUMERIC(1));
        REAL(new_obj)[0] = R_NaN;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "float#inf") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_NUMERIC(1));
        REAL(new_obj)[0] = R_PosInf;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "float#neginf") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(new_obj = NEW_NUMERIC(1));
        REAL(new_obj)[0] = R_NegInf;
      }
      else {
        coercionError = 1;
      }
    }
    else if (strcmp(tag, "null") == 0) {
      new_obj = R_NilValue;
    }
    else if (strcmp(tag, "expr") == 0) {
      if (event_type == YAML_SCALAR_EVENT) {
        PROTECT(expr = R_ParseVector(obj, -1, &parseStatus, R_NilValue));
        if (parseStatus != PARSE_OK) {
          UNPROTECT(1); // expr
          sprintf(error_msg, "Could not parse expression: %s", CHAR(STRING_ELT(obj, 0)));
          coercionError = 1;
        }
        else {
          /* NOTE: R_tryEval will not return if R_Interactive is FALSE. */
          for (i = 0; i < length(expr); i++) {
            new_obj = R_tryEval(VECTOR_ELT(expr, i), R_GlobalEnv, &coercionError);
            if (coercionError) {
              break;
            }
          }
          UNPROTECT(1); // expr

          if (coercionError) {
            sprintf(error_msg, "Could not evaluate expression: %s", CHAR(STRING_ELT(obj, 0)));
          }
          else {
            PROTECT(new_obj);
          }
        }
      }
      else {
        coercionError = 1;
      }
    }
  }

  if (coercionError == 1) {
    if (error_msg[0] == 0) {
      sprintf(error_msg, "Invalid tag: %s for %s", tag, (event_type == YAML_SCALAR_EVENT ? "scalar" : (event_type == YAML_SEQUENCE_END_EVENT ? "sequence" : "map")));
    }
    return 1;
  }

  if (new_obj != NULL) {
    UNPROTECT_PTR(obj);
    s_obj->obj = new_obj;
    s_obj->orphan = new_obj != R_NilValue;
  }

  return 0;
}

static int
handle_scalar(event, stack, return_tag)
  yaml_event_t *event;
  s_stack_entry **stack;
  yaml_char_t **return_tag;
{
  SEXP obj;
  yaml_char_t *value, *tag;
  size_t len;

  tag = event->data.scalar.tag;
  value = event->data.scalar.value;
  if (tag == NULL || strcmp(tag, "!") == 0) {
    /* If there's no tag, try to tag it */
    len = event->data.scalar.length;
    tag = find_implicit_tag(value, len);
  }
  else {
    tag = process_tag(tag);
  }
  *return_tag = tag;

#if DEBUG
  printf("Value: (%s), Tag: (%s)\n", value, tag);
#endif

  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar(value));

  stack_push(stack, 0, NULL, new_prot_object(obj));
  return 0;
}

static int
handle_sequence(event, stack, return_tag)
  yaml_event_t *event;
  s_stack_entry **stack;
  char **return_tag;
{
  s_stack_entry *stack_ptr;
  s_prot_object *obj;
  int count, i, type;
  char *tag;
  SEXP list;

  /* Find out how many elements there are */
  stack_ptr = *stack;
  count = 0;
  while (!stack_ptr->placeholder) {
    count++;
    stack_ptr = stack_ptr->prev;
  }

  /* Initialize list */
  PROTECT(list = allocVector(VECSXP, count));

  /* Populate the list, popping items off the stack as we go */
  type = -2;
  for (i = count - 1; i >= 0; i--) {
    stack_pop(stack, &obj);
    SET_VECTOR_ELT(list, i, obj->obj);
    if (type == -2) {
      type = TYPEOF(obj->obj);
    }
    else if (type != -1 && (TYPEOF(obj->obj) != type || LENGTH(obj->obj) != 1)) {
      type = -1;
    }
    prune_prot_object(obj);
  }

  /* Tags! */
  tag = (*stack)->tag;
  if (tag == NULL) {
    tag = "seq";
  }
  else {
    tag = process_tag(tag);
  }
  *return_tag = tag;

  (*stack)->obj->obj = list;
  (*stack)->obj->seq_type = type;
  (*stack)->placeholder = 0;
  return 0;
}

static int
handle_map(event, stack, return_tag, coerce_keys)
  yaml_event_t *event;
  s_stack_entry **stack;
  char **return_tag;
  int coerce_keys;
{
  s_prot_object *value_obj, *key_obj;
  s_stack_entry *stack_ptr;
  int count, i, j, orphan_key, dup_key = 0;
  SEXP list, keys, key, key_str, value;
  char *tag;

  /* Find out how many elements there are */
  stack_ptr = *stack;
  count = 0;
  while (!stack_ptr->placeholder) {
    count++;
    stack_ptr = stack_ptr->prev;
  }
  count /= 2;

  /* Initialize value list */
  PROTECT(list = allocVector(VECSXP, count));

  /* Initialize key list/vector */
  if (coerce_keys) {
    keys = NEW_STRING(count);
    SET_NAMES(list, keys);
  }
  else {
    keys = allocVector(VECSXP, count);
    setAttrib(list, R_KeysSymbol, keys);
  }

  /* Populate the list, popping items off the stack as we go */
  for (i = count - 1; i >= 0; i--) {
    stack_pop(stack, &value_obj);
    stack_pop(stack, &key_obj);

    SET_VECTOR_ELT(list, i, value_obj->obj);
    dup_key = 0;

    /* map key */
    if (coerce_keys) {
      key_str = AS_CHARACTER(key_obj->obj);
      orphan_key = (key_str != key_obj->obj);
      if (orphan_key) {
        /* This key has been coerced into a character, and is a new object. */
        PROTECT(key_str);
      }

      switch (LENGTH(key_str)) {
        case 0:
          warning("Empty character vector used as a list name");
          key = mkChar("");
          break;
        default:
          warning("Character vector of length greater than 1 used as a list name");
        case 1:
          key = STRING_ELT(key_str, 0);
          break;
      }

      /* Look for duplicate keys */
      if (R_rindex(keys, key, 1, i) >= 0) {
        /* Duplicate found */
        dup_key = 1;
        sprintf(error_msg, "Duplicate map key: '%s'", CHAR(key));
      }

      if (!dup_key) {
        SET_STRING_ELT(keys, i, key);
      }

      if (orphan_key) {
        UNPROTECT(1);
      }
    }
    else {
      /* Look for duplicate keys */
      if (R_rindex(keys, key, 0, i) >= 0) {
        /* Duplicate found */
        dup_key = 1;
        /* FIXME: doesn't print lists properly */
        sprintf(error_msg, "Duplicate map key: %s", R_format(key_obj->obj));
      }

      if (!dup_key) {
        SET_VECTOR_ELT(keys, i, key_obj->obj);
      }
    }
    prune_prot_object(key_obj);
    prune_prot_object(value_obj);

    if (dup_key) {
      break;
    }
  }

  if (dup_key) {
    UNPROTECT(1); // list
    return 1;
  }

  /* Tags! */
  tag = (*stack)->tag;
  if (tag == NULL) {
    tag = "map";
  }
  else {
    tag = process_tag(tag);
  }
  *return_tag = tag;

  (*stack)->obj->obj = list;
  (*stack)->placeholder = 0;
  return 0;
}

static void
possibly_record_alias(anchor, aliases, obj)
  yaml_char_t *anchor;
  s_alias_entry **aliases;
  s_prot_object *obj;
{
  s_alias_entry *alias;

  if (anchor != NULL) {
    alias = (s_alias_entry *)malloc(sizeof(s_alias_entry));
    alias->name = yaml_strdup(anchor);
    alias->obj = obj;
    obj->refcount++;
    alias->prev = *aliases;
    *aliases = alias;
  }
}

SEXP
load_yaml_str(s_str, s_use_named, s_handlers)
  SEXP s_str;
  SEXP s_use_named;
  SEXP s_handlers;
{
  s_prot_object *obj;
  SEXP retval, R_hndlr, names;
  yaml_parser_t parser;
  yaml_event_t event;
  const char *str, *name;
  char *tag;
  long len;
  int use_named, i, done = 0, errorOccurred;
  s_stack_entry *stack = NULL;
  s_alias_entry *aliases = NULL, *alias;

  if (!isString(s_str) || length(s_str) != 1) {
    error("first argument must be a character vector of length 1");
    return R_NilValue;
  }

  if (!isLogical(s_use_named) || length(s_use_named) != 1) {
    error("second argument must be a logical vector of length 1");
    return R_NilValue;
  }

  if (s_handlers == R_NilValue) {
    // Do nothing
  }
  else if (!R_is_named_list(s_handlers)) {
    error("handlers must be either NULL or a named list of functions");
    return R_NilValue;
  }
  else {
    names = GET_NAMES(s_handlers);
    for (i = 0; i < LENGTH(names); i++) {
      name = CHAR(STRING_ELT(names, i));
      R_hndlr = VECTOR_ELT(s_handlers, i);

      if (TYPEOF(R_hndlr) != CLOSXP) {
        warning("your handler for '%s' is not a function; using default", name);
        continue;
      }

      /* custom handlers for merge, default, and anchor#bad are illegal */
      if ( strcmp( name, "merge" ) == 0   ||
           strcmp( name, "default" ) == 0 ||
           strcmp( name, "anchor#bad" ) == 0 )
      {
        warning("custom handling of %s type is not allowed; handler ignored", name);
        continue;
      }
    }
  }

  str = CHAR(STRING_ELT(s_str, 0));
  len = LENGTH(STRING_ELT(s_str, 0));
  use_named = LOGICAL(s_use_named)[0];

  yaml_parser_initialize(&parser);
  yaml_parser_set_input_string(&parser, str, len);

  error_msg[0] = 0;
  while (!done) {
    if (yaml_parser_parse(&parser, &event)) {
      errorOccurred = 0;

      switch (event.type) {
        case YAML_ALIAS_EVENT:
#if DEBUG
          printf("ALIAS: %s\n", event.data.alias.anchor);
#endif
          handle_alias(&event, &stack, aliases);
          break;

        case YAML_SCALAR_EVENT:
#if DEBUG
          printf("SCALAR: %s (%s)\n", event.data.scalar.value, event.data.scalar.tag);
#endif
          errorOccurred = handle_scalar(&event, &stack, &tag);
          if (!errorOccurred) {
            errorOccurred = convert_object(event.type, stack->obj, tag, s_handlers, use_named);
          }
          possibly_record_alias(event.data.scalar.anchor, &aliases, stack->obj);
          break;

        case YAML_SEQUENCE_START_EVENT:
#if DEBUG
          printf("SEQUENCE START: (%s)\n", event.data.sequence_start.tag);
#endif
          handle_start_event(event.data.sequence_start.tag, &stack);
          possibly_record_alias(event.data.sequence_start.anchor, &aliases, stack->obj);
          break;

        case YAML_SEQUENCE_END_EVENT:
#if DEBUG
          printf("SEQUENCE END\n");
#endif
          errorOccurred = handle_sequence(&event, &stack, &tag);
          if (!errorOccurred) {
            errorOccurred = convert_object(event.type, stack->obj, tag, s_handlers, use_named);
          }
          break;

        case YAML_MAPPING_START_EVENT:
#if DEBUG
          printf("MAPPING START: (%s)\n", event.data.mapping_start.tag);
#endif
          handle_start_event(event.data.mapping_start.tag, &stack);
          possibly_record_alias(event.data.mapping_start.anchor, &aliases, stack->obj);
          break;

        case YAML_MAPPING_END_EVENT:
#if DEBUG
          printf("MAPPING END\n");
#endif
          errorOccurred = handle_map(&event, &stack, &tag, use_named);
          if (!errorOccurred) {
            errorOccurred = convert_object(event.type, stack->obj, tag, s_handlers, use_named);
          }

          break;

        case YAML_STREAM_END_EVENT:
          if (stack != NULL) {
            stack_pop(&stack, &obj);
            retval = obj->obj;
            prune_prot_object(obj);
          }
          else {
            retval = R_NilValue;
          }

          done = 1;
          break;
      }

      if (errorOccurred) {
        retval = R_NilValue;
        done = 1;
      }
    }
    else {
      retval = R_NilValue;

      /* Parser error */
      switch (parser.error) {
        case YAML_MEMORY_ERROR:
          sprintf(error_msg, "Memory error: Not enough memory for parsing");
          break;

        case YAML_READER_ERROR:
          if (parser.problem_value != -1) {
            sprintf(error_msg, "Reader error: %s: #%X at %zd", parser.problem,
              parser.problem_value, parser.problem_offset);
          }
          else {
            sprintf(error_msg, "Reader error: %s at %zd", parser.problem,
              parser.problem_offset);
          }
          break;

        case YAML_SCANNER_ERROR:
          if (parser.context) {
            sprintf(error_msg, "Scanner error: %s at line %zd, column %zd"
              "%s at line %zd, column %zd\n", parser.context,
              parser.context_mark.line+1, parser.context_mark.column+1,
              parser.problem, parser.problem_mark.line+1,
              parser.problem_mark.column+1);
          }
          else {
            sprintf(error_msg, "Scanner error: %s at line %zd, column %zd",
              parser.problem, parser.problem_mark.line+1,
              parser.problem_mark.column+1);
          }
          break;

        case YAML_PARSER_ERROR:
          if (parser.context) {
            sprintf(error_msg, "Parser error: %s at line %zd, column %zd"
              "%s at line %zd, column %zd", parser.context,
              parser.context_mark.line+1, parser.context_mark.column+1,
              parser.problem, parser.problem_mark.line+1,
              parser.problem_mark.column+1);
          }
          else {
            sprintf(error_msg, "Parser error: %s at line %zd, column %zd",
              parser.problem, parser.problem_mark.line+1,
              parser.problem_mark.column+1);
          }
          break;

        default:
          /* Couldn't happen. */
          sprintf(error_msg, "Internal error");
          break;
      }
      done = 1;
    }

    yaml_event_delete(&event);
  }

  /* Clean up stack. This only happens if there was an error. */
  while (stack != NULL) {
    stack_pop(&stack, &obj);
    prune_prot_object(obj);
  }

  /* Clean up aliases */
  while (aliases != NULL) {
    alias = aliases;
    aliases = aliases->prev;
    alias->obj->refcount--;
    prune_prot_object(alias->obj);
    free(alias->name);
    free(alias);
  }

  yaml_parser_delete(&parser);

  if (error_msg[0] != 0) {
    error(error_msg);
  }

  return retval;
}

R_CallMethodDef callMethods[] = {
  {"yaml.load",(DL_FUNC)&load_yaml_str, 3},
  {NULL,NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  R_KeysSymbol = install("keys");
  R_IdenticalFunc = findFun(install("identical"), R_GlobalEnv);
  R_FormatFunc = findFun(install("format"), R_GlobalEnv);
  R_registerRoutines(dll,NULL,callMethods,NULL,NULL);
}
