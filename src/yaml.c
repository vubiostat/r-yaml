#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"

#define HAVE_ST_H
#include "syck.h"

#define PRESERVE(x) R_do_preserve(x)
#define RELEASE(x)  R_do_release(x)

/* R hashing stuff */

static SEXP R_CmpFunc = NULL;
static SEXP R_SerializedSymbol = NULL;

static int
Rcmp(st_x, st_y)
  char *st_x;
  char *st_y;
{
  int i, retval = 0, *arr;
  SEXP call, result;
  SEXP x = (SEXP)st_x;
  SEXP y = (SEXP)st_y;

  if (R_CmpFunc == NULL)
    R_CmpFunc = findFun(install("=="), R_GlobalEnv);

  PROTECT(call = lang3(R_CmpFunc, x, y));
  PROTECT(result = eval(call, R_GlobalEnv));
  
  arr = LOGICAL(result);
  for(i = 0; i < LENGTH(result); i++) {
    if (!arr[i]) {
      retval = 1;
      break;
    }
  }
  UNPROTECT(2);
  return retval;
}

static SEXP
get_or_create_serialized_attr(obj)
  SEXP obj;
{
  SEXP attr, serialized, tru;
  if (R_SerializedSymbol == NULL)
    R_SerializedSymbol = install("serialized");

  attr = getAttrib(obj, R_SerializedSymbol);
  if (attr != R_NilValue)
    return attr;

  PROTECT(tru = NEW_LOGICAL(1));
  LOGICAL(tru)[0] = 1;
  PROTECT(serialized = R_serialize(obj, R_NilValue, tru, R_NilValue));
  UNPROTECT_PTR(tru);

  char str[LENGTH(serialized) + 1];
  strncpy(str, (char *)RAW(serialized), LENGTH(serialized));
  str[LENGTH(serialized)] = 0;

  PROTECT(attr = NEW_STRING(1));
  SET_STRING_ELT(attr, 0, mkChar(str));
  UNPROTECT(2);

  return attr;
}

static int
Rhash(st_obj)
  register char *st_obj;
{
  SEXP obj, serialized;
  int i, hash;
  obj = (SEXP)st_obj;

  /* serialize the object */
  serialized = get_or_create_serialized_attr(obj);
  hash = strhash(CHAR(STRING_ELT(serialized, 0)));

  return hash;
}

static struct st_hash_type type_Rhash = {
    Rcmp,
    Rhash,
};

static st_table* 
st_init_Rtable()
{
    return st_init_table(&type_Rhash);
}

static st_table* 
st_init_Rtable_with_size(size)
    int size;
{
    return st_init_table_with_size(&type_Rhash, size);
}

static int 
st_insert_R(table, key, value)
    register st_table *table;
    register SEXP key;
    SEXP value;
{
    return st_insert(table, (st_data_t)key, (st_data_t)value);
}

static int
st_lookup_R(table, key, value)
    st_table *table;
    register SEXP key;
    SEXP *value;
{
  return st_lookup(table, (st_data_t)key, (st_data_t *)value);
}

typedef struct {
  SEXP key;
  SEXP val;
  void *next;
} linked;

typedef struct {
  int use_named;
  st_table *user_handlers;

  /* handlers */
  SEXP (*null_handler)();
  SEXP (*binary_handler)();
  SEXP (*bool_yes_handler)();
  SEXP (*bool_no_handler)();
  SEXP (*int_hex_handler)();
  SEXP (*int_oct_handler)();
  SEXP (*int_base60_handler)();
  SEXP (*int_handler)();
  SEXP (*float_base60_handler)();
  SEXP (*float_nan_handler)();
  SEXP (*float_inf_handler)();
  SEXP (*float_neginf_handler)();
  SEXP (*float_handler)();
  SEXP (*timestamp_iso8601_handler)();
  SEXP (*timestamp_spaced_handler)();
  SEXP (*timestamp_ymd_handler)();
  SEXP (*timestamp_handler)();
  SEXP (*merge_handler)();
  SEXP (*default_handler)();
  SEXP (*str_handler)();
  SEXP (*anchor_bad_handler)();
  SEXP (*unknown_handler)();
  SEXP (*seq_handler)();
  SEXP (*map_handler)();
} parser_xtra;

