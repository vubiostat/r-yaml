dyn.load("../src/yaml.so")
for (f in list.files("../R", full.names = TRUE))
  source(f)
#library(yaml)

assert <- function(bool) {
  if (!bool) stop(bool, " is not TRUE")
}
assert_equal <- function(expected, actual) {
  if (any(expected != actual)) stop("Expected <", expected, ">, got <", actual, ">")
}
assert_nan <- function(value) {
  if (!is.nan(value)) stop(value, " is not NaN")
}
assert_lists_equal <- function(expected, actual) {
  if (any(sort(names(expected)) != sort(names(actual)))) stop("Lists are not equal (names differ)")
  if (any(class(expected) != class(actual))) stop("Lists are not equal (classes differ)")

  for (n in names(expected)) {
    e <- expected[[n]]
    a <- actual[[n]]
    if (class(e) == "list" && class(a) == "list") assert_lists_equal(e, a)
    else if (e != a) stop("Lists are not equal (elements differ)")
  }
}
