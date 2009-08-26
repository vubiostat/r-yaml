\name{yaml.load}
\alias{yaml.load}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{ Convert a YAML string into R objects }
\description{
  This function implements the Syck YAML parser for R.  It parses a YAML string
  into R objects.
}
\usage{
yaml.load(input, as.named.list = TRUE, handlers = NULL)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{input}{ the YAML string to be parsed; if not a string, it will be treated as a connection }
  \item{as.named.list}{ whether or not to return a named list for maps (TRUE by default) }
  \item{handlers}{ named list of custom handler functions for YAML types (see Details). }
}
\details{
  Sequences of uniform data (i.e. a sequence of integers) are converted into vectors.  If
  the sequence is not uniform, it's returned as a list. Maps are converted into named lists
  by default, and all the keys in the map are converted to strings.  If you don't want the
  keys to be coerced into strings, set \code{as.named.list} to FALSE.  When it's FALSE, a
  list will be returned with an additional attribute named 'keys', which is a list of the
  un-coerced keys in the map (in the same order as the main list).

  You can specify custom handler functions via the \code{handlers} argument.  This argument
  must be a named list of functions, where the names are the YAML types (i.e., 'int', 'float',
  'timestamp\#iso8601', etc).  The functions you provide will be passed one argument.  Custom
  handler functions for string types (all types except sequence and map) will receive a
  character vector of length 1.  Custom sequence functions will be passed a list of objects.
  Custom map functions will be passed the object that the internal map handler creates, which
  is either a named list or a list with a 'keys' attribute (depending on \code{as.named.list}).
  ALL functions you provide must return an object.  See the examples for custom handler use.
}
\value{
  If the root YAML object is a map, a named list or list with an attribute of 'keys' is
  returned.  If the root object is a sequence, a list or vector is returned, depending
  on the contents of the sequence.  A vector of length 1 is returned for single objects.
}
\references{
  YAML: http://yaml.org
  Syck YAML Parser: (formerly) http://whytheluckystiff.net/syck/
}
\author{ Jeremy Stephens <jeremy.stephens@vanderbilt.edu }
\seealso{ \code{\link{as.yaml}} }
\examples{
  yaml.load("- hey\n- hi\n- hello")
  yaml.load("foo: 123\nbar: 456")
  yaml.load("- foo\n- bar\n- 3.14")
  yaml.load("foo: bar\n123: 456", as.named.list = FALSE)

  # reading from a file (uses readLines internally)
  cat("foo: 123", file="foo.yml", sep="\n")
  foo <- file("foo.yml", "r")
  yaml.load(foo)
  close(foo)
  unlink("foo.yml") # tidy up

  # custom string-type handler
  my.float.handler <- function(x) { as.numeric(x) + 123 }
  yaml.load("123.456", handlers=list("float#fix"=my.float.handler))

  # custom sequence handler
  yaml.load("- 1\n- 2\n- 3", handlers=list(seq=function(x) { as.integer(x) + 3 }))

  # custom map handler
  yaml.load("foo: 123", handlers=list(map=function(x) { x$foo <- x$foo + 123; x }))

  # handling custom types
  yaml.load("!sqrt 555", handlers=list(sqrt=function(x) { sqrt(as.integer(x)) }))
  yaml.load("!foo\n- 1\n- 2", handlers=list(foo=function(x) { as.integer(x) + 1 }))
  yaml.load("!bar\none: 1\ntwo: 2", handlers=list(foo=function(x) { x$one <- "one"; x }))
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ programming }
\keyword{ data }
\keyword{ manip }
