#dyn.load("../src/yaml.so")
#for (f in list.files("../R", full.names = TRUE))
#  source(f)
library(yaml)

assert <- function(bool) {
  if (!bool) stop(bool, " is not TRUE")
}
assert_equal <- function(expected, actual) {
  if (any(expected != actual)) stop("Expected <", expected, ">, got <", actual, ">")
}
assert_nan <- function(value) {
  if (!is.nan(value)) stop(" is not NaN")
}
