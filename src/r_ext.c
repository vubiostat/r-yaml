#include "r_ext.h"

SEXP Ryaml_KeysSymbol = NULL;
SEXP Ryaml_IdenticalFunc = NULL;
SEXP Ryaml_FormatFunc = NULL;
SEXP Ryaml_PasteFunc = NULL;
SEXP Ryaml_CollapseSymbol = NULL;
SEXP Ryaml_DeparseFunc = NULL;
SEXP Ryaml_Sentinel = NULL;
SEXP Ryaml_SequenceStart = NULL;
SEXP Ryaml_MappingStart = NULL;
SEXP Ryaml_MappingEnd = NULL;
char Ryaml_error_msg[ERROR_MSG_SIZE];

void
Ryaml_set_error_msg(const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vsnprintf(Ryaml_error_msg, ERROR_MSG_SIZE, format, args);
  if (result >= ERROR_MSG_SIZE) {
    warning("an error occurred, but the message was too long to format properly");

    /* ensure the string is null terminated */
    Ryaml_error_msg[ERROR_MSG_SIZE-1] = 0;
  }
}

/* Returns true if obj is a named list */
int
Ryaml_is_named_list(obj)
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
Ryaml_collapse(obj, collapse)
  SEXP obj;
  char *collapse;
{
  SEXP call, pcall, retval;

  PROTECT(call = pcall = allocList(3));
  SET_TYPEOF(call, LANGSXP);
  SETCAR(pcall, Ryaml_PasteFunc); pcall = CDR(pcall);
  SETCAR(pcall, obj);         pcall = CDR(pcall);
  SETCAR(pcall, PROTECT(allocVector(STRSXP, 1)));
  SET_STRING_ELT(CAR(pcall), 0, mkCharCE(collapse, CE_UTF8));
  SET_TAG(pcall, Ryaml_CollapseSymbol);
  retval = eval(call, R_GlobalEnv);
  UNPROTECT(2);

  return retval;
}

/* Return a string representation of the object for error messages */
SEXP
Ryaml_inspect(obj)
  SEXP obj;
{
  SEXP call, str, result;

  /* Using format/paste here is not really what I want, but without
   * jumping through all kinds of hoops so that I can get the output
   * of print(), this is the most effort I want to put into this. */

  PROTECT(call = lang2(Ryaml_FormatFunc, obj));
  str = eval(call, R_GlobalEnv);
  UNPROTECT(1);

  PROTECT(str);
  result = Ryaml_collapse(str, " ");
  UNPROTECT(1);

  return result;
}

/* Return 1 if obj is of the specified class */
int
Ryaml_has_class(s_obj, name)
  SEXP s_obj;
  char *name;
{
  SEXP s_class = NULL;
  int i = 0, len = 0, result = 0;

  PROTECT(s_obj);
  PROTECT(s_class = GET_CLASS(s_obj));
  if (TYPEOF(s_class) == STRSXP) {
    len = length(s_class);
    for (i = 0; i < len; i++) {
      if (strcmp(CHAR(STRING_ELT(s_class, i)), name) == 0) {
        result = 1;
        break;
      }
    }
  }
  UNPROTECT(2);
  return result;
}

R_CallMethodDef callMethods[] = {
  {"unserialize_from_yaml", (DL_FUNC)&Ryaml_unserialize_from_yaml, 6},
  {"serialize_to_yaml",     (DL_FUNC)&Ryaml_serialize_to_yaml,     8},
  {NULL, NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  Ryaml_KeysSymbol = install("keys");
  Ryaml_CollapseSymbol = install("collapse");
  Ryaml_IdenticalFunc = findFun(install("identical"), R_GlobalEnv);
  Ryaml_FormatFunc = findFun(install("format"), R_GlobalEnv);
  Ryaml_PasteFunc = findFun(install("paste"), R_GlobalEnv);
  Ryaml_DeparseFunc = findFun(install("deparse"), R_GlobalEnv);
  Ryaml_Sentinel = install("sentinel");
  Ryaml_SequenceStart = install("sequence.start");
  Ryaml_MappingStart = install("mapping.start");
  Ryaml_MappingEnd = install("mapping.end");
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
