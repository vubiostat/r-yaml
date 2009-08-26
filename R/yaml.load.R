`yaml.load` <-
function(input, as.named.list = TRUE, handlers = NULL) {
  if (is.character(input)) {
    string <- input
  }
  else {
    # try connection
    string <- paste(readLines(input), collapse="\n")
  }
  .Call("yaml.load", string, as.named.list, handlers, PACKAGE="yaml")
}
