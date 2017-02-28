`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL, error.label = NULL) {
  .Call("unserialize_from_yaml", enc2utf8(string), as.named.list, handlers, error.label, PACKAGE="yaml")
}
