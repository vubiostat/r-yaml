`yaml.load_file` <-
function(input, error.label, ...) {
  if (missing(error.label)) {
    if (inherits(input, "connection")) {
      # try to guess filename
      s <- try(summary(input), silent = TRUE)
      if (!inherits(s, "try-error") && is.list(s) && "description" %in% names(s)) {
        error.label <- s$description
      }
    }
    else if (is.character(input) && nzchar(input[1])) {
      error.label <- input[1]
    }
  }
  
  con <- file(input, encoding = 'UTF-8');
  on.exit(close(con));
  yaml.load(paste(readLines(con), collapse="\n"),
            error.label = error.label, ...)
}
