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

#define REAL_BUF_SIZE 256
#define FORMAT_BUF_SIZE 128

/* From implicit.c */
char *find_implicit_tag(const char *value, size_t size);

/* Common functions */
int R_is_named_list(SEXP obj);
SEXP R_collapse(SEXP obj, char *collapse);
const char *R_inspect(SEXP obj);
int R_has_class(SEXP obj, char *name);

/* Exported functions */
SEXP as_yaml(SEXP s_obj, SEXP s_line_sep, SEXP s_indent, SEXP s_omap,
    SEXP s_column_major, SEXP s_unicode, SEXP s_precision,
    SEXP s_indent_mapping_sequence);

SEXP load_yaml_str(SEXP s_str, SEXP s_use_named, SEXP s_handlers);

#endif
