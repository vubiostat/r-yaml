\name{write_yaml}
\alias{write_yaml}
\title{Write a YAML file}
\description{
  Write the YAML representation of an R object to a file
}
\usage{
  write_yaml(x, file, fileEncoding = "UTF-8", \dots)
}
\arguments{
  \item{x}{the object to be converted}
  \item{file}{either a character string naming a file or a \link{connection}
    open for writing}
  \item{fileEncoding}{character string: if non-empty declares the
    encoding to be used on a file (not a connection) so the character data can
    be re-encoded as they are written.  See \code{\link{file}}.}
  \item{\dots}{arguments to \code{\link{as.yaml}}}
}
\details{
  If \code{file} is a non-open connection, an attempt is made to open it
  and then close it after use.

  This function is a convenient wrapper around \code{\link{as.yaml}}.
}
\author{Jeremy Stephens <jeremy.stephens@vanderbilt.edu>}
\seealso{\code{\link{as.yaml}}, \code{\link{read_yaml}}, \code{\link{yaml.load_file}}}
\examples{
\dontrun{
  # writing to a file connection
  filename <- tempfile()
  con <- file(filename, "w")
  write_yaml(data.frame(a=1:10, b=letters[1:10], c=11:20), con)
  close(con)

  # using a filename to specify output file
  write_yaml(data.frame(a=1:10, b=letters[1:10], c=11:20), filename)
}
}
\keyword{data}
\keyword{manip}
