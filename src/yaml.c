#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"

#define HAVE_ST_H
#include "syck.h"

#define PRESERVE(x) R_do_preserve(x)
#define RELEASE(x)  R_do_release(x)

typedef size_t R_size_t;
typedef struct membuf_st {
  R_size_t size;
  R_size_t count;
  unsigned char *buf;
  int errorOccurred;
} *membuf_t;

typedef struct {
  SEXP (*func)();
  SEXP R_cmd;  /* for private data */
} handler;

typedef struct {
  int use_named;
  st_table *handlers;
} parser_xtra;

static int Rhash(register char*);
static int Rcmp(char*, char*);

static struct st_hash_type
type_Rhash = {
    Rcmp,
    Rhash,
};
static SEXP R_CmpFunc = NULL;
static SEXP R_SerializedSymbol = NULL;
static SEXP R_KeysSymbol = NULL;
static SEXP YAML_Namespace = NULL;


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

/* NOTE: this is where all of the preserved R objects get released */
static void
free_all(p)
  SyckParser *p;
{
  int i;
  st_table_entry *entry;
  handler *hndlr;
  parser_xtra *xtra = (parser_xtra *)p->bonus;

  /* release R objects */
  if (p->syms) {
    for(i = 0; i < p->syms->num_bins; i++) {
      entry = p->syms->bins[i];
      while (entry) {
        RELEASE(entry->record);
        entry = entry->next;
      }
    }
  }

  /* free xtra */
  for(i = 0; i < xtra->handlers->num_bins; i++) {
    entry = xtra->handlers->bins[i];
    while (entry) {
      hndlr = (handler *)entry->record;
      RELEASE(hndlr->func);
      Free(hndlr);
      entry = entry->next;
    }
  }
  st_free_table(xtra->handlers);
  Free(xtra);
  syck_free_parser(p);
}

/* stuff needed for serialization; why isn't this exposed?? */
static void resize_buffer(membuf_t mb, R_size_t needed)
{
  /* This used to allocate double 'needed', but that was problematic for
     large buffers */
  R_size_t newsize = needed;

  mb->buf = realloc(mb->buf, newsize);
  if (mb->buf == NULL) {
    mb->errorOccurred = 1;
    return;
  }

  mb->size = newsize;
}

static void free_mem_buffer(void *data)
{
  membuf_t mb = data;
  if (mb->buf != NULL) {
    unsigned char *buf = mb->buf;
    mb->buf = NULL;
    free(buf);
  }
}

static void OutCharMem(R_outpstream_t stream, int c)
{
  membuf_t mb = stream->data;
  if (mb->errorOccurred)
    return;

  if (mb->count >= mb->size)
    resize_buffer(mb, mb->count + 50);
  if (mb->errorOccurred)
    return;

  mb->buf[mb->count++] = c;
}

static void OutBytesMem(R_outpstream_t stream, void *buf, int length)
{
  membuf_t mb = stream->data;
  if (mb->errorOccurred)
    return;

  R_size_t needed = mb->count + (R_size_t) length;

  if (needed > mb->size)
    resize_buffer(mb, needed + 50);
  if (mb->errorOccurred)
    return;

  memcpy(mb->buf + mb->count, buf, length);
  mb->count = needed;
}

