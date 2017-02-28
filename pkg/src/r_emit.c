#include "r_ext.h"

extern SEXP R_DeparseFunc;
extern char error_msg[ERROR_MSG_SIZE];

typedef struct {
  char *buffer;
  size_t size;
  size_t capa;
} s_emitter_output;

static SEXP
R_deparse_function(s_obj)
  SEXP s_obj;
{
  SEXP s_call, s_result, s_chr;
  int i, j, res_len, chr_len, str_len, str_end;
  char *str;

  /* first get R's deparsed character vector */
  PROTECT(s_call = lang2(R_DeparseFunc, s_obj));
  s_result = eval(s_call, R_GlobalEnv);
  UNPROTECT(1);

  str_len = 0;
  res_len = length(s_result);
  for (i = 0; i < res_len; i++) {
    str_len += length(STRING_ELT(s_result, i));
  }
  str_len += length(s_result);  /* for newlines */

  /* The point of this is to collapse the deparsed function whilst
   * eliminating trailing spaces. LibYAML's emitter won't output
   * a string with trailing spaces as a multiline scalar. */
  str = (char *)malloc(sizeof(char) * (str_len + 1));
  str_end = 0;
  for (i = 0; i < res_len; i++) {
    s_chr = STRING_ELT(s_result, i);
    chr_len = length(s_chr);
    memcpy((void *)(str + str_end), (void *)CHAR(s_chr), chr_len);
    str_end += chr_len;

    /* find place to terminate line */
    for (j = str_end - 1; j > 0; j--) {
      if (str[j] != ' ') {
        break;
      }
    }
    str[j + 1] = '\n';
    str_end = j + 2;
  }

  /* null terminate string */
  str[str_end] = 0;

  PROTECT(s_result = allocVector(STRSXP, 1));
  SET_STRING_ELT(s_result, 0, mkCharCE(str, CE_UTF8));
  UNPROTECT(1);
  free(str);

  return s_result;
}

/* Format a vector of reals for emitting */
static SEXP
R_format_real(obj, precision)
  SEXP obj;
  int precision;
{
  SEXP retval;
  int i, j, k, n, suffix_len;
  double x, e;
  char str[REAL_BUF_SIZE], format[5] = "%.*f", *strp;

  PROTECT(retval = allocVector(STRSXP, length(obj)));
  for (i = 0; i < length(obj); i++) {
    x = REAL(obj)[i];
    if (x == R_PosInf) {
      SET_STRING_ELT(retval, i, mkChar(".inf"));
    }
    else if (x == R_NegInf) {
      SET_STRING_ELT(retval, i, mkChar("-.inf"));
    }
    else if (R_IsNA(x)) {
      SET_STRING_ELT(retval, i, mkChar(".na.real"));
    }
    else if (R_IsNaN(x)) {
      SET_STRING_ELT(retval, i, mkChar(".nan"));
    }
    else {
      e = log10(x);
      if (e < -4 || e >= precision) {
        format[3] = 'e';
      }
      n = snprintf(str, REAL_BUF_SIZE, format, precision, x);
      if (n >= REAL_BUF_SIZE) {
        warning("string representation of numeric was truncated because it was more than %d characters", REAL_BUF_SIZE);
      }
      else if (n < 0) {
        error("couldn't format numeric value");
      }
      else {
        /* tweak the string a little */
        strp = str + n; /* end of the string */
        j = n - 1;
        if (format[3] == 'e') {
          /* find 'e' first */
          for (k = 0; j >= 0; j--, k++) {
            if (str[j] == 'e') {
              break;
            }
          }
          if (k == 4 && str[j+2] == '0') {
            /* windows sprintf likes to add an extra 0 to the exp part */
            /* ex: 1.000e+007 */
            str[j+2] = str[j+3];
            str[j+3] = str[j+4];
            str[j+4] = str[j+5]; /* null */
            n -= 1;
          }
          strp = str + j;
          j -= 1;
        }
        suffix_len = n - j;

        /* remove trailing zeros */
        for (k = 0; j >= 0; j--, k++) {
          if (str[j] != '0' || str[j-1] == '.') {
            break;
          }
        }
        if (k > 0) {
          memmove(str + j + 1, strp, suffix_len);
        }
      }

      SET_STRING_ELT(retval, i, mkCharCE(str, CE_UTF8));
    }
  }
  UNPROTECT(1);
  return retval;
}

