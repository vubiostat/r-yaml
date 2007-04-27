#include <stdio.h>
#include "R.h"
#include "Rdefines.h"
#include "Rinternals.h"
#include "Rembedded.h"

#define HASH_LOG
#include "../src/st.h"

static char letters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 
                         'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                         's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

void 
assert( char *file_name, unsigned line_num )
{
    fflush( NULL );
    fprintf( stderr, "\nAssertion failed: %s, line %u\n",
             file_name, line_num );
    fflush( stderr );
    abort();
}
# define ASSERT(f) \
    if ( f ) \
        {}   \
    else     \
        assert( __FILE__, __LINE__ )

enum st_retval
printR(st_key, st_value, arg)
  char *st_key;
  char *st_value;
  char *arg;
{
  SEXP key = (SEXP)st_key;
  SEXP val = (SEXP)st_value;

  Rprintf("========== key ==========\n");
  PrintValue(key);
  Rprintf("========= value =========\n");
  PrintValue(val);
  Rprintf("\n");
  
  return ST_CONTINUE;
}

SEXP
make_random_obj()
{
  SEXP obj;
  int i, j, len, len2;
  char *str;

  len = rand() % 100;
  switch(rand() % 3) {
    case 0:
      /* INTSXP */
      PROTECT(obj = NEW_INTEGER(len));
      for (j = 0; j < len; j++)
        INTEGER(obj)[j] = rand() % 12345;

      UNPROTECT(1);
      break;
    case 1:
      /* REALSXP */
      PROTECT(obj = NEW_NUMERIC(len));
      for (j = 0; j < len; j++)
        REAL(obj)[j] = drand48();

      UNPROTECT(1);
      break;
    case 2:
      /* STRSXP */
      PROTECT(obj = NEW_STRING(len));
      for (j = 0; j < len; j++) {
        len2 = rand() % 50;
        str = Calloc(len2, char);
        for (i = 0; i < len2-1; i++)
          str[i] = letters[rand() % 26];
        str[len2-1] = 0;

        SET_STRING_ELT(obj, j, mkChar(str));
        Free(str);
      }

      UNPROTECT(1);
      break;
    case 3:
      /* LGLSXP */
      PROTECT(obj = NEW_LOGICAL(len));
      for (j = 0; j < len; j++)
        LOGICAL(obj)[j] = rand() % 2;

      UNPROTECT(1);
      break;
  }
  return obj;
}

void
test_should_handle_big_hash()
{
  SEXP key, val, tmp, foo;
  int i, j, k, len, len2;
  char *str;
  st_table_entry *entry;
  st_table *map = st_init_Rtable();

  //srand(1337);
  key = val = tmp = NULL;
  for (i = 0; i < 1000; i++) {

    PROTECT(key = make_random_obj());
    PROTECT(val = make_random_obj());
    if (st_lookup_R(map, key, &tmp)) {
      /*
      Rprintf("*** conflicting keys found ***\n");
      Rprintf("============ key =============\n");
      PrintValue(key);
      Rprintf("\n");
      */

      UNPROTECT_PTR(tmp);
    }

    st_insert_R(map, key, val);
    key = val = tmp = NULL;
  }

  /* print */
  /*
  Rprintf("\n\n=== result ===\n\n");
  st_foreach(map, printR, NULL);
  */

  /* clean up */
  for (i = 0; i < map->num_bins; i++) {
    for(entry = map->bins[i]; entry != 0;) {
      UNPROTECT_PTR((SEXP)entry->record);
      UNPROTECT_PTR((SEXP)entry->key);
      entry = entry->next;
    }
  }
  st_free_table(map);
}

int 
main(argc, argv) 
  int argc;
  char **argv;
{
  int   R_argc   = 3;
  char *R_argv[] = {"RembeddedTest", "--silent", "--vanilla"};
  char *tmpdir;

  /* set R_TempDir */
  tmpdir = getenv("TMPDIR");
  if (!tmpdir) {
    tmpdir = getenv("TMP");
    if (!tmpdir) {
      tmpdir = getenv("TEMP");
      if (!tmpdir) 
        tmpdir = "/tmp";
    }
  }
  R_TempDir = tmpdir;
  setenv("R_SESSION_TMPDIR", tmpdir, 1);

  /* make sure R_HOME exists; initialize R engine */
  setenv("R_HOME", RHOME, 0);
  Rf_initEmbeddedR(R_argc, R_argv);

  test_should_handle_big_hash();
  
  return 0;
}
