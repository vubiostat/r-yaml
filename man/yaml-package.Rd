\name{yaml-package}
\alias{yaml-package}
\alias{yaml}
\docType{package}
\title{
  Methods to convert R to YAML and back
}
\description{
  This package implements the libyaml YAML parser and emitter for R.
}
\details{
\tabular{ll}{
Package: \tab yaml\cr
Type: \tab Package\cr
Version: \tab 2.1.0\cr
Date: \tab 2011-12-02\cr
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

  libyaml: http://pyyaml.org/wiki/LibYAML

  More information: http://biostat.mc.vanderbilt.edu/wiki/Main/YamlR
}
\keyword{ package }
\keyword{ programming }
\keyword{ data }
\keyword{ manip }
\examples{
  x <- data.frame(a=sample(1:100, 10), b=sample(letters, 10))
  as.yaml(x)
  yaml.load("foo: 123\nbar:\n  - 3.14\n  - 2.71")
\dontrun{
  yaml.load_file("foo.yml")
}
}
