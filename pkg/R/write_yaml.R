`write_yaml` <-
function(x, file, line.sep = c('\n', '\r\n', '\r'), indent = 2, omap = FALSE, column.major = TRUE, unicode = TRUE, precision = getOption('digits'), indent.mapping.sequence = FALSE, fileEncoding = "UTF-8") {
  if (is.character(file)) {
    file <-
      if (nzchar(fileEncoding)) {
        file(file, "w", encoding = fileEncoding)
      } else {
        file(file, "w")
      }
    on.exit(close(file))
  }
  else if (!isOpen(file, "w")) {
    open(file, "w")
    on.exit(close(file))
  }
  if (!inherits(file, "connection")) {
    stop("'file' must be a character string or connection")
  }

  .Call("as.yaml", x, file, line.sep, indent, omap, column.major, unicode, precision, indent.mapping.sequence, PACKAGE="yaml")
}
