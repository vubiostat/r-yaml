#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"
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

  /* This is for tracking whether or not this object has a parent.
   * If there is no parent, that means this object should be UNPROTECT'd
   * when assigned to a parent SEXP object */
  int orphan;
} s_prot_object;

typedef struct {
  s_prot_object *obj;
  int placeholder;
  void *prev;
} s_stack_entry;

typedef struct {
  yaml_char_t *name;
  s_prot_object *obj;
  void *prev;
} s_alias_entry;

static int
Rcmp(x, y)
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

static const char *
Rformat(x)
  SEXP x;
{
  SEXP call, result;

  PROTECT(call = lang2(R_FormatFunc, x));
  result = eval(call, R_GlobalEnv);
  UNPROTECT(1);

  return CHAR(STRING_ELT(result, 0));
}

static s_prot_object *
new_prot_object(obj, orphan)
  SEXP obj;
  int orphan;
{
  s_prot_object *result;

  result = (s_prot_object *)malloc(sizeof(s_prot_object));
  result->refcount = 0;
  result->obj = obj;
  result->orphan = orphan;

  return result;
}

static void
prune_prot_object(obj)
  s_prot_object *obj;
{
  if (obj == NULL)
    return;

  if (obj->obj != NULL && obj->orphan == 1) {
    /* obj is now part of list and is therefore protected */
    UNPROTECT_PTR(obj->obj);
    obj->orphan = 0;
  }

  if (obj->refcount == 0) {
    /* Don't need this object wrapper anymore */
    free(obj);
  }
}