/* R hashing stuff */
static int
Rcmp(st_x, st_y)
  char *st_x;
  char *st_y;
{
  int i, retval = 0, *arr;
  SEXP call, result;
  SEXP x = (SEXP)st_x;
  SEXP y = (SEXP)st_y;

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
create_serialized_attr(obj, errorOccurred)
  SEXP obj;
  int *errorOccurred;
{
  SEXP attr, serialized;
  struct R_outpstream_st out;
  struct membuf_st mbs;

  /* create a stream for serialization */
  mbs.count = 0;
  mbs.size  = 0;
  mbs.buf   = NULL;
  mbs.errorOccurred = 0;
  R_InitOutPStream(&out, (R_pstream_data_t)&mbs, R_pstream_ascii_format, 0,
                   OutCharMem, OutBytesMem, NULL, R_NilValue);
  R_Serialize(obj, &out);
  OutCharMem(&out, 0);

  if (!mbs.errorOccurred) {
    PROTECT(attr = NEW_STRING(1));
    SET_STRING_ELT(attr, 0, mkChar(mbs.buf));
    setAttrib(obj, R_SerializedSymbol, attr);
    UNPROTECT(1);
    *errorOccurred = 0;
  }
  else {
    attr = R_NilValue;
    *errorOccurred = 1;
  }

  free_mem_buffer(&mbs);
  return attr;
}

static int
Rhash(st_obj)
  register char *st_obj;
{
  SEXP obj, key;
  int hash;
  obj = (SEXP)st_obj;
  key = getAttrib(obj, R_SerializedSymbol);

  /* serialize the object */
  hash = strhash(CHAR(STRING_ELT(key, 0)));

  return hash;
}

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
/* end R hashing */


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

static int
R_do_map_insert( map, key, value, use_named )
  st_table *map;
  SEXP key;
  SEXP value;
  int use_named;
{
  SEXP k, s;
  int errorOccurred = 0;

  if (use_named) {
    /* coerce key to character */
    k = AS_CHARACTER(key);
    PRESERVE(k);
  }
  else
    k = key;

  /* serialize the key if necessary */
  s = getAttrib(k, R_SerializedSymbol);
  if (s == R_NilValue)
    create_serialized_attr(k, &errorOccurred);

  if (errorOccurred)
    return 0;

  /* only insert if lookup fails; this ignores duplicate keys */
  if (!st_lookup_R(map, k, NULL)) {
    st_insert_R(map, k, value);
  }
  return 1;
}

static int
R_merge_list( map, list, use_named )
  st_table *map;
  SEXP list;
  int use_named;
{
  SEXP tmp, name;
  int i, success;

  if (use_named) {
    tmp = GET_NAMES(list);
    for ( i = 0; i < LENGTH(list); i++ ) {
      PROTECT(name = NEW_STRING(1));
      SET_STRING_ELT(name, 0, STRING_ELT(tmp, i));

      success = R_do_map_insert( map, name, VECTOR_ELT(list, i), 0 );
      UNPROTECT(1);

      if (!success)
        break;
    }
  }
  else {
    tmp = getAttrib(list, R_KeysSymbol);
    for ( i = 0; i < LENGTH(list); i++ ) {
      success = R_do_map_insert( map, VECTOR_ELT(tmp, i), VECTOR_ELT(list, i), 0 );

      if (!success)
        break;
    }
  }
  return success;
}

static SEXP
default_null_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  return R_NilValue;
}

/*
static SEXP
default_binary_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

static SEXP
default_bool_yes_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_LOGICAL(1);
  LOGICAL(obj)[0] = 1;
  return obj;
}

static SEXP
default_bool_no_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_LOGICAL(1);
  LOGICAL(obj)[0] = 0;
  return obj;
}

static SEXP
default_int_hex_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 16);
  return obj;
}

static SEXP
default_int_oct_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 8);
  return obj;
}

/*
static SEXP
default_int_base60_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

static SEXP
default_int_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_INTEGER(1);
  INTEGER(obj)[0] = (int)strtol(data, NULL, 10);
  return obj;
}

static SEXP
default_float_nan_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_NaN;
  return obj;
}

static SEXP
default_float_inf_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_PosInf;
  return obj;
}

static SEXP
default_float_neginf_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj = NEW_NUMERIC(1);
  REAL(obj)[0] = R_NegInf;
  return obj;
}

static SEXP
default_float_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
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
default_timestamp_iso8601_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

/*
static SEXP
default_timestamp_spaced_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

/*
static SEXP
default_timestamp_ymd_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

/*
static SEXP
default_timestamp_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

static SEXP
default_merge_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj;
  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar("_yaml.merge_"));
  R_set_class(obj, "_yaml.merge_");
  UNPROTECT(1);

  return obj;
}

static SEXP
default_default_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj;
  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar("_yaml.default_"));
  R_set_class(obj, "_yaml.default_");
  UNPROTECT(1);

  return obj;
}

static SEXP
default_str_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj;
  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar(data));
  UNPROTECT(1);

  return obj;
}

static SEXP
default_anchor_bad_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP obj;
  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar("_yaml.bad-anchor_"));
  R_set_str_attrib(obj, install("name"), data);
  R_set_class(obj, "_yaml.bad-anchor_");
  UNPROTECT(1);

  return obj;
}

/*
static SEXP
default_unknown_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
}
*/

