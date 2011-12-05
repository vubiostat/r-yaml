failure = FALSE
for (i in ls(pattern = "^test")) {
  cat("running ", i, "...\n");
  func <- get(i)
  res <- try(func(), TRUE)
  if (inherits(res, "try-error")) {
    cat("assertion failed in function <", i, ">\n", sep="")
    cat("\t", res)
    failure = TRUE
  }
}
if (failure) {
  stop("errors occurred")
}
