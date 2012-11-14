context("yaml.load_file")

test_that("test should read from connection", {
  cat("foo: 123", file="files/foo.yml", sep="\n")
  foo <- file('files/foo.yml', 'r')
  x <- yaml.load_file(foo)
  close(foo)
  unlink("files/foo.yml")
  expect_equal(123L, x$foo)
})

test_that("test should read from filename", {
  cat("foo: 123", file="files/foo.yml", sep="\n")
  x <- yaml.load_file('files/foo.yml')
  unlink("files/foo.yml")
  expect_equal(123L, x$foo)
})

test_that("test should read complicated document", {
  x <- yaml.load_file('files/test.yml')
})
