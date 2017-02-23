`as.yaml` <-
function(x, line.sep = c('\n', '\r\n', '\r'), indent = 2, omap = FALSE, column.major = TRUE, unicode = TRUE, precision = getOption('digits'), indent.mapping.sequence = FALSE) {
  line.sep <- match.arg(line.sep)
  conn <- rawConnection(raw(0), open="w")
  on.exit(close(conn))
  .Call("as.yaml", x, conn, line.sep, indent, omap, column.major, unicode, precision, indent.mapping.sequence, PACKAGE="yaml")
  rawToChar(rawConnectionValue(conn))
}
