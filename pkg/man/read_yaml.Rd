\name{read_yaml}
\alias{read_yaml}
\title{Read a YAML file}
\description{
  Read a YAML document from a file and create an R object from it
}
\usage{
  read_yaml(file, fileEncoding = "UTF-8", text, \dots)
}
\arguments{
  \item{file}{either a character string naming a file or a \link{connection}
    open for writing}
  \item{fileEncoding}{character string: if non-empty declares the
    encoding used on a file (not a connection) so the character data can
    be re-encoded.  See \code{\link{file}}.}
  \item{text}{character string: if \code{file} is not supplied and this is,
    then data are read from the value of \code{text} via a text connection.
    Notice that a literal string can be used to include (small) data sets
    within R code.}
  \item{...}{arguments to pass to \code{\link{yaml.load}}}
}
\details{
  This function is a convenient wrapper for \code{\link{yaml.load}} and is a
  nicer alternative to \code{\link{yaml.load_file}}.
}
\value{
  If the root YAML object is a map, a named list or list with an attribute of 'keys' is
  returned.  If the root object is a sequence, a list or vector is returned, depending
  on the contents of the sequence.  A vector of length 1 is returned for single objects.
}
\references{
  YAML: http://yaml.org

  libyaml: http://pyyaml.org/wiki/LibYAML
}
\author{Jeremy Stephens <jeremy.stephens@vanderbilt.edu>}
\seealso{\code{\link{yaml.load}}, \code{\link{write_yaml}}, \code{\link{yaml.load_file}}}
\examples{
\dontrun{
  # reading from a file connection
  con <- file("data.yml", "w")
  read_yaml(con)

  # using a filename to specify input file
  read_yaml("data.yml")
}

  # reading from a character vector
  read_yaml(text="- hey\n- hi\n- hello")
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{programming}
\keyword{data}
\keyword{manip}
