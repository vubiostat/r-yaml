`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL, error.label = NULL, eval.expr = getOption("yaml.eval.expr", TRUE)) {
  eval.warning <- missing(eval.expr) && is.null(getOption("yaml.eval.expr"))
  string <- enc2utf8(paste(string, collapse = "\n"))
  .Call(C_unserialize_from_yaml, string, as.named.list, handlers, error.label, eval.expr, eval.warning, PACKAGE="yaml")
}
