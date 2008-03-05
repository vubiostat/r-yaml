`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL) {
  .Call("yaml.load", string, as.named.list, handlers, PACKAGE="yaml")
}