static SEXP R_KeysSymbol = NULL;

static void
R_do_preserve(x)
  SEXP x;
{
  if (x != R_NilValue) {
    R_PreserveObject(x);
  }
}

static void
R_do_release(x)
  SEXP x;
{
  if (x != R_NilValue) {
    R_ReleaseObject(x);
  }
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
  SEXP class = GET_CLASS(obj);
  if (TYPEOF(class) == STRSXP)
    return strcmp(CHAR(STRING_ELT(GET_CLASS(obj), 0)), name) == 0;

  return 0;
}

static int 
R_is_named_list( obj )
  SEXP obj;
{
  SEXP names;
  if (TYPEOF(obj) != VECSXP)
    return 0;

  names = GET_NAMES(obj);
  return (TYPEOF(names) == STRSXP && LENGTH(names) == LENGTH(obj));
}

static int 
R_is_pseudo_hash( obj )
  SEXP obj;
{
  SEXP keys;
  if (TYPEOF(obj) != VECSXP)
    return 0;

  keys = getAttrib(obj, R_KeysSymbol);
  return (keys != R_NilValue && TYPEOF(keys) == VECSXP);
}

static void
R_do_map_insert( map, key, value, use_named ) 
  st_table *map;
  SEXP key;
  SEXP value;
  int use_named;
{
  /* do a lookup first to ignore duplicate keys */
  if (!st_lookup_R(map, key, NULL)) {
    if (use_named) {
      /* coerce key to character */
      st_insert_R(map, AS_CHARACTER(key), value);
    }
    else {
      st_insert_R(map, key, value);
    }
  }
}

static int
R_merge_list( map, list, use_named )
  st_table *map;
  SEXP list;
  int use_named;
{
  SEXP tmp, name;
  int i;

  if (use_named) {
    tmp = GET_NAMES(list);
    for ( i = 0; i < LENGTH(list); i++ ) {
      PROTECT(name = NEW_STRING(1));
      SET_STRING_ELT(name, 0, STRING_ELT(tmp, i));
      R_do_map_insert( map, name, VECTOR_ELT(list, i), 0 ); /* don't coerce, since it's been done */
      UNPROTECT(1);
    }
  }
  else {
    tmp = getAttrib(list, R_KeysSymbol);
    for ( i = 0; i < LENGTH(list); i++ ) {
      R_do_map_insert( map, VECTOR_ELT(tmp, i), VECTOR_ELT(list, i), 0 );
    }
  }
}

/* call like this:
 *  tail = add_item(tail, foo); */
static linked*
add_item(tail, key, val)
  linked *tail;
  SEXP key;
  SEXP val;
{
  linked *retval;
  tail->next = retval = Calloc(1, linked);
  retval->next = NULL;
  retval->key  = key;
  retval->val  = val;

  return retval;
}

void
free_list(head)
  linked *head;
{
  linked *tmp;
  tmp = head->next;
  Free(head);

  if (tmp != NULL)
    free_list(tmp);
}

static SEXP 
default_null_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  return R_NilValue;
}

/*
static SEXP 
default_binary_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

static SEXP
default_bool_yes_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_LOGICAL(1);
  LOGICAL(obj)[0] = 1;
  return obj;
}

static SEXP
default_bool_no_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_LOGICAL(1);
  LOGICAL(obj)[0] = 0;
  return obj;
}

static SEXP
default_int_hex_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 16);
  return obj;
}

static SEXP
default_int_oct_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 8);
  return obj;
}

/*
static SEXP
default_int_base60_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

static SEXP
default_int_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 10);
  return obj;
}

static SEXP
default_float_nan_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_NaN;
  return obj;
}

static SEXP
default_float_inf_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_PosInf;
  return obj;
}

static SEXP
default_float_neginf_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_NegInf;
  return obj;
}

static SEXP
default_float_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  double f;
  f = strtod( data, NULL );

  SEXP obj;
  obj = NEW_NUMERIC(1);
  REAL(obj)[0] = f;
  return obj;
}

/*
static SEXP
default_timestamp_iso8601_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

/*
static SEXP
default_timestamp_spaced_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

/*
static SEXP
default_timestamp_ymd_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

/*
static SEXP
default_timestamp_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}
*/

