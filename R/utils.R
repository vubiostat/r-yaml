`as.utf8` <-
function(x) {
  if (is.character(x)) {
    coded <- enc2utf8(x)
    attributes(coded) <- attributes(x)
    coded
  } else if (is.list(x)) {
    coded <- lapply(x, as.utf8)
    attributes(coded) <- attributes(x)
    coded
  } else {
    x
  }
}