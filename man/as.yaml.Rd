\name{as.yaml}
\alias{as.yaml}
\title{ Convert an R object into a YAML string }
\description{
  Convert an R object into a YAML string
}
\usage{
  as.yaml(x, line.sep = c("\n", "\r\n", "\r"), indent = 2, omap = FALSE,
          column.major = TRUE, unicode = TRUE, precision = getOption('digits'),
          indent.mapping.sequence = FALSE)
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
}
\value{
  Returns a YAML string which can be loaded using \code{\link{yaml.load}} or copied into
  a file for external use.
}
\references{
  YAML: http://yaml.org

  YAML omap type: http://yaml.org/type/omap.html
}
\author{ Jeremy Stephens <jeremy.stephens@vanderbilt.edu> }
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
}
\keyword{ data }
\keyword{ manip }
