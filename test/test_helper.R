dyn.load("../src/yaml.so")
for (f in list.files("../R", full.names = TRUE))
  source(f)

assert <- function(bool) {
  if (!bool) stop(bool, " is not TRUE")
}
assert_equal <- function(expected, actual) {
  if (any(expected != actual)) stop("Expected <", expected, ">, got <", actual, ">")
}
