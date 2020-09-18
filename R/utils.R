`as.utf8` <-
function(x) {
  
  if (is.character(x)) {
    enc2utf8(x)
  } else if (is.list(x)) {
    lapply(x, as.utf8)
  } else {
    x
  }
  
}