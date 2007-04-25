/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#ifndef ST_INCLUDED

#define ST_INCLUDED

#include "R.h"
#include "Rdefines.h"
#include "Rinternals.h"

#define HASH_ELFHASH

SEXP R_serialize(SEXP object, SEXP icon, SEXP ascii, SEXP fun);

typedef struct st_table_entry st_table_entry;

struct st_table_entry {
    unsigned int hash;
    char *key;
    char *record;
    st_table_entry *next;
};


typedef struct st_table st_table;

struct st_hash_type {
    int (*compare)();
    int (*hash)();
};

struct st_table {
    struct st_hash_type *type;
    int num_bins;
    int num_entries;
    struct st_table_entry **bins;
};

#define st_is_member(table,key) st_lookup(table,key,(char **)0)

enum st_retval {ST_CONTINUE, ST_STOP, ST_DELETE};

st_table *st_init_table(struct st_hash_type *);
st_table *st_init_table_with_size(struct st_hash_type *, int);
st_table *st_init_numtable();
st_table *st_init_numtable_with_size(int);
st_table *st_init_strtable();
st_table *st_init_strtable_with_size(int);
st_table *st_init_Rtable();
st_table *st_init_Rtable_with_size(int);
int st_delete(register st_table *, register char **, char **), st_delete_safe(register st_table *, register char **, char **, char *);
int st_insert_R(register st_table *, register SEXP, SEXP), st_insert(register st_table *, register char *, char *), st_lookup_R(st_table *, register SEXP, SEXP *), st_lookup(st_table *, register char *, char **);
void st_foreach(st_table *, enum st_retval (*)(char *, char *, char *), char *), st_add_direct(st_table *, char *, char *), st_free_table(st_table *), st_cleanup_safe(st_table *, char *);
st_table *st_copy(st_table *);

#define ST_NUMCMP	((int (*)()) 0)
#define ST_NUMHASH	((int (*)()) -2)

#define st_numcmp	ST_NUMCMP
#define st_numhash	ST_NUMHASH

int st_strhash(register char *);
int Rhash(register char *);

#endif /* ST_INCLUDED */
