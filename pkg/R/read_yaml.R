`read_yaml` <-
function(file, fileEncoding = "UTF-8", text, ...) {
  if (missing(file) && !missing(text)) {
    file <- textConnection(text, encoding = "UTF-8")
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
  string <- paste(readLines(file), collapse="\n")
  yaml.load(string, ...)
}
