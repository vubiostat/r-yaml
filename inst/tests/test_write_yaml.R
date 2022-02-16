test_output_is_written_to_a_file_when_a_filename_is_specified <- function() {
  filename <- tempfile()
  write_yaml(1:3, filename)
  output <- readLines(filename)
  unlink(filename)
  checkEquals(c("- 1", "- 2", "- 3"), output)
}

# Latin1 UTF-8 test
test_output_from_latin1_works <- function()
{
  filename <- tempfile()
  write_yaml(list("\xab"), filename)
  output <- readLines(filename)
  unlink(filename)
  checkEquals(c("- \xab"), output)
}