#include <stdio.h>
#include <stdlib.h>
#include "R.h"
#include "Rdefines.h"
#include "R_ext/Rdynload.h"
#include <yaml.h>

typedef struct {
  SEXP obj;
  void *prev;
} s_stack;

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

static SEXP
handle_scalar(event)
  yaml_event_t *event;
{
  SEXP obj;
  PROTECT(obj = NEW_STRING(1));
  SET_STRING_ELT(obj, 0, mkChar((char *)event->data.scalar.value));
  return obj;
}

s_stack *
push(stack, obj)
  s_stack *stack;
  SEXP obj;
{
  s_stack *result;

  result = (s_stack *)malloc(sizeof(s_stack));
  result->obj = obj;
  result->prev = stack;

  return result;
}

SEXP
load_yaml_str(s_str, s_use_named, s_handlers)
  SEXP s_str;
  SEXP s_use_named;
  SEXP s_handlers;
{
  SEXP retval, R_hndlr, cmd, names;
  yaml_parser_t parser;
  yaml_event_t event;
  const char *str, *name;
  char error_msg[255];
  long len;
  int use_named, i, done = 0;
  s_stack *stack = NULL;

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

  yaml_parser_initialize(&parser);
  yaml_parser_set_input_string(&parser, str, len);

  error_msg[0] = 0;
  while (!done) {
    if (yaml_parser_parse(&parser, &event)) {
      switch (event.type) {
        case YAML_ALIAS_EVENT:
          printf("ALIAS: %s\n", event.data.alias.anchor);
          break;
        case YAML_SCALAR_EVENT:
          printf("SCALAR: %s (%s)\n", event.data.scalar.value, event.data.scalar.tag);
          stack = push(stack, handle_scalar(&event));
          break;
        case YAML_SEQUENCE_START_EVENT:
          printf("SEQUENCE START: (%s)\n", event.data.sequence_start.tag);
          break;
        case YAML_SEQUENCE_END_EVENT:
          printf("SEQUENCE END\n");
          break;
        case YAML_MAPPING_START_EVENT:
          printf("MAPPING START: (%s)\n", event.data.mapping_start.tag);
          break;
        case YAML_MAPPING_END_EVENT:
          printf("MAPPING END\n");
          break;
        case YAML_STREAM_END_EVENT:
          if (stack != NULL) {
            retval = stack->obj;
            UNPROTECT(1);
            free(stack);
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
              "%s at line %zd, column %zd", parser.context,
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

  yaml_parser_delete(&parser);

  if (error_msg[0] != 0) {
    error(error_msg);
  }

  return retval;;
}

R_CallMethodDef callMethods[] = {
  {"yaml.load",(DL_FUNC)&load_yaml_str, 3},
  {NULL,NULL, 0}
};

void R_init_yaml(DllInfo *dll) {
  R_registerRoutines(dll,NULL,callMethods,NULL,NULL);
}
