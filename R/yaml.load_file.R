`yaml.load_file` <-
function(input,
         error.label,
         readLines.warn=TRUE,
         fileEncoding="UTF-8",
         ...)
{
  if (missing(error.label))
  {
    if (inherits(input, "connection"))
    {
      # try to guess filename
      s <- try(summary(input), silent = TRUE)
      if (!inherits(s, "try-error") && is.list(s) && "description" %in% names(s)) {
        error.label <- s$description
      }
    }
    else if (is.character(input) && nzchar(input[1]))
    {
      error.label <- input[1]
    }
    else
    {
      error.label <- NULL
    }
  }

  if (is.character(input))
  {
    con <- base::file(input, encoding=fileEncoding)
    on.exit(close(con), add = TRUE)
  } else {
    con <- input
  }
  
  if(fileEncoding != 'UTF-8')
  {
    yaml.load(enc2utf8(readLines(con, warn=readLines.warn)),
              error.label = error.label,
              ...)
  } else
  {
    yaml.load(readLines(con, warn=readLines.warn),
              error.label = error.label,
              ...)
  }
  
}
