`yaml.load_file` <-
function(file, as.named.list = TRUE, handlers = NULL) {
  if (is.character(file)) {
    file <- file(file, "r")
    on.exit(close(file))
  }
  .Call("yaml.load", file, as.named.list, handlers, PACKAGE="yaml")
}
