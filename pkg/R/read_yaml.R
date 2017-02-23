`read_yaml` <-
function(file, as.named.list = TRUE, handlers = NULL, fileEncoding = "UTF-8", text) {
  if (missing(file) && !missing(text)) {
    file <- rawConnection(charToRaw(enc2utf8(text)))
    on.exit(close(file))
  }
  if (is.character(file)) {
    file <- if (nzchar(fileEncoding))
      file(file, "rt", encoding = fileEncoding)
    else file(file, "rt")
    on.exit(close(file))
  }
  if (!inherits(file, "connection"))
    stop("'file' must be a character string or connection")
  if (!isOpen(file, "rt")) {
    open(file, "rt")
    on.exit(close(file))
  }
  .Call("yaml.load", file, as.named.list, handlers, PACKAGE="yaml")
}
