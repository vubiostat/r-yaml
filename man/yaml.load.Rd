\name{yaml.load}
\alias{yaml.load}
\alias{yaml.load_file}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{ Convert a YAML string into R objects }
\description{
  Parse a YAML string and return R objects.
}
\usage{
  yaml.load(string, as.named.list = TRUE, handlers = NULL, error.label = NULL,
            eval.expr = getOption("yaml.eval.expr", TRUE),
            merge.precedence = c("order", "override"), merge.warning = FALSE)
  yaml.load_file(input, error.label, ...)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{string}{ the YAML string to be parsed }
  \item{as.named.list}{ whether or not to return a named list for maps (TRUE by default) }
  \item{handlers}{ named list of custom handler functions for YAML types (see Details) }
  \item{input}{ a filename or connection; if \code{input} is a filename, that file must be encoded in UTF-8 }
  \item{error.label}{ a label to prepend to error messages (see Details) }
  \item{eval.expr}{ whether or not to evaluate expressions found in the YAML document (see Details) }
  \item{merge.precedence}{ behavior of precedence during map merges (see Details) }
  \item{merge.warning}{ whether or not to warn about ignored key/value pairs during map merges }
  \item{...}{ arguments to pass to yaml.load }
}
\details{
  Use \code{yaml.load} to load a YAML string.  For files and connections, use
  \code{yaml.load_file}, which calls \code{yaml.load} with the contents of the specified
  file or connection.

  Sequences of uniform data (e.g. a sequence of integers) are converted into vectors.  If
  the sequence is not uniform, it's returned as a list. Maps are converted into named lists
  by default, and all the keys in the map are converted to strings.  If you don't want the
  keys to be coerced into strings, set \code{as.named.list} to FALSE.  When it's FALSE, a
  list will be returned with an additional attribute named 'keys', which is a list of the
  un-coerced keys in the map (in the same order as the main list).

  You can specify custom handler functions via the \code{handlers} argument.  This argument
  must be a named list of functions, where the names are the YAML types (i.e., 'int', 'float',
  'seq', etc).  The functions you provide will be passed one argument.  Custom
  handler functions for string types (all types except sequence and map) will receive a
  character vector of length 1.  Custom sequence functions will be passed a list of objects.
  Custom map functions will be passed the object that the internal map handler creates, which
  is either a named list or a list with a 'keys' attribute (depending on \code{as.named.list}).
  ALL functions you provide must return an object.  See the examples for custom handler use.

  You can specify a label to be prepended to error messages via the
  \code{error.label} argument.  When using \code{yaml.load_file}, you can
  either set the \code{error.label} argument explicitly or leave it missing.
  If missing, \code{yaml.load_file} will make an educated guess for the value
  of \code{error.label} by either using the specified filename (when
  \code{input} is a character vector) or using the description of the supplied
  connection object (via the \code{summary} function).  You can explicity set
  \code{error.label} to \code{NULL} if you don't want to use this functionality.

  There is a built-in handler that will evaluate expressions that are tagged
  with the \sQuote{!expr} tag.  Currently this handler is disabled by default
  for security reasons.  If a \sQuote{!expr} tag exists and this is set to 
  FALSE a warning will occur. Alternately, you can set the option named
  \sQuote{yaml.eval.expr} via the \code{options} function to turn on 
  evaluation.

  The \code{merge.precedence} parameter controls how merge keys are handled.
  The YAML merge key specification is not specific about how key/value
  conflicts are resolved during map merges.  As a result, various YAML library
  implementations vary in merge key behavior (notably Python and Ruby).  This
  package's default behavior (when \code{merge.precedence} is \sQuote{order})
  is to give precedence to key/value pairs that appear first.  If you set
  \code{merge.precedence} to \sQuote{override}, natural map key/value pairs
  will override any duplicate keys found in merged maps, regardless of order.
  This is the default behavior in Python's YAML library.

  This function uses the YAML parser provided by libyaml, which conforms to the YAML 1.1
  specification.
}
\value{
  If the root YAML object is a map, a named list or list with an attribute of 'keys' is
  returned.  If the root object is a sequence, a list or vector is returned, depending
  on the contents of the sequence.  A vector of length 1 is returned for single objects.
}
\references{
  YAML: http://yaml.org

  libyaml: https://pyyaml.org/wiki/LibYAML

  YAML merge specification: http://yaml.org/type/merge.html
}
\author{ Jeremy Stephens <jeremy.f.stephens@vumc.org> }
\seealso{ \code{\link{as.yaml}} }
\examples{
  yaml.load("- hey\n- hi\n- hello")
  yaml.load("foo: 123\nbar: 456")
  yaml.load("- foo\n- bar\n- 3.14")
  yaml.load("foo: bar\n123: 456", as.named.list = FALSE)

\dontrun{
  # reading from a file (uses readLines internally)
  filename <- tempfile()
  cat("foo: 123", file=filename, sep="\n")
  yaml.load_file(filename)
}

  # custom scalar handler
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

  # loading R expressions
  # NOTE: this will not be done by default in the near future
  doc <- yaml.load("inc: !expr function(x) x + 1")
  doc$inc(1)

  # adding a label to error messages
  try(yaml.load("*", error.label = "foo"))
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ programming }
\keyword{ data }
\keyword{ manip }
