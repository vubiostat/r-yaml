`as.yaml` <-
function(x, omap = FALSE, column.major = TRUE, ...) {
  # FIXME: add line.sep, indent, and pre.indent
  .Call("as.yaml", x, omap, column.major, PACKAGE="yaml")
}