static SEXP
run_R_handler_function(func, arg, type)
  SEXP func;
  SEXP arg;
  const char *type;
{
  SEXP obj;
  int errorOccurred = 0;
  char error_msg[255];

  SETCADR(func, arg);
  obj = R_tryEval(func, YAML_Namespace, &errorOccurred);

  if (errorOccurred) {
    sprintf(error_msg, "an error occurred when handling type '%s'; returning default object", type);
    warning(_(error_msg));
    return arg;
  }
  return obj;
}

static SEXP
run_user_handler(type, data, R_cmd)
  const char *type;
  const char *data;
  SEXP R_cmd;
{
  SEXP tmp_obj, obj;

  /* create a string to pass to the handler */
  tmp_obj = NEW_STRING(1);
  PROTECT(tmp_obj);
  SET_STRING_ELT(tmp_obj, 0, mkChar(data));

  /* call R function */
  obj = run_R_handler_function(R_cmd, tmp_obj, type);
  UNPROTECT(1);

  return obj;
}

static SEXP
find_and_run_handler(p, type, data)
  SyckParser *p;
  const char *type;
  const char *data;
{
  handler *hndlr;
  st_table *hash = ((parser_xtra *)p->bonus)->handlers;

  if (!st_lookup(hash, (st_data_t)type, (st_data_t *)&hndlr))
    st_lookup(hash, (st_data_t)"unknown", (st_data_t *)&hndlr);

  return hndlr->func(type, data, hndlr->R_cmd);
}

/* originally from Ruby's syck extension; modified for R */
static void
yaml_org_handler( p, n, ref )
  SyckParser *p;
  SyckNode *n;
  SEXP *ref;
{
  int             type, len, total_len, count, do_insert, errorOccurred, success, custom;
  long            i, j;
  char           *type_id, *data_ptr;
  SYMID           syck_key, syck_val;
  SEXP            obj, s_keys, R_key, R_val, tmp, *k, *v, *list, tmp_obj, R_func, R_cmd;
  st_table       *object_map;
  st_table_entry *entry;
  parser_xtra    *xtra;
  handler        *hndlr;

  i = j = errorOccurred = 0;
  obj     = R_NilValue;
  type_id = n->type_id;
  xtra    = (parser_xtra *)p->bonus;

  if ( type_id != NULL && strncmp( type_id, "tag:yaml.org,2002:", 18 ) == 0 ) {
    type_id += 18;
  }

  switch (n->kind)
  {
    case syck_str_kind:
      data_ptr = n->data.str->ptr;

      if ( type_id == NULL ) {
        obj = find_and_run_handler(p, "unknown", data_ptr);
      }
      else {
        if ( strncmp( type_id, "int", 3 ) == 0 || strncmp( type_id, "float", 5 ) == 0 )
          syck_str_blow_away_commas( n );

        obj = find_and_run_handler(p, type_id, data_ptr);
      }

      PRESERVE(obj);
      break;

    case syck_seq_kind:
      /* if there's a user handler for this type, we should always construct a list */
      custom = st_lookup(xtra->handlers, (st_data_t)(type_id == NULL ? "seq" : type_id), (st_data_t *)&hndlr);

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

      /* check the list for uniformity; only logical, integer, numeric, and character
       * vectors are supported for uniformity ATM */
      if (custom || type < 0 || !(type == LGLSXP || type == INTSXP || type == REALSXP || type == STRSXP)) {
        type = VECSXP;
        total_len = len;
      }

      /* allocate object accordingly */
      obj = allocVector(type, total_len);
      PROTECT(obj);
      switch(type) {
        case VECSXP:
          for ( i = 0; i < len; i++ ) {
            SET_VECTOR_ELT(obj, i, list[i]);
          }
          break;

        case LGLSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              LOGICAL(obj)[count++] = LOGICAL(tmp)[j];
            }
          }
          break;

        case INTSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              INTEGER(obj)[count++] = INTEGER(tmp)[j];
            }
          }
          break;

        case REALSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              REAL(obj)[count++] = REAL(tmp)[j];
            }
          }
          break;

        case STRSXP:
          for ( i = 0, count = 0; i < len; i++ ) {
            tmp = list[i];
            for ( j = 0; j < LENGTH(tmp); j++ ) {
              SET_STRING_ELT(obj, count++, STRING_ELT(tmp, j));
            }
          }
          break;
      }
      Free(list);

      if (custom) {
        /* hndlr->func isn't even used in this case, which might need to change at some point */
        tmp_obj = run_R_handler_function(hndlr->R_cmd, obj, type_id == NULL ? "seq" : type_id);
        UNPROTECT_PTR(obj);
        obj = tmp_obj;
      }
      else
        UNPROTECT_PTR(obj);

      PRESERVE(obj);
      break;

    case syck_map_kind:
      custom = st_lookup(xtra->handlers, (st_data_t)(type_id == NULL ? "map" : type_id), (st_data_t *)&hndlr);

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
            success = R_merge_list( object_map, R_val, xtra->use_named );
            if (!success) {
              st_free_table(object_map);
              free_all(p);
              error(_("memory allocation error"));
            }
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
                success = R_merge_list( object_map, tmp, xtra->use_named );
                if (!success) {
                  st_free_table(object_map);
                  free_all(p);
                  error(_("memory allocation error"));
                }
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

                success = R_do_map_insert( object_map, R_key, tmp, xtra->use_named );
                if (!success) {
                  st_free_table(object_map);
                  free_all(p);
                  error(_("memory allocation error"));
                }
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
          success = R_do_map_insert( object_map, R_key, R_val, xtra->use_named );
          if (!success) {
            st_free_table(object_map);
            free_all(p);
            error(_("memory allocation error"));
          }
        }
      }

      obj = allocVector(VECSXP, object_map->num_entries);
      PROTECT(obj);
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

          if (xtra->use_named) {
            SET_STRING_ELT(s_keys, j, STRING_ELT((SEXP)entry->key, 0));

            /* This is necessary here because the key was coerced into a string, which
             * may or may not have created a new R object.  Even if it didn't create
             * a new object, it doesn't hurt anything to release the object more than once
             * (except losing a few clock cycles). */
            RELEASE((SEXP)entry->key);
          }
          else {
            SET_VECTOR_ELT(s_keys, j, (SEXP)entry->key);
          }
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
      UNPROTECT_PTR(s_keys);

      st_free_table(object_map);

      if (custom) {
        /* hndlr->func isn't even used in this case, which might need to change at some point */
        tmp_obj = run_R_handler_function(hndlr->R_cmd, obj, type_id == NULL ? "map" : type_id);
        UNPROTECT_PTR(obj);
        obj = tmp_obj;
      }
      else
        UNPROTECT_PTR(obj);

      PRESERVE(obj);
      break;
  }

  *ref = obj;
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
  SYMID retval;
  SEXP *obj, tmp;

  obj = Calloc(1, SEXP);
  yaml_org_handler( p, n, obj );

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
  char error_msg[255];

  while ( *endl != '\0' && *endl != '\n' )
    endl++;

  endl[0] = '\0';

  sprintf(error_msg, "%s on line %d, col %d: `%s'",
      msg,
      p->linect,
      p->cursor - p->lineptr,
      p->lineptr);
  free_all(p);
  error(_(error_msg));
}

