`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL) {
  .Call("yaml.load", enc2utf8(string), as.named.list, handlers, PACKAGE="yaml")
}
