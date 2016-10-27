`yaml.load_file` <-
function(input, ...) {
  yaml.load(paste(readLines(input, encoding = 'UTF-8'), collapse="\n"), ...)
}
