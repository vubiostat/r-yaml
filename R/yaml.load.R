`yaml.load` <-
function(string, as.named.list = TRUE) {
  .Call("yaml.load", string, as.named.list, PACKAGE="yaml")
}
