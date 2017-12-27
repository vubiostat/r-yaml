context("write_yaml")

test_that("output is written to a file when a filename is specified", {
  filename <- tempfile()
  write_yaml(1:3, filename)
  output <- readLines(filename)
  unlink(filename)
  expect_equal(c("- 1", "- 2", "- 3"), output)
})
