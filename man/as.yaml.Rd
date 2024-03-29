\name{as.yaml}
\alias{as.yaml}
\title{ Convert an R object into a YAML string }
\description{
  Convert an R object into a YAML string
}
\usage{
  as.yaml(x, line.sep = c("\n", "\r\n", "\r"), indent = 2, omap = FALSE,
          column.major = TRUE, unicode = TRUE, precision = getOption('digits'),
          indent.mapping.sequence = FALSE, handlers = NULL)
}
\arguments{
  \item{x}{ the object to be converted }
  \item{line.sep}{ the line separator character(s) to use }
  \item{indent}{ the number of spaces to use for indenting }
  \item{omap}{ determines whether or not to convert a list to a YAML omap; see Details }
  \item{column.major}{ determines how to convert a data.frame; see Details }
  \item{unicode}{ determines whether or not to allow unescaped unicode characters in output }
  \item{precision}{ number of significant digits to use when formatting numeric values }
  \item{indent.mapping.sequence}{ determines whether or not to indent sequences in mapping context }
  \item{handlers}{ named list of custom handler functions for R objects; see Details }
}
\details{
  If you set the \code{omap} option to TRUE, as.yaml will create ordered maps
  (or omaps) instead of normal maps.

  The \code{column.major} option determines how a data frame is converted. If TRUE, the data
  frame is converted into a map of sequences where the name of each column is a key. If FALSE,
  the data frame is converted into a sequence of maps, where each element in the sequence is a
  row.  You'll probably almost always want to leave this as TRUE (which is the default),
  because using \code{\link{yaml.load}} on the resulting string returns an object which is
  much more easily converted into a data frame via \code{\link{as.data.frame}}.

  You can specify custom handler functions via the \code{handlers} argument.
  This argument must be a named list of functions, where the names are R object
  class names (i.e., 'numeric', 'data.frame', 'list', etc).  The function(s)
  you provide will be passed one argument (the R object) and can return any R
  object.  The returned object will be emitted normally.

  Character vectors that have a class of \sQuote{verbatim} will not be quoted
  in the output YAML document except when the YAML specification requires it.
  This means that you cannot do anything that would result in an invalid YAML
  document, but you can emit strings that would otherwise be quoted.  This is
  useful for changing how logical vectors are emitted (see below for example).

  Character vectors that have an attribute of \sQuote{quoted} will be wrapped
  in double quotes (see below for example).

  You can specify YAML tags for R objects by setting the \sQuote{tag} attribute
  to a character vector of length 1.  If you set a tag for a vector, the tag
  will be applied to the YAML sequence as a whole, unless the vector has only 1
  element.  If you wish to tag individual elements, you must use a list of
  1-length vectors, each with a tag attribute.  Likewise, if you set a tag for
  an object that would be emitted as a YAML mapping (like a data frame or a
  named list), it will be applied to the mapping as a whole.  Tags can be used
  in conjunction with YAML deserialization functions like
  \code{\link{yaml.load}} via custom handlers, however, if you set an internal
  tag on an incompatible data type (like \dQuote{!seq 1.0}), errors will occur
  when you try to deserialize the document.
}
\value{
  Returns a YAML string which can be loaded using \code{\link{yaml.load}} or copied into
  a file for external use.
}
\references{
  YAML: http://yaml.org

  YAML omap type: http://yaml.org/type/omap.html
}
\author{ Jeremy Stephens <jeremy.f.stephens@vumc.org> }
\seealso{ \code{\link{yaml.load}} }
\examples{
  as.yaml(1:10)
  as.yaml(list(foo=1:10, bar=c("test1", "test2")))
  as.yaml(list(foo=1:10, bar=c("test1", "test2")), indent=3)
  as.yaml(list(foo=1:10, bar=c("test1", "test2")), indent.mapping.sequence=TRUE)
  as.yaml(data.frame(a=1:10, b=letters[1:10], c=11:20))
  as.yaml(list(a=1:2, b=3:4), omap=TRUE)
  as.yaml("multi\nline\nstring")
  as.yaml(function(x) x + 1)
  as.yaml(list(foo=list(list(x = 1, y = 2), list(x = 3, y = 4))))

  # custom handler
  as.yaml(Sys.time(), handlers = list(
    POSIXct = function(x) format(x, "\%Y-\%m-\%d")
  ))

  # custom handler with verbatim output to change how logical vectors are
  # emitted
  as.yaml(c(TRUE, FALSE), handlers = list(
    logical = verbatim_logical))

  # force quotes around a string
  port_def <- "80:80"
  attr(port_def, "quoted") <- TRUE
  x <- list(ports = list(port_def))
  as.yaml(x)

  # custom tag for scalar
  x <- "thing"
  attr(x, "tag") <- "!thing"
  as.yaml(x)

  # custom tag for sequence
  x <- 1:10
  attr(x, "tag") <- "!thing"
  as.yaml(x)

  # custom tag for mapping
  x <- data.frame(a = letters[1:5], b = letters[6:10])
  attr(x, "tag") <- "!thing"
  as.yaml(x)

  # custom tag for each element in a list
  x <- list(1, 2, 3)
  attr(x[[1]], "tag") <- "!a"
  attr(x[[2]], "tag") <- "!b"
  attr(x[[3]], "tag") <- "!c"
  as.yaml(x)
}
\keyword{ data }
\keyword{ manip }
