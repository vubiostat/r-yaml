source("test_helper.r")

test_should_read_from_connection <- function() {
  cat("foo: 123", file="foo.yml", sep="\n")
  foo <- file('foo.yml', 'r')
  x <- yaml.load_file(foo)
  close(foo)
  unlink("foo.yml")
  assert_equal(123L, x$foo)
}

test_should_read_from_filename <- function() {
  cat("foo: 123", file="foo.yml", sep="\n")
  x <- yaml.load_file('foo.yml')
  unlink("foo.yml")
  assert_equal(123L, x$foo)
}

test_should_read_complicated_document <- function() {
  x <- yaml.load_file('test.yml')
}

source("test_runner.r")