/* Format a vector of ints for emitting. Handle NAs. */
static SEXP
R_format_int(obj)
  SEXP obj;
{
  SEXP retval;
  int i;

  PROTECT(retval = coerceVector(obj, STRSXP));
  for (i = 0; i < length(obj); i++) {
    if (INTEGER(obj)[i] == NA_INTEGER) {
      SET_STRING_ELT(retval, i, mkChar(".na.integer"));
    }
  }
  UNPROTECT(1);

  return retval;
}

/* Format a vector of logicals for emitting. Handle NAs. */
static SEXP
R_format_logical(obj)
  SEXP obj;
{
  SEXP retval;
  int i, val;

  PROTECT(retval = allocVector(STRSXP, length(obj)));
  for (i = 0; i < length(obj); i++) {
    val = LOGICAL(obj)[i];
    if (val == NA_LOGICAL) {
      SET_STRING_ELT(retval, i, mkChar(".na"));
    }
    else if (val == 0) {
      SET_STRING_ELT(retval, i, mkChar("no"));
    }
    else {
      SET_STRING_ELT(retval, i, mkChar("yes"));
    }
  }
  UNPROTECT(1);

  return retval;
}

/* Format a vector of strings for emitting. Handle NAs. */
static SEXP
R_format_string(obj)
  SEXP obj;
{
  SEXP retval;
  int i;

  PROTECT(retval = duplicate(obj));
  for (i = 0; i < length(obj); i++) {
    if (STRING_ELT(obj, i) == NA_STRING) {
      SET_STRING_ELT(retval, i, mkCharCE(".na.character", CE_UTF8));
    }
  }
  UNPROTECT(1);

  return retval;
}

/* Take a CHARSXP, return a scalar style (for emitting) */
static yaml_scalar_style_t
R_string_style(obj)
  SEXP obj;
{
  char *tag;
  const char *chr = CHAR(obj);
  int len = length(obj), j;

  tag = find_implicit_tag(chr, len);
  if (strcmp((char *) tag, "str#na") == 0) {
    return YAML_ANY_SCALAR_STYLE;
  }

  if (strcmp((char *) tag, "str") != 0) {
    /* If this element has an implicit tag, it needs to be quoted */
    return YAML_SINGLE_QUOTED_SCALAR_STYLE;
  }

  /* Change to literal if there's a newline in this string */
  for (j = 0; j < len; j++) {
    if (chr[j] == '\n') {
      return YAML_LITERAL_SCALAR_STYLE;
    }
  }
  return YAML_ANY_SCALAR_STYLE;
}

/* Take a vector and an index and return another vector of size 1 */
static SEXP
R_yoink(vec, index)
  SEXP vec;
  int index;
{
  SEXP tmp, levels;
  int type, factor, level_idx;

  type = TYPEOF(vec);
  factor = type == INTSXP && R_has_class(vec, "factor");
  PROTECT(tmp = allocVector(factor ? STRSXP : type, 1));

  switch(type) {
    case LGLSXP:
      LOGICAL(tmp)[0] = LOGICAL(vec)[index];
      break;
    case INTSXP:
      if (factor) {
        levels = GET_LEVELS(vec);
        level_idx = INTEGER(vec)[index];
        if (level_idx == NA_INTEGER || level_idx < 1 || level_idx > LENGTH(levels)) {
          SET_STRING_ELT(tmp, 0, NA_STRING);
        }
        else {
          SET_STRING_ELT(tmp, 0, STRING_ELT(levels, level_idx - 1));
        }
      }
      else {
        INTEGER(tmp)[0] = INTEGER(vec)[index];
      }
      break;
    case REALSXP:
      REAL(tmp)[0] = REAL(vec)[index];
      break;
    case CPLXSXP:
      COMPLEX(tmp)[0] = COMPLEX(vec)[index];
      break;
    case STRSXP:
      SET_STRING_ELT(tmp, 0, STRING_ELT(vec, index));
      break;
    case RAWSXP:
      RAW(tmp)[0] = RAW(vec)[index];
      break;
  }
  UNPROTECT(1);

  return tmp;
}

