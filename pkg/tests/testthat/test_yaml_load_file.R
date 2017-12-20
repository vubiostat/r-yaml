context("yaml.load_file")

test_that("reading from a connection works", {
  cat("foo: 123", file="files/foo.yml", sep="\n")
  foo <- file('files/foo.yml', 'r')
  x <- yaml.load_file(foo)
  close(foo)
  unlink("files/foo.yml")
  expect_equal(123L, x$foo)
})

test_that("reading from specified filename works", {
  cat("foo: 123", file="files/foo.yml", sep="\n")
  x <- yaml.load_file('files/foo.yml')
  unlink("files/foo.yml")
  expect_equal(123L, x$foo)
})

test_that("reading a complicated document works", {
  x <- yaml.load_file('files/test.yml')
  expected <- list(
    foo = list(one = 1, two = 2),
    bar = list(three = 3, four = 4),
    baz = list(list(one = 1, two = 2), list(three = 3, four = 4)),
    quux = list(one = 1, two = 2, three = 3, four = 4, five = 5, six = 6),
    corge = list(
      list(one = 1, two = 2, three = 3, four = 4, five = 5, six = 6),
      list(xyzzy = list(one = 1, two = 2, three = 3, four = 4, five = 5, six = 6))
    )
  )
  expect_equal(expected, x)
})