static SEXP
default_merge_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_STRING(1);
  SET_STRING_ELT(obj, 0, mkChar("_yaml.merge_"));
  R_set_class(obj, "_yaml.merge_");
  return obj;
}

static SEXP
default_default_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_STRING(1);
  SET_STRING_ELT(obj, 0, mkChar("_yaml.default_"));
  R_set_class(obj, "_yaml.default_");
  return obj;
}

static SEXP
default_str_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_STRING(1);
  SET_STRING_ELT(obj, 0, mkChar(data));
  return obj;
}

static SEXP
default_anchor_bad_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  SEXP obj = NEW_STRING(1);
  SET_STRING_ELT(obj, 0, mkChar("_yaml.bad-anchor_"));
  R_set_str_attrib(obj, install("name"), data);
  R_set_class(obj, "_yaml.bad-anchor_");
  return obj;
}

static int
find_user_handler(type, xtra, ptr)
  const char *type;
  parser_xtra *xtra;
  SEXP *ptr;
{
  return st_lookup(xtra->user_handlers, (st_data_t)type, (st_data_t *)ptr);
}

static SEXP
run_user_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  int errorOccurred;
  char msg[255];
  SEXP func, tmp_obj, obj;

  // create a string to pass to the handler
  tmp_obj = NEW_STRING(1);
  PROTECT(tmp_obj);
  SET_STRING_ELT(tmp_obj, 0, mkChar(data));

  // find the handler
  find_user_handler(type, xtra, &func);
  if (!func) {
    sprintf(msg, "INTERNAL ERROR: couldn't find the handler for type '%s'!", type);
    error(_(msg));
    UNPROTECT(1);
    return tmp_obj;
  }
  SETCADR(func, tmp_obj);

  obj = R_tryEval(func, R_GlobalEnv, &errorOccurred);
  UNPROTECT(1);

  if (errorOccurred) {
    sprintf(msg, "an error occurred when handling type '%s'; returning string", type);
    warning(_(msg));
    return tmp_obj;
  }
  return obj;
}

static SEXP
default_unknown_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
  // see if there's a user handler for this type
  if (find_user_handler(type, xtra, 0)) {
    run_user_handler(type, data, xtra);
  }
  else {
    // run the string handler
    default_str_handler(type, data, xtra);
  }
}

static SEXP
default_seq_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}

static SEXP
default_map_handler(type, data, xtra)
  const char *type;
  const char *data;
  parser_xtra *xtra;
{
}

/* originally from Ruby's syck extension; modified for R */
static int
yaml_org_handler( p, n, ref )
  SyckParser *p;
  SyckNode *n;
  SEXP *ref;
{
  int             transferred, type, len, total_len, count, do_insert, errorOccurred;
  long            i, j;
  char           *type_id, *data_ptr;
  SYMID           syck_key, syck_val;
  SEXP            obj, s_keys, R_key, R_val, tmp, *k, *v, *list, tmp_obj, R_func, R_cmd;
  st_table       *object_map;
  st_table_entry *entry;
  parser_xtra    *xtra;

  obj         = R_NilValue;
  type_id     = n->type_id;
  transferred = i = j = 0;
  xtra        = (parser_xtra *)p->bonus;

  if ( type_id != NULL && strncmp( type_id, "tag:yaml.org,2002:", 18 ) == 0 )
  {
    type_id += 18;
  }

  switch (n->kind)
  {
    case syck_str_kind:
      transferred = 1;
      data_ptr = n->data.str->ptr;

      if ( type_id == NULL )
      {
        obj = xtra->unknown_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "null" ) == 0 )
      {
        obj = xtra->null_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "binary" ) == 0 )
      {
        obj = xtra->binary_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "bool#yes" ) == 0 )
      {
        obj = xtra->bool_yes_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "bool#no" ) == 0 )
      {
        obj = xtra->bool_no_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "int#hex" ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->int_hex_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "int#oct" ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->int_oct_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "int#base60" ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->int_base60_handler(type_id, data_ptr, xtra);
      }
      else if ( strncmp( type_id, "int", 3 ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->int_handler("int", data_ptr, xtra);
      }
      else if ( strcmp( type_id, "float#base60" ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->float_base60_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "float#nan" ) == 0 )
      {
        obj = xtra->float_nan_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "float#inf" ) == 0 )
      {
        obj = xtra->float_inf_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "float#neginf" ) == 0 )
      {
        obj = xtra->float_neginf_handler(type_id, data_ptr, xtra);
      }
      else if ( strncmp( type_id, "float", 5 ) == 0 )
      {
        syck_str_blow_away_commas( n );
        obj = xtra->float_handler("float", data_ptr, xtra);
      }
      else if ( strcmp( type_id, "timestamp#iso8601" ) == 0 )
      {
        obj = xtra->timestamp_iso8601_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "timestamp#spaced" ) == 0 )
      {
        obj = xtra->timestamp_spaced_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "timestamp#ymd" ) == 0 )
      {
        obj = xtra->timestamp_ymd_handler(type_id, data_ptr, xtra);
      }
      else if ( strncmp( type_id, "timestamp", 9 ) == 0 )
      {
        obj = xtra->timestamp_handler("timestamp", data_ptr, xtra);
      }
      else if ( strncmp( type_id, "merge", 5 ) == 0 )
      {
        obj = xtra->merge_handler("merge", data_ptr, xtra);
      }
      else if ( strncmp( type_id, "default", 7 ) == 0 )
      {
        obj = xtra->default_handler("default", data_ptr, xtra);
      }