static int
R_serialize_to_yaml_write_handler(data, buffer, size)
  void *data;
  unsigned char *buffer;
  size_t size;
{
  s_emitter_output *output = (s_emitter_output *)data;
  if (output->size + size > output->capa) {
    output->capa = (output->capa + size) * 2;
    output->buffer = (char *)realloc(output->buffer, output->capa * sizeof(char));

    if (output->buffer == NULL) {
      return 0;
    }
  }
  memmove((void *)(output->buffer + output->size), (void *)buffer, size);
  output->size += size;

  return 1;
}

static int
emit_char(emitter, event, obj, tag, implicit_tag, scalar_style)
  yaml_emitter_t *emitter;
  yaml_event_t *event;
  SEXP obj;
  char *tag;
  int implicit_tag;
  yaml_scalar_style_t scalar_style;
{
  yaml_scalar_event_initialize(event, NULL, (yaml_char_t *)tag,
      (yaml_char_t *)CHAR(obj), LENGTH(obj),
      implicit_tag, implicit_tag, scalar_style);

  if (!yaml_emitter_emit(emitter, event))
    return 0;

  return 1;
}

static int
emit_factor(emitter, event, obj)
  yaml_emitter_t *emitter;
  yaml_event_t *event;
  SEXP obj;
{
  SEXP levels, level_chr;
  yaml_scalar_style_t *scalar_styles, scalar_style;
  int i, len, level_idx, retval, *scalar_style_is_set;

  levels = GET_LEVELS(obj);
  len = length(levels);
  scalar_styles = (yaml_scalar_style_t *)malloc(sizeof(yaml_scalar_style_t) * len);
  scalar_style_is_set = (int *)calloc(len, sizeof(int));

  retval = 1;
  for (i = 0; i < length(obj); i++) {
    level_idx = INTEGER(obj)[i];
    if (level_idx == NA_INTEGER || level_idx < 1 || level_idx > len) {
      level_chr = mkChar(".na.character");
      scalar_style = YAML_ANY_SCALAR_STYLE;
    }
    else {
      level_chr = STRING_ELT(levels, level_idx - 1);
      if (!scalar_style_is_set[level_idx - 1]) {
        scalar_styles[level_idx - 1] = R_string_style(level_chr);
      }
      scalar_style = scalar_styles[level_idx - 1];
    }

    if (!emit_char(emitter, event, level_chr, NULL, 1, scalar_style)) {
      retval = 0;
      break;
    }
  }
  free(scalar_styles);
  free(scalar_style_is_set);
  return retval;
}

static int
emit_nil(emitter, event, obj)
  yaml_emitter_t *emitter;
  yaml_event_t *event;
  SEXP obj;
{
  yaml_scalar_event_initialize(event, NULL, NULL, (yaml_char_t *)"~", 1, 1, 1,
      YAML_ANY_SCALAR_STYLE);

  return yaml_emitter_emit(emitter, event);
}

