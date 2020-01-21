`write_yaml` <-
function(x, file, fileEncoding = "UTF-8", delimiter = FALSE,...) {
  result <- as.yaml(x, ...)

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
  if (delimiter) cat ("---", file=file, sep="\n")
  cat(result, file=file, sep="")
  if (delimiter) cat ("---", file=file, sep="\n")
}