static void
setup_handler(p, type, func, R_cmd)
  SyckParser *p;
  const char *type;
  SEXP (*func)();
  SEXP R_cmd;
{
  st_table *hash;
  handler *hndlr;
  int found;

  hash  = ((parser_xtra *)p->bonus)->handlers;
  found = st_lookup(hash, (st_data_t)type, (st_data_t *)&hndlr);
  if (!found) {
    hndlr = Calloc(1, handler);
    if (hndlr == NULL) {
      free_all(p);
      error(_("memory allocation error"));
    }
  }

  hndlr->func  = func;
  hndlr->R_cmd = R_cmd;

  if (!found)
    st_insert(hash, (st_data_t)type, (st_data_t)hndlr);
}

SEXP
load_yaml_str(s_str, s_use_named, s_handlers)
  SEXP s_str;
  SEXP s_use_named;
  SEXP s_handlers;
{
  SEXP retval, R_hndlr, cmd, names;
  SYMID root_id;
  SyckParser *parser;
  parser_xtra *xtra;
  const char *str, *name;
  char error_msg[255];
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

  parser = syck_new_parser();
  xtra = Calloc(1, parser_xtra);
  if (xtra == NULL)
    error(_("memory allocation error"));
  xtra->use_named = use_named;
  parser->bonus = xtra;
  xtra->handlers = st_init_strtable_with_size(23);

  /* setup default handlers */
  setup_handler(parser, "null",              default_null_handler,         R_NilValue);
  setup_handler(parser, "binary",            default_str_handler,          R_NilValue);
  setup_handler(parser, "bool#yes",          default_bool_yes_handler,     R_NilValue);
  setup_handler(parser, "bool#no",           default_bool_no_handler,      R_NilValue);
  setup_handler(parser, "int#hex",           default_int_hex_handler,      R_NilValue);
  setup_handler(parser, "int#oct",           default_int_oct_handler,      R_NilValue);
  setup_handler(parser, "int#base60",        default_str_handler,          R_NilValue);
  setup_handler(parser, "int",               default_int_handler,          R_NilValue);
  setup_handler(parser, "float#base60",      default_str_handler,          R_NilValue);
  setup_handler(parser, "float#nan",         default_float_nan_handler,    R_NilValue);
  setup_handler(parser, "float#inf",         default_float_inf_handler,    R_NilValue);
  setup_handler(parser, "float#neginf",      default_float_neginf_handler, R_NilValue);
  setup_handler(parser, "float#fix",         default_float_handler,        R_NilValue);
  setup_handler(parser, "float",             default_float_handler,        R_NilValue);
  setup_handler(parser, "timestamp#iso8601", default_str_handler,          R_NilValue);
  setup_handler(parser, "timestamp#spaced",  default_str_handler,          R_NilValue);
  setup_handler(parser, "timestamp#ymd",     default_str_handler,          R_NilValue);
  setup_handler(parser, "timestamp",         default_str_handler,          R_NilValue);
  setup_handler(parser, "merge",             default_merge_handler,        R_NilValue);
  setup_handler(parser, "default",           default_default_handler,      R_NilValue);
  setup_handler(parser, "str",               default_str_handler,          R_NilValue);
  setup_handler(parser, "anchor#bad",        default_anchor_bad_handler,   R_NilValue);
  setup_handler(parser, "unknown",           default_str_handler,          R_NilValue);

//  xtra->seq_handler               = 0;
//  xtra->map_handler               = 0;

  /* find user handlers */
  if (s_handlers != R_NilValue) {
    names = GET_NAMES(s_handlers);
    for (i = 0; i < LENGTH(names); i++) {
      name = CHAR(STRING_ELT(names, i));
      R_hndlr = VECTOR_ELT(s_handlers, i);

      if (TYPEOF(R_hndlr) != CLOSXP) {
        sprintf(error_msg, "your handler for '%s' is not a function; using default", name);
        warning(_(error_msg));
        continue;
      }

      /* custom handlers for merge, default, and anchor#bad are illegal */
      if ( strcmp( name, "merge" ) == 0   ||
           strcmp( name, "default" ) == 0 ||
           strcmp( name, "anchor#bad" ) == 0 )
      {
        sprintf(error_msg, "custom handling of %s type is not allowed; handler ignored", name);
        warning(_(error_msg));
        continue;
      }

      /* create function call */
      PRESERVE(cmd = allocVector(LANGSXP, 2));
      SETCAR(cmd, R_hndlr);

      /* put into handler hash */
      setup_handler(parser, name, run_user_handler, cmd);
    }
  }   /* end user handlers */

  /* setup parser hooks */
  syck_parser_str( parser, (char *)str, len, NULL );
  syck_parser_handler( parser, &R_yaml_handler );
  syck_parser_bad_anchor_handler( parser, &R_bad_anchor_handler );
  syck_parser_error_handler( parser, &R_error_handler );

  root_id = syck_parse( parser );
  if (syck_lookup_sym(parser, root_id, (char **)&retval) == 0) {
    // empty document
    retval = R_NilValue;
  }
  free_all(parser);

  return retval;
}

SEXP
set_yaml_env(envir)
  SEXP envir;
{
  if(envir && !isNull(envir))
    YAML_Namespace = envir;

  if(!YAML_Namespace)
    YAML_Namespace = R_GlobalEnv;

  return envir;
}

R_CallMethodDef callMethods[] = {
  {"yaml.load",(DL_FUNC)&load_yaml_str, 3},
  {"setYAMLenv",(DL_FUNC)&set_yaml_env, 1},
  {NULL,NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  R_KeysSymbol = install("keys");
  R_SerializedSymbol = install("serialized");
  R_CmpFunc = findFun(install("=="), R_GlobalEnv);
  R_registerRoutines(dll,NULL,callMethods,NULL,NULL);
}
