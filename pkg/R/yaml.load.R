`yaml.load` <-
function(string, as.named.list = TRUE, handlers = NULL) {
  conn <- rawConnection(charToRaw(enc2utf8(string)))
  on.exit(close(conn))
  .Call("yaml.load", conn, as.named.list, handlers, PACKAGE="yaml")
}
