\name{verbatim_logical}
\alias{verbatim_logical}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
Alternative logical handler
}
\description{
A yaml handler function that causes logical vectors to emit 
\code{true}/\code{false} instead of \code{yes}/\code{no} values.
}
\usage{
verbatim_logical(x)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{x}{
	logical vector to convert to \code{true}/\code{false}.
}
}
\details{
Pass this function to \code{\link[=as.yaml]{as.yaml()}} as part of the
\code{handler} argument list like \code{list(logical = verbatim_logical)}.
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
