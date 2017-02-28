#include "r_ext.h"

SEXP R_KeysSymbol = NULL;
SEXP R_IdenticalFunc = NULL;
SEXP R_FormatFunc = NULL;
SEXP R_PasteFunc = NULL;
SEXP R_CollapseSymbol = NULL;
SEXP R_DeparseFunc = NULL;
char error_msg[ERROR_MSG_SIZE];

void
set_error_msg(const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vsnprintf(error_msg, ERROR_MSG_SIZE, format, args);
  if (result >= ERROR_MSG_SIZE) {
    warning("an error occurred, but the message was too long to format properly");

    /* ensure the string is null terminated */
    error_msg[ERROR_MSG_SIZE-1] = 0;
  }
}

/* Returns true if obj is a named list */
int
R_is_named_list(obj)
  SEXP obj;
{
  SEXP names;
  if (TYPEOF(obj) != VECSXP)
    return 0;

  names = GET_NAMES(obj);
  return (TYPEOF(names) == STRSXP && LENGTH(names) == LENGTH(obj));
}

/* Call R's paste() function with collapse */
SEXP
R_collapse(obj, collapse)
  SEXP obj;
  char *collapse;
{
  SEXP call, pcall, retval;

  PROTECT(call = pcall = allocList(3));
  SET_TYPEOF(call, LANGSXP);
  SETCAR(pcall, R_PasteFunc); pcall = CDR(pcall);
  SETCAR(pcall, obj);         pcall = CDR(pcall);
  SETCAR(pcall, PROTECT(allocVector(STRSXP, 1)));
  SET_STRING_ELT(CAR(pcall), 0, mkCharCE(collapse, CE_UTF8));
  SET_TAG(pcall, R_CollapseSymbol);
  retval = eval(call, R_GlobalEnv);
  UNPROTECT(2);

  return retval;
}

/* Return a string representation of the object for error messages */
const char *
R_inspect(obj)
  SEXP obj;
{
  SEXP call, str, result;

  /* Using format/paste here is not really what I want, but without
   * jumping through all kinds of hoops so that I can get the output
   * of print(), this is the most effort I want to put into this. */

  PROTECT(call = lang2(R_FormatFunc, obj));
  str = eval(call, R_GlobalEnv);
  UNPROTECT(1);

  PROTECT(str);
  result = R_collapse(str, " ");
  UNPROTECT(1);

  return CHAR(STRING_ELT(result, 0));
}

/* Return 1 if obj is of the specified class */
int
R_has_class(obj, name)
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

R_CallMethodDef callMethods[] = {
  {"unserialize_from_yaml", (DL_FUNC)&R_unserialize_from_yaml, 4},
  {"serialize_to_yaml",     (DL_FUNC)&R_serialize_to_yaml,     8},
  {NULL, NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  R_KeysSymbol = install("keys");
  R_CollapseSymbol = install("collapse");
  R_IdenticalFunc = findFun(install("identical"), R_GlobalEnv);
  R_FormatFunc = findFun(install("format"), R_GlobalEnv);
  R_PasteFunc = findFun(install("paste"), R_GlobalEnv);
  R_DeparseFunc = findFun(install("deparse"), R_GlobalEnv);
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
}
