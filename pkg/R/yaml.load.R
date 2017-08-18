`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL, error.label = NULL) {
  string <- enc2utf8(paste(string, collapse = "\n"))
  .Call("unserialize_from_yaml", string, as.named.list, handlers, error.label, PACKAGE="yaml")
}
