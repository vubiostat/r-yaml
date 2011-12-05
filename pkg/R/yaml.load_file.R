`yaml.load_file` <-
function(input, ...) {
  yaml.load(paste(readLines(input), collapse="\n"), ...)
}
