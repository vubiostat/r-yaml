\name{yaml-package}
\alias{yaml-package}
\alias{yaml}
\docType{package}
\title{
  Methods to convert R to YAML and back
}
\description{
  This package implements the Syck YAML parser for R.  There are also methods to convert
  R objects into YAML.
}
\details{
\tabular{ll}{
Package: \tab yaml\cr
Type: \tab Package\cr
Version: \tab 1.1.0\cr
Date: \tab 2009-09-11\cr
License: \tab BSD\cr
}
Use \code{\link{as.yaml}} to convert R objects into YAML and \code{\link{yaml.load}} or
\code{\link{yaml.load_file}} to parse YAML into R objects.
}
\author{
Jeremy Stephens <jeremy.stephens@vanderbilt.edu>
}
\references{
  YAML: http://yaml.org

  Syck YAML Parser: (formerly) http://whytheluckystiff.net/syck/
}
\keyword{ package }
\keyword{ programming }
\keyword{ data }
\keyword{ manip }
\examples{
  x <- data.frame(a=sample(1:100, 10), b=sample(letters, 10))
  as.yaml(x)
  yaml.load("foo: bar\njunk:\n  - stuff\n  - blah")
\dontrun{
  yaml.load_file("foo.yml")
}
}
