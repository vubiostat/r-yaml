context("write_yaml")

#test_that("output is written to stdout when file is an empty string", {
#  con <- textConnection("output", "w")
#  sink(con)
#  write_yaml(1:3, "")
#  sink()
#  expect_equal(c("- 1", "- 2", "- 3"), output)
#})

test_that("output is written to a file when a filename is specified", {
  filename <- tempfile()
  write_yaml(1:3, filename)
  output <- readLines(filename)
  unlink(filename)
  expect_equal(c("- 1", "- 2", "- 3"), output)
})
