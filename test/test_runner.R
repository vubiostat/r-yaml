for (i in ls(pattern = "^test")) {
  res <- try(eval(call(i)), TRUE)
  if (inherits(res, "try-error")) {
    cat("assertion failed in function <", i, ">\n", sep="")
    cat("\t", res)
  }
}
