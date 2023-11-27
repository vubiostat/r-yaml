\name{verbatim_logical}
\alias{verbatim_logical}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Alternalte logical handler
}
\description{
A yaml handler function that will cause logical vectors to emit true/false value
instead of yes/no value
}
\usage{
verbatim_logical(x)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{x}{

}
}
\details{
To use pass into as.yaml as part of a handler arguement list like 
\code{list(logical=verbatim_logical)}.
}
\value{
Returns a vector of strings of either \code{true} or \code{false} of class
\code{verbatim}.
}
\author{
Charles Dupont and James Goldie (jimjam-slam)
}

%% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
\code{\link{as.yaml}}
}
\examples{
vector <- c(TRUE, FALSE, TRUE)

as.yaml(vector, handlers=list(logical=verbatim_logical))
}
\keyword{ data }
\keyword{ manip }