static int
emit_object(emitter, event, obj, tag, omap, column_major, precision)
  yaml_emitter_t *emitter;
  yaml_event_t *event;
  SEXP obj;
  char *tag;
  int omap;
  int column_major;
  int precision;
{
  SEXP chr, names, thing, type, class, tmp;
  yaml_scalar_style_t scalar_style;
  int implicit_tag, rows, cols, i, j, result;

  /*Rprintf("=== Emitting ===\n");*/
  /*PrintValue(obj);*/

  scalar_style = YAML_ANY_SCALAR_STYLE;
  implicit_tag = 1;
  tag = NULL;
  switch (TYPEOF(obj)) {
    case NILSXP:
      return emit_nil(emitter, event, obj);

    case CLOSXP:
    case SPECIALSXP:
    case BUILTINSXP:
      /* Function! Deparse, then fall through */
      tag = "!expr";
      implicit_tag = 0;
      obj = R_deparse_function(obj);
      scalar_style = YAML_LITERAL_SCALAR_STYLE;

    /* atomic vector types */
    case LGLSXP:
    case REALSXP:
    case INTSXP:
    case STRSXP:
      /* FIXME: add complex and raw */
      if (length(obj) != 1) {
        yaml_sequence_start_event_initialize(event, NULL, NULL, 1, YAML_ANY_SEQUENCE_STYLE);
        if (!yaml_emitter_emit(emitter, event))
          return 0;
      }

      if (length(obj) >= 1) {
        if (R_has_class(obj, "factor")) {
          if (!emit_factor(emitter, event, obj))
            return 0;
        }
        else if (TYPEOF(obj) == STRSXP) {
          /* Might need to add quotes */
          PROTECT(obj = R_format_string(obj));

          result = 0;
          for (i = 0; i < length(obj); i++) {
            chr = STRING_ELT(obj, i);
            result = emit_char(emitter, event, chr, tag, implicit_tag,
                R_string_style(chr));

            if (!result)
              break;
          }
          UNPROTECT(1);

          if (!result)
            return 0;
        }
        else {
          switch(TYPEOF(obj)) {
            case REALSXP:
              obj = R_format_real(obj, precision);
              break;

            case INTSXP:
              obj = R_format_int(obj);
              break;

            case LGLSXP:
              obj = R_format_logical(obj);
              break;

            default:
              /* If you get here, you made a mistake. */
              return 0;
          }
          PROTECT(obj);

          result = 0;
          for (i = 0; i < length(obj); i++) {
            chr = STRING_ELT(obj, i);
            result = emit_char(emitter, event, chr, tag, implicit_tag,
                YAML_ANY_SCALAR_STYLE);

            if (!result)
              break;
          }
          UNPROTECT(1);

          if (!result)
            return 0;
        }
      }

      if (length(obj) != 1) {
        yaml_sequence_end_event_initialize(event);
        if (!yaml_emitter_emit(emitter, event))
          return 0;
      }
      break;

    case VECSXP:
      if (R_has_class(obj, "data.frame") && length(obj) > 0 && !column_major) {
        rows = length(VECTOR_ELT(obj, 0));
        cols = length(obj);
        names = GET_NAMES(obj);

        yaml_sequence_start_event_initialize(event, NULL, (yaml_char_t *)tag,
            implicit_tag, YAML_ANY_SEQUENCE_STYLE);
        if (!yaml_emitter_emit(emitter, event))
          return 0;

        for (i = 0; i < rows; i++) {
          yaml_mapping_start_event_initialize(event, NULL, (yaml_char_t *)tag,
              implicit_tag, YAML_ANY_MAPPING_STYLE);

          if (!yaml_emitter_emit(emitter, event))
            return 0;

          for (j = 0; j < cols; j++) {
            chr = STRING_ELT(names, j);
            if (!emit_char(emitter, event, chr, NULL, 1, R_string_style(chr)))
              return 0;

            /* Need to create a vector of size one, then emit it */
            thing = VECTOR_ELT(obj, j);
            PROTECT(tmp = R_yoink(thing, i));
            result = emit_object(emitter, event, tmp, NULL, omap, column_major, precision);
            UNPROTECT(1);

            if (!result)
              return 0;
          }

          yaml_mapping_end_event_initialize(event);
          if (!yaml_emitter_emit(emitter, event))
            return 0;
        }

        yaml_sequence_end_event_initialize(event);
        if (!yaml_emitter_emit(emitter, event))
          return 0;
      }
      else if (R_is_named_list(obj)) {
        if (omap) {
          yaml_sequence_start_event_initialize(event, NULL,
              (yaml_char_t *)"!omap", 0, YAML_ANY_SEQUENCE_STYLE);

          if (!yaml_emitter_emit(emitter, event))
            return 0;
        }
        else {
          yaml_mapping_start_event_initialize(event, NULL, (yaml_char_t *)tag,
              implicit_tag, YAML_ANY_MAPPING_STYLE);

          if (!yaml_emitter_emit(emitter, event))
            return 0;
        }

        names = GET_NAMES(obj);
        for (i = 0; i < length(obj); i++) {
          if (omap) {
            yaml_mapping_start_event_initialize(event, NULL, (yaml_char_t *)tag,
                implicit_tag, YAML_ANY_MAPPING_STYLE);

            if (!yaml_emitter_emit(emitter, event))
              return 0;
          }

          chr = STRING_ELT(names, i);
          if (!emit_char(emitter, event, chr, NULL, 1, R_string_style(chr)))
            return 0;

          if (!emit_object(emitter, event, VECTOR_ELT(obj, i), NULL, omap, column_major, precision))
            return 0;

          if (omap) {
            yaml_mapping_end_event_initialize(event);
            if (!yaml_emitter_emit(emitter, event))
              return 0;
          }
        }

        if (omap) {
          yaml_sequence_end_event_initialize(event);
          if (!yaml_emitter_emit(emitter, event))
            return 0;
        }
        else {
          yaml_mapping_end_event_initialize(event);
          if (!yaml_emitter_emit(emitter, event))
            return 0;
        }
      }
      else {
        yaml_sequence_start_event_initialize(event, NULL, (yaml_char_t *)tag,
            1, YAML_ANY_SEQUENCE_STYLE);
        if (!yaml_emitter_emit(emitter, event))
          return 0;

        for (i = 0; i < length(obj); i++) {
          if (!emit_object(emitter, event, VECTOR_ELT(obj, i), NULL, omap, column_major, precision))
            return 0;
        }
        yaml_sequence_end_event_initialize(event);
        if (!yaml_emitter_emit(emitter, event))
          return 0;
      }
      break;

    default:
      PROTECT(type = type2str(TYPEOF(obj)));
      class = GET_CLASS(obj);
      if (TYPEOF(class) != STRSXP || LENGTH(class) == 0) {
        warning("don't know how to emit object of type: '%s'\n", CHAR(type));
      }
      else {
        warning("don't know how to emit object of type: '%s', class: %s\n", CHAR(type), R_inspect(class));
      }
      UNPROTECT(1);
      return 0;
  }

  return 1;
}

