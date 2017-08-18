`as.yaml` <-
function(x, line.sep = c('\n', '\r\n', '\r'), indent = 2, omap = FALSE, column.major = TRUE, unicode = TRUE, precision = getOption('digits'), indent.mapping.sequence = FALSE) {
  line.sep <- match.arg(line.sep)
  res <- .Call("serialize_to_yaml", x, line.sep, indent, omap, column.major, unicode, precision, indent.mapping.sequence, PACKAGE="yaml")
  Encoding(res) <- "UTF-8"
  res
}
