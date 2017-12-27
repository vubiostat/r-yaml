context("read_yaml")

test_that("reading from a connection works", {
  filename <- tempfile()
  cat("foo: 123", file=filename, sep="\n")
  foo <- file(filename, 'r')
  x <- read_yaml(foo)
  close(foo)
  unlink(filename)
  expect_equal(123L, x$foo)
})

test_that("reading from specified filename works", {
  filename <- tempfile()
  cat("foo: 123", file=filename, sep="\n")
  x <- read_yaml(filename)
  unlink(filename)
  expect_equal(123L, x$foo)
})

test_that("reading from text works", {
  x <- read_yaml(text="foo: 123")
  expect_equal(123L, x$foo)
})

test_that("reading a complicated document works", {
  x <- read_yaml('files/test.yml')
  expected <- list(
    foo   = list(one=1, two=2),
    bar   = list(three=3, four=4),
    baz   = list(list(one=1, two=2), list(three=3, four=4)),
    quux  = list(one=1, two=2, three=3, four=4, five=5, six=6),
    corge = list(
      list(one=1, two=2, three=3, four=4, five=5, six=6),
      list(xyzzy=list(one=1, two=2, three=3, four=4, five=5, six=6))
    )
  )
  expect_equal(expected, x)
})