SEXP
R_serialize_to_yaml(s_obj, s_line_sep, s_indent, s_omap, s_column_major, s_unicode, s_precision, s_indent_mapping_sequence)
  SEXP s_obj;
  SEXP s_line_sep;
  SEXP s_indent;
  SEXP s_omap;
  SEXP s_column_major;
  SEXP s_unicode;
  SEXP s_precision;
  SEXP s_indent_mapping_sequence;
{
  SEXP retval;
  yaml_emitter_t emitter;
  yaml_event_t event;
  s_emitter_output output;
  int status, line_sep, indent, omap, column_major, unicode, precision, indent_mapping_sequence;
  const char *c_line_sep;

  c_line_sep = CHAR(STRING_ELT(s_line_sep, 0));
  if (c_line_sep[0] == '\n') {
    line_sep = YAML_LN_BREAK;
  }
  else if (c_line_sep[0] == '\r') {
    if (c_line_sep[1] == '\n') {
      line_sep = YAML_CRLN_BREAK;
    }
    else {
      line_sep = YAML_CR_BREAK;
    }
  }
  else {
    error("argument `line.sep` must be a either '\n', '\r\n', or '\r'");
    return R_NilValue;
  }

  if (isNumeric(s_indent) && length(s_indent) == 1) {
    s_indent = coerceVector(s_indent, INTSXP);
    indent = INTEGER(s_indent)[0];
  }
  else if (isInteger(s_indent) && length(s_indent) == 1) {
    indent = INTEGER(s_indent)[0];
  }
  else {
    error("argument `indent` must be a numeric or integer vector of length 1");
    return R_NilValue;
  }

  if (indent <= 0) {
    error("argument `indent` must be greater than 0");
    return R_NilValue;
  }

  if (!isLogical(s_omap) || length(s_omap) != 1) {
    error("argument `omap` must be either TRUE or FALSE");
    return R_NilValue;
  }
  omap = LOGICAL(s_omap)[0];

  if (!isLogical(s_column_major) || length(s_column_major) != 1) {
    error("argument `column.major` must be either TRUE or FALSE");
    return R_NilValue;
  }
  column_major = LOGICAL(s_column_major)[0];

  if (!isLogical(s_unicode) || length(s_unicode) != 1) {
    error("argument `unicode` must be either TRUE or FALSE");
    return R_NilValue;
  }
  unicode = LOGICAL(s_unicode)[0];

  if (isNumeric(s_precision) && length(s_precision) == 1) {
    s_precision = coerceVector(s_precision, INTSXP);
    precision = INTEGER(s_precision)[0];
  }
  else if (isInteger(s_precision) && length(s_precision) == 1) {
    precision = INTEGER(s_precision)[0];
  }
  else {
    error("argument `precision` must be a numeric or integer vector of length 1");
    return R_NilValue;
  }
  if (precision < 1 || precision > 22) {
    error("argument `precision` must be in the range 1..22");
  }

  if (!isLogical(s_indent_mapping_sequence) || length(s_indent_mapping_sequence) != 1) {
    error("argument `indent.mapping.sequence` must be either TRUE or FALSE");
    return R_NilValue;
  }
  indent_mapping_sequence = LOGICAL(s_indent_mapping_sequence)[0];

  yaml_emitter_initialize(&emitter);
  yaml_emitter_set_unicode(&emitter, unicode);
  yaml_emitter_set_break(&emitter, line_sep);
  yaml_emitter_set_indent(&emitter, indent);
  yaml_emitter_set_indent_mapping_sequence(&emitter, indent_mapping_sequence);

  output.buffer = NULL;
  output.size = output.capa = 0;
  yaml_emitter_set_output(&emitter, R_serialize_to_yaml_write_handler, &output);

  yaml_stream_start_event_initialize(&event, YAML_ANY_ENCODING);
  status = yaml_emitter_emit(&emitter, &event);
  if (!status)
    goto done;

  yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1);
  status = yaml_emitter_emit(&emitter, &event);
  if (!status)
    goto done;

  status = emit_object(&emitter, &event, s_obj, NULL, omap, column_major, precision);
  if (!status)
    goto done;

  yaml_document_end_event_initialize(&event, 1);
  status = yaml_emitter_emit(&emitter, &event);
  if (!status)
    goto done;

  yaml_stream_end_event_initialize(&event);
  status = yaml_emitter_emit(&emitter, &event);

done:

  if (status) {
    PROTECT(retval = allocVector(STRSXP, 1));
    SET_STRING_ELT(retval, 0, mkCharLen(output.buffer, output.size));
    UNPROTECT(1);
  }
  else {
    if (emitter.problem != NULL) {
      set_error_msg("Emitter error: %s", emitter.problem);
    }
    else {
      set_error_msg("Unknown emitter error");
    }
    retval = R_NilValue;
  }

  yaml_emitter_delete(&emitter);

  if (status) {
    free(output.buffer);
  }
  else {
    error(error_msg);
  }

  return retval;
}
