`as.yaml` <-
function(x, line.sep = c('\n', '\r\n', '\r'), indent = 2, omap = FALSE, column.major = TRUE, unicode = TRUE, precision = getOption('digits')) {
  line.sep <- match.arg(line.sep)
  .Call("as.yaml", x, line.sep, indent, omap, column.major, unicode, precision, PACKAGE="yaml")
}