/*
 *      else if ( n->data.str->style == scalar_plain &&
 *          n->data.str->len > 1 && 
 *          strncmp( n->data.str->ptr, ":", 1 ) == 0 )
 *      {
 *        obj = rb_funcall( oDefaultResolver, s_transfer, 2, 
 *            rb_str_new2( "tag:ruby.yaml.org,2002:sym" ), 
 *            rb_str_new( n->data.str->ptr + 1, n->data.str->len - 1 ) );
 *      }
 */
      else if ( strcmp( type_id, "str" ) == 0 )
      {
        obj = xtra->str_handler(type_id, data_ptr, xtra);
      }
      else if ( strcmp( type_id, "anchor#bad" ) == 0 )
      {
        obj = xtra->anchor_bad_handler(type_id, data_ptr, xtra);
      }
      else
      {
        transferred = 0;
        obj = xtra->unknown_handler(type_id, data_ptr, xtra);
      }
      if (obj != R_NilValue)
        PRESERVE(obj);
      break;

    case syck_seq_kind:
      if ( type_id == NULL || strcmp( type_id, "seq" ) == 0 )
      {
        transferred = 1;
      }

      /* check the list for uniformity */
      list = Calloc(n->data.list->idx, SEXP);
      type = -1;
      len  = n->data.list->idx;
      total_len = 0;  /* this is for auto-flattening, which is what R does with nested vectors */
      for ( i = 0; i < len; i++ )
      {
        syck_val = syck_seq_read(n, i);
        syck_lookup_sym( p, syck_val, (char **)&R_val );
        if ( i == 0 ) {
          type = TYPEOF(R_val);
        }
        else if ( type >= 0 && type != TYPEOF(R_val) ) {
          type = -1;
        }
        list[i]    = R_val;
        total_len += length(R_val);
      }
      /* only logical, integer, numeric, and character vectors supported for uniformity ATM */
      if (type < 0 || !(type == LGLSXP || type == INTSXP || type == REALSXP || type == STRSXP)) {
        type = VECSXP;
        total_len = len;
      }

      /* allocate object accordingly */
      obj = allocVector(type, total_len);
      PRESERVE(obj);
      switch(type) {
        case VECSXP:
          for ( i = 0; i < len; i++ ) {
            SET_VECTOR_ELT(obj, i, list[i]);
            RELEASE(list[i]);
          }
          break;
          
        case LGLSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              LOGICAL(obj)[count++] = LOGICAL(tmp)[j];
            }
            RELEASE(tmp);
          }
          break;

        case INTSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              INTEGER(obj)[count++] = INTEGER(tmp)[j];
            }
            RELEASE(tmp);
          }
          break;

        case REALSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              REAL(obj)[count++] = REAL(tmp)[j];
            }
            RELEASE(tmp);
          }
          break;

        case STRSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              SET_STRING_ELT(obj, count++, STRING_ELT(tmp, j));
            }
            RELEASE(tmp);
          }
          break;
      }
      Free(list);
      break;

    case syck_map_kind:
      if ( type_id == NULL || strcmp( type_id, "map" ) == 0 )
      {
        transferred = 1;
      }

      /* st_table to temporarily store normal pairs */
      object_map = st_init_Rtable();

      for ( i = 0; i < n->data.pairs->idx; i++ )
      {
        do_insert = 1;
        syck_key = syck_map_read( n, map_key, i );
        syck_val = syck_map_read( n, map_value, i );
        syck_lookup_sym( p, syck_key, (char **)&R_key );
        syck_lookup_sym( p, syck_val, (char **)&R_val );

        /* handle merge keys; see http://yaml.org/type/merge.html */
        if ( R_class_of(R_key, "_yaml.merge_") )
        {
          if ( (xtra->use_named && R_is_named_list(R_val)) || (!xtra->use_named && R_is_pseudo_hash(R_val)) )
          {
            /* i.e.
             *    - &bar { hey: dude }
             *    - foo:
             *        hello: friend
             *        <<: *bar
             */
            do_insert = 0;
            R_merge_list( object_map, R_val, xtra->use_named );
          }
          else if ( TYPEOF(R_val) == VECSXP )
          {
            /* i.e.
             *    - &bar { hey: dude }
             *    - &baz { hi: buddy }
             *    - foo:
             *        hello: friend
             *        <<: [*bar, *baz]
             */
            do_insert = 0;
            for ( j = 0; j < LENGTH(R_val); j++ ) {
              tmp = VECTOR_ELT(R_val, j);
              if ( (xtra->use_named && R_is_named_list(tmp)) || (!xtra->use_named && R_is_pseudo_hash(tmp)) ) {
                R_merge_list( object_map, tmp, xtra->use_named );
              }
              else {
                /* this is probably undesirable behavior; don't write crappy YAML
                 * i.e.
                 *    - &bar { hey: dude }
                 *    - &baz { hi: buddy }
                 *    - foo:
                 *        hello: friend
                 *        <<: [*bar, "bad yaml!", *baz]
                 */
                   
                R_do_map_insert( object_map, R_key, tmp, xtra->use_named );
              }
            }
          }
        }
        /* R doesn't have defaults, doh! */
/*
 *        else if ( rb_obj_is_kind_of( k, cDefaultKey ) )
 *        {
 *          rb_funcall( obj, s_default_set, 1, v );
 *          skip_aset = 1;
 *        }
 */
        
        /* insert into hash if not already done */
        if (do_insert) {
          R_do_map_insert( object_map, R_key, R_val, xtra->use_named ); 
        }
      }

      obj = allocVector(VECSXP, object_map->num_entries);
      PRESERVE(obj);
      if (xtra->use_named) {
        PROTECT(s_keys = NEW_STRING(object_map->num_entries));
      }
      else {
        PROTECT(s_keys = allocVector(VECSXP, object_map->num_entries));
      }

      for(i = 0, j = 0; i < object_map->num_bins; i++) {
        entry = object_map->bins[i];
        while (entry) {
          SET_VECTOR_ELT(obj, j, (SEXP)entry->record);
          RELEASE((SEXP)entry->record);

          if (xtra->use_named) {
            SET_STRING_ELT(s_keys, j, STRING_ELT((SEXP)entry->key, 0));
          }
          else {
            SET_VECTOR_ELT(s_keys, j, (SEXP)entry->key);
          }
          RELEASE((SEXP)entry->key);
          j++;
          entry = entry->next;
        }
      }
      if (xtra->use_named) {
        SET_NAMES(obj, s_keys);
      }
      else {
        setAttrib(obj, R_KeysSymbol, s_keys);
      }
      UNPROTECT(1);

      st_free_table(object_map);
      break;
  }

  *ref = obj;
  return transferred;
}

