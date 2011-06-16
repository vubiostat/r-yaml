\name{as.yaml}
\alias{as.yaml}
\alias{as.yaml.list}
\alias{as.yaml.data.frame}
\alias{as.yaml.default}
\title{ Convert an R object into a YAML string }
\description{
  Generic functions to convert R objects into YAML strings
}
\usage{
  \method{as.yaml}{list}(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, omap = FALSE, ...)
  \method{as.yaml}{data.frame}(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, column.major = TRUE, ...)
  \method{as.yaml}{default}(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, ...)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{x}{ the object to be converted }
  \item{line.sep}{ the line separator character(s) to use }
  \item{indent}{ the number of spaces to use for indenting }
  \item{pre.indent}{ number of indentations to put before each line; see Details }
  \item{omap}{ determines whether or not to convert a list to a YAML omap; see Details }
  \item{column.major}{ determines how to convert a data.frame; see Details }
  \item{\dots}{ other options to pass to the generic functions }
}
\details{
  The \code{pre.indent} option specifies how many times to indent before each line, rather
  than how many spaces you want before each line.  For example, if you want 3 spaces before
  each line, you would say: \code{as.yaml(1:10, indent = 3, pre.indent = 1)}.

  If you set the \code{omap} option to TRUE, as.yaml will tag the YAML document as an omap,
  which is an ordered map.  This will maintain the order of elements in the list.

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
\author{ Jeremy Stephens <jeremy.stephens@vanderbilt.edu }
\seealso{ \code{\link{yaml.load}} }
\examples{
  as.yaml(1:10)
  as.yaml(list(foo=1:10, bar=c("test1", "test2")))
  as.yaml(data.frame(a=1:10, b=letters[1:10], c=11:20))
  as.yaml(list(a=1:2, b=3:4), omap=TRUE)
  as.yaml("multi\nline\nstring")
  as.yaml(function(x) x + 1)
  as.yaml(list(foo=list(list(x = 1, y = 2), list(x = 3, y = 4))))
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ data }
\keyword{ manip }
