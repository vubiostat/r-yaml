source("test_helper.r")

test_should_read_from_connection <- function() {
  cat("foo: 123", file="pants.yml", sep="\n")
  pants <- file('pants.yml', 'r')
  x <- yaml.load_file(pants)
  close(pants)
  unlink("pants.yml")
  assert_equal(123L, x$foo)
}

test_should_read_from_filename <- function() {
  cat("foo: 123", file="pants.yml", sep="\n")
  x <- yaml.load_file('pants.yml')
  unlink("pants.yml")
  assert_equal(123L, x$foo)
}

source("test_runner.r")