SyckNode *
R_bad_anchor_handler(p, a)
  SyckParser *p;
  char *a;
{
  SyckNode *n;
  n = syck_new_str( a, scalar_plain );
  n->type_id = syck_strndup( "anchor#bad", 10 );
  return n;
}

SYMID
R_yaml_handler(p, n)
  SyckParser *p;
  SyckNode *n;
{
  int transferred;
  SYMID retval;
  SEXP *obj, tmp;

  obj = Calloc(1, SEXP);
  transferred = yaml_org_handler( p, n, obj );

  /* if n->id > 0, it means that i've run across a bad anchor that was just defined... or something.
   * so i want to overwrite the existing node with this one */
  if (n->id > 0) {
    st_insert( p->syms, (st_data_t)n->id, (st_data_t)(*obj) ); 
  }
  
  retval = syck_add_sym( p, (char *)(*obj) );
  Free(obj);
  return retval;
}

void
R_error_handler(p, msg)
    SyckParser *p;
    char *msg;
{
  char *endl = p->cursor;

  while ( *endl != '\0' && *endl != '\n' )
    endl++;

  endl[0] = '\0';
  error("%s on line %d, col %d: `%s'",
      msg,
      p->linect,
      p->cursor - p->lineptr, 
      p->lineptr); 
}


