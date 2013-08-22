#ifndef _R_EXT_H
#define _R_EXT_H

#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"
#include "R_ext/Parse.h"
#include "R_ext/Print.h"
#include "R_ext/PrtUtil.h"
#include "yaml.h"
#include "yaml_private.h"

#define REAL_BUF_SIZE 256
#define FORMAT_BUF_SIZE 128

static SEXP R_KeysSymbol = NULL;
static SEXP R_IdenticalFunc = NULL;
static SEXP R_FormatFunc = NULL;
static SEXP R_PasteFunc = NULL;
static SEXP R_CollapseSymbol = NULL;
static SEXP R_DeparseFunc = NULL;
static SEXP R_NSmallSymbol = NULL;
static SEXP R_TrimSymbol = NULL;
static char error_msg[255];

/* From implicit.c */
yaml_char_t *find_implicit_tag(yaml_char_t *value, size_t size);

/* For keeping track of R objects in the stack */
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

/* Stack entry */
typedef struct {
  /* The R object wrapper. May be NULL if this is a placeholder */
  s_prot_object *obj;

  /* This is 1 when this entry is a placeholder for a map or a
   * sequence. (YAML_SEQUENCE_START_EVENT and YAML_MAPPING_START_EVENT) */
  int placeholder;

  /* The YAML tag for this node */
  yaml_char_t *tag;

  void *prev;
} s_stack_entry;

/* Alias entry */
typedef struct {
  /* Anchor name */
  yaml_char_t *name;

  /* R object wrapper */
  s_prot_object *obj;

  void *prev;
} s_alias_entry;

/* Used when building a map */
typedef struct {
  /* R object that represents the key */
  s_prot_object *key;

  /* R object that represents the value */
  s_prot_object *value;

  /* Tracks whether or not this object is from a merge. If it is,
   * this value could potentially get overwritten by an earlier key. */
  int merged;

  void *prev;
  void *next;
} s_map_entry;

/* For emitting */
typedef struct {
  char *buffer;
  size_t size;
  size_t capa;
} s_emitter_output;

SEXP R_data_class(SEXP, Rboolean);

#endif
