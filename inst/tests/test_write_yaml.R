test_output_is_written_to_a_file_when_a_filename_is_specified <- function() {
  filename <- tempfile()
  write_yaml(1:3, filename)
  output <- readLines(filename)
  unlink(filename)
  checkEquals(c("- 1", "- 2", "- 3"), output)
}

# Latin1 UTF-8 test
test_output_from_latin1_no_enc_works <- function()
{
  filename <- tempfile()
  write_yaml(list("\xab"), filename)
  output <- readLines(filename)
  unlink(filename)
  checkEquals(c("- <ab>"), output)
}

test_output_from_latin1_enc_works <- function()
{
  x <- "\xab\xef"
  Encoding(x) <- "latin1"
  filename <- tempfile()
  write_yaml(list(x), filename)
  output <- readLines(filename)
  unlink(filename)
  checkEquals("- «ï", output)
}