SEXP 
load_yaml_str(s_str, s_use_named, s_handlers)
  SEXP s_str;
  SEXP s_use_named;
  SEXP s_handlers;
{
  SEXP retval, handler, cmd, names;
  SYMID root_id;
  SyckParser *parser;
  parser_xtra *xtra;
  st_table_entry *entry;
  const char *str, *name;
  char msg[255];
  long len;
  int use_named, i;
  
  if (!isString(s_str) || length(s_str) != 1) {
    error("first argument must be a character vector of length 1");
    return R_NilValue;
  }

  if (!isLogical(s_use_named) || length(s_use_named) != 1) {
    error("second argument must be a logical vector of length 1");
    return R_NilValue;
  }

  if (s_handlers != R_NilValue && !R_is_named_list(s_handlers)) {
    error("handlers must be either NULL or a named list of functions");
    return R_NilValue;
  }

  str = CHAR(STRING_ELT(s_str, 0));
  len = LENGTH(STRING_ELT(s_str, 0));
  use_named = LOGICAL(s_use_named)[0];
  xtra = Calloc(1, parser_xtra);

  // setup data handlers
  xtra->null_handler              = default_null_handler;        
  xtra->binary_handler            = default_str_handler;
  xtra->bool_yes_handler          = default_bool_yes_handler;
  xtra->bool_no_handler           = default_bool_no_handler;
  xtra->int_hex_handler           = default_int_hex_handler;
  xtra->int_oct_handler           = default_int_oct_handler;
  xtra->int_base60_handler        = default_str_handler;
  xtra->int_handler               = default_int_handler;
  xtra->float_base60_handler      = default_str_handler;
  xtra->float_nan_handler         = default_float_nan_handler;
  xtra->float_inf_handler         = default_float_inf_handler;
  xtra->float_neginf_handler      = default_float_neginf_handler;
  xtra->float_handler             = default_float_handler;
  xtra->timestamp_iso8601_handler = default_str_handler;
  xtra->timestamp_spaced_handler  = default_str_handler;
  xtra->timestamp_ymd_handler     = default_str_handler;
  xtra->timestamp_handler         = default_str_handler;
  xtra->merge_handler             = default_merge_handler;
  xtra->default_handler           = default_default_handler;
  xtra->str_handler               = default_str_handler;
  xtra->anchor_bad_handler        = default_anchor_bad_handler;
  xtra->unknown_handler           = default_unknown_handler;
  xtra->seq_handler               = default_seq_handler;
  xtra->map_handler               = default_map_handler;

  // setup user handlers
  xtra->user_handlers = st_init_strtable_with_size(13);

  if (s_handlers != R_NilValue) {
    names = GET_NAMES(s_handlers);
    for (i = 0; i < LENGTH(names); i++) {
      name = CHAR(STRING_ELT(names, i));
      handler = VECTOR_ELT(s_handlers, i);

      if (TYPEOF(handler) != CLOSXP) {
        sprintf(msg, "your handler for '%s' is not a function!", name);
        warning(_(msg));
        continue;
      }

      // custom handlers for merge, default, and anchor#bad are illegal
      if ( strcmp( name, "merge" ) == 0   || 
           strcmp( name, "default" ) == 0 ||
           strcmp( name, "anchor#bad" ) == 0 ) 
      {
        sprintf(msg, "custom handling of %s type is not allowed; handler ignored", name);
        warning(_(msg));
        continue;
      }

      // create function call
      PRESERVE(cmd = allocVector(LANGSXP, 2));
      SETCAR(cmd, handler);

      // put into handler hash
      st_insert(xtra->user_handlers, (st_data_t)name, (st_data_t)cmd);

      // set handler pointer (if a default handler exists) to call user function
      if ( strcmp( name, "null" ) == 0 ) {
        xtra->null_handler = run_user_handler;
      }
      else if ( strcmp( name, "binary" ) == 0 ) {
        xtra->binary_handler = run_user_handler;
      }
      else if ( strcmp( name, "bool#yes" ) == 0 ) {
        xtra->bool_yes_handler = run_user_handler;
      }
      else if ( strcmp( name, "bool#no" ) == 0 ) {
        xtra->bool_no_handler = run_user_handler;
      }
      else if ( strcmp( name, "int#hex" ) == 0 ) {
        xtra->int_hex_handler = run_user_handler;
      }
      else if ( strcmp( name, "int#oct" ) == 0 ) {
        xtra->int_oct_handler = run_user_handler;
      }
      else if ( strcmp( name, "int#base60" ) == 0 ) {
        xtra->int_base60_handler = run_user_handler;
      }
      else if ( strcmp( name, "int" ) == 0 ) {
        xtra->int_handler = run_user_handler;
      }
      else if ( strcmp( name, "float#base60" ) == 0 ) {
        xtra->float_base60_handler = run_user_handler;
      }
      else if ( strcmp( name, "float#nan" ) == 0 ) {
        xtra->float_nan_handler = run_user_handler;
      }
      else if ( strcmp( name, "float#inf" ) == 0 ) {
        xtra->float_inf_handler = run_user_handler;
      }
      else if ( strcmp( name, "float#neginf" ) == 0 ) {
        xtra->float_neginf_handler = run_user_handler;
      }
      else if ( strcmp( name, "float" ) == 0 ) {
        xtra->float_handler = run_user_handler;
      }
      else if ( strcmp( name, "timestamp#iso8601" ) == 0 ) {
        xtra->timestamp_iso8601_handler = run_user_handler;
      }
      else if ( strcmp( name, "timestamp#spaced" ) == 0 ) {
        xtra->timestamp_spaced_handler = run_user_handler;
      }
      else if ( strcmp( name, "timestamp#ymd" ) == 0 ) {
        xtra->timestamp_ymd_handler = run_user_handler;
      }
      else if ( strcmp( name, "timestamp" ) == 0 ) {
        xtra->timestamp_handler = run_user_handler;
      }
      else if ( strcmp( name, "str" ) == 0 ) {
        xtra->str_handler = run_user_handler;
      }
      else if ( strcmp( name, "seq" ) == 0 ) {
        xtra->seq_handler = run_user_handler;
      }
      else if ( strcmp( name, "map" ) == 0 ) {
        xtra->map_handler = run_user_handler;
      }
    }
  }   // end user handlers

  /* setup parser */
  parser = syck_new_parser();
  syck_parser_str( parser, (char *)str, len, NULL);
  syck_parser_handler( parser, &R_yaml_handler );
  syck_parser_bad_anchor_handler( parser, &R_bad_anchor_handler );
  syck_parser_error_handler( parser, &R_error_handler );
  xtra->use_named = use_named;
  parser->bonus = xtra;

  root_id = syck_parse( parser );
  syck_lookup_sym(parser, root_id, (char **)&retval);
  RELEASE(retval);
  syck_free_parser(parser);

  for(i = 0; i < xtra->user_handlers->num_bins; i++) {
    entry = xtra->user_handlers->bins[i];
    while (entry) {
      RELEASE(entry->record);
      entry = entry->next;
    }
  }
  st_free_table(xtra->user_handlers);
  Free(xtra);

  return retval;
}

R_CallMethodDef callMethods[] = {
  {"yaml.load",(DL_FUNC)&load_yaml_str, 3},
  {NULL,NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  R_KeysSymbol = install("keys");
  R_registerRoutines(dll,NULL,callMethods,NULL,NULL);
}