static void
stack_push(stack, placeholder, obj)
  s_stack_entry **stack;
  int placeholder;
  s_prot_object *obj;
{
  s_stack_entry *result;

  result = (s_stack_entry *)malloc(sizeof(s_stack_entry));
  result->placeholder = placeholder;
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
  free(top);

  *stack = result;
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
handle_alias(event, stack, aliases)
  yaml_event_t *event;
  s_stack_entry **stack;
  s_alias_entry *aliases;
{
  s_alias_entry *alias = aliases;
  while (alias) {
    if (strcmp((char *)alias->name, event->data.alias.anchor) == 0) {
      stack_push(stack, 0, alias->obj);
      break;
    }
  }
  return 0;
}

static int
handle_start_event(event, stack)
  yaml_event_t *event;
  s_stack_entry **stack;
{
  stack_push(stack, 1, new_prot_object(NULL, 1));
  return 0;
}

static int
handle_scalar(event, stack, s_handlers)
  yaml_event_t *event;
  s_stack_entry **stack;
  SEXP s_handlers;
{
  SEXP obj, names, cmd, tmp_obj, handler;
  yaml_char_t *tag, *value;
  size_t len;
  int errorOccurred = 0, handled = 0, str = 0, i;
  double f;
  PROTECT_INDEX ipx;

  tag = event->data.scalar.tag;
  value = event->data.scalar.value;
  if (tag == NULL || strcmp(tag, "!") == 0) {
    /* If there's no tag, try to tag it */
    tag = find_implicit_tag(value, len);
  }
  else {
    /* Explicit tag handling */

    if (strncmp(tag, "tag:yaml.org,2002:", 18) == 0) {
      tag = tag + 18;
    }
    else {
      while (*tag == '!') {
        tag++;
      }
    }
  }
#if DEBUG
  printf("Value: (%s), Tag: (%s)\n", value, tag);
#endif

  /* Look for a custom R handler */
  handler = find_handler(s_handlers, tag);
  if (handler != R_NilValue) {
    PROTECT_WITH_INDEX(obj = NEW_STRING(1), &ipx);
    SET_STRING_ELT(obj, 0, mkChar(value));

    PROTECT(cmd = allocVector(LANGSXP, 2));
    SETCAR(cmd, handler);
    SETCADR(cmd, obj);
    tmp_obj = R_tryEval(cmd, R_GlobalEnv, &errorOccurred);
    UNPROTECT(1);

    if (errorOccurred) {
      warning("an error occurred when handling type '%s'; using default handler", tag);
      UNPROTECT(1);
    }
    else {
      handled = 1;
      obj = tmp_obj;
      if (obj == R_NilValue) {
        UNPROTECT(1);
      }
      else {
        REPROTECT(obj, ipx);
      }
    }
  }

  if (!handled) {
    /* default handlers, ordered by most-used */
    if (strcmp(tag, "str") == 0) {
      str = 1;
    }
    else if (strcmp(tag, "int") == 0) {
      PROTECT(obj = NEW_INTEGER(1));
      INTEGER(obj)[0] = (int)strtol((char *)value, NULL, 10);
    }
    else if (strcmp(tag, "float") == 0 || strcmp(tag, "float#fix") == 0) {
      f = strtod((char *)value, NULL);
      PROTECT(obj = NEW_NUMERIC(1));
      REAL(obj)[0] = f;
    }
    else if (strcmp(tag, "bool#yes") == 0) {
      PROTECT(obj = NEW_LOGICAL(1));
      LOGICAL(obj)[0] = 1;
    }
    else if (strcmp(tag, "bool#no") == 0) {
      PROTECT(obj = NEW_LOGICAL(1));
      LOGICAL(obj)[0] = 0;
    }
    else if (strcmp(tag, "int#hex") == 0) {
      PROTECT(obj = NEW_INTEGER(1));
      INTEGER(obj)[0] = (int)strtol((char *)value, NULL, 16);
    }
    else if (strcmp(tag, "int#oct") == 0) {
      PROTECT(obj = NEW_INTEGER(1));
      INTEGER(obj)[0] = (int)strtol((char *)value, NULL, 8);
    }
    else if (strcmp(tag, "float#nan") == 0) {
      PROTECT(obj = NEW_NUMERIC(1));
      REAL(obj)[0] = R_NaN;
    }
    else if (strcmp(tag, "float#inf") == 0) {
      PROTECT(obj = NEW_NUMERIC(1));
      REAL(obj)[0] = R_PosInf;
    }
    else if (strcmp(tag, "float#neginf") == 0) {
      PROTECT(obj = NEW_NUMERIC(1));
      REAL(obj)[0] = R_NegInf;
    }
    else if (strcmp(tag, "null") == 0) {
      obj = R_NilValue;
    }
    else {
      str = 1;
    }

    if (str == 1) {
      PROTECT(obj = NEW_STRING(1));
      SET_STRING_ELT(obj, 0, mkChar((char *)value));
    }
  }

  stack_push(stack, 0, new_prot_object(obj, obj != R_NilValue));
  return 0;
}

static int
handle_sequence(event, stack)
  yaml_event_t *event;
  s_stack_entry **stack;
{
  s_stack_entry *stack_ptr;
  s_prot_object *obj;
  int count, i;
  SEXP list;

  /* Find out how many elements there are */
  stack_ptr = *stack;
  count = 0;
  while (!stack_ptr->placeholder) {
    count++;
    stack_ptr = stack_ptr->prev;
  }

  /* Initialize list */
  list = allocVector(VECSXP, count);
  PROTECT(list);

  /* Populate the list, popping items off the stack as we go */
  for (i = count - 1; i >= 0; i--) {
    stack_pop(stack, &obj);
    SET_VECTOR_ELT(list, i, obj->obj);
    prune_prot_object(obj);
  }
  (*stack)->obj->obj = list;
  (*stack)->placeholder = 0;
  return 0;
}

static int
handle_map(event, stack, coerce)
  yaml_event_t *event;
  s_stack_entry **stack;
  int coerce;
{
  s_prot_object *obj;
  s_stack_entry *stack_ptr;
  int count, i, j, orphan_key, dup_key = 0;
  SEXP list, keys, key, key_str;
  PROTECT_INDEX ipx;

  /* Find out how many elements there are */
  stack_ptr = *stack;
  count = 0;
  while (!stack_ptr->placeholder) {
    count++;
    stack_ptr = stack_ptr->prev;
  }

  /* Initialize value list */
  list = allocVector(VECSXP, count / 2);
  PROTECT_WITH_INDEX(list, &ipx);

  /* Initialize key list/vector */
  if (coerce) {
    PROTECT(keys = NEW_STRING(count / 2));
  }
  else {
    PROTECT(keys = allocVector(VECSXP, count / 2));
  }

  /* Populate the list, popping items off the stack as we go */
  for (i = count - 1; i >= 0; i--) {
    stack_pop(stack, &obj);

    if (i % 2 == 1) {
      /* map value */
      SET_VECTOR_ELT(list, i / 2, obj->obj);
    }
    else {
      dup_key = 0;

      /* map key */
      if (coerce) {
        key_str = AS_CHARACTER(obj->obj);
        orphan_key = (key_str != obj->obj);
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
        for (j = (count / 2) - 1; j > i; j--) {
          if (strcmp(CHAR(key), CHAR(STRING_ELT(keys, j))) == 0) {
            /* Duplicate found */
            dup_key = 1;
            sprintf(error_msg, "Duplicate map key: '%s'", CHAR(key));
            break;
          }
        }

        if (!dup_key) {
          SET_STRING_ELT(keys, i / 2, key);
        }

        if (orphan_key) {
          UNPROTECT(1);
        }
      }
      else {
        /* Look for duplicate keys */
        for (j = (count / 2) - 1; j > i; j--) {
          if (Rcmp(VECTOR_ELT(keys, j), obj->obj) == 0) {
            /* Duplicate found */
            dup_key = 1;
            /* FIXME: doesn't print lists properly */
            sprintf(error_msg, "Duplicate map key: %s", Rformat(obj->obj));
            break;
          }
        }

        if (!dup_key) {
          SET_VECTOR_ELT(keys, i / 2, obj->obj);
        }
      }

    }
    prune_prot_object(obj);

    if (dup_key) {
      break;
    }
  }

  if (!dup_key) {
    if (coerce) {
      SET_NAMES(list, keys);
    }
    else {
      setAttrib(list, R_KeysSymbol, keys);
    }
    (*stack)->obj->obj = list;
    (*stack)->placeholder = 0;
    UNPROTECT(1); // keys
  }
  else {
    UNPROTECT(2); // keys and list
  }

  return dup_key;
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

SEXP
load_yaml_str(s_str, s_use_named, s_handlers)
  SEXP s_str;
  SEXP s_use_named;
  SEXP s_handlers;
{
  s_prot_object *obj;
  SEXP retval, R_hndlr, cmd, names;
  yaml_parser_t parser;
  yaml_event_t event;
  const char *str, *name;
  long len;
  int use_named, i, done = 0, errorOccurred;
  s_stack_entry *stack = NULL, *stack_ptr;
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
          handle_scalar(&event, &stack, s_handlers);
          possibly_record_alias(event.data.scalar.anchor, &aliases, stack->obj);
          break;

        case YAML_SEQUENCE_START_EVENT:
#if DEBUG
          printf("SEQUENCE START: (%s)\n", event.data.sequence_start.tag);
#endif
          handle_start_event(&event, &stack);
          possibly_record_alias(event.data.sequence_start.anchor, &aliases, stack->obj);
          break;

        case YAML_SEQUENCE_END_EVENT:
#if DEBUG
          printf("SEQUENCE END\n");
#endif
          handle_sequence(&event, &stack);
          break;

        case YAML_MAPPING_START_EVENT:
#if DEBUG
          printf("MAPPING START: (%s)\n", event.data.mapping_start.tag);
#endif
          handle_start_event(&event, &stack);
          possibly_record_alias(event.data.mapping_start.anchor, &aliases, stack->obj);
          break;

        case YAML_MAPPING_END_EVENT:
#if DEBUG
          printf("MAPPING END\n");
#endif
          errorOccurred = handle_map(&event, &stack, use_named);
          if (errorOccurred) {
            retval = R_NilValue;
            done = 1;
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
