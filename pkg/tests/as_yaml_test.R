source("test_helper.r")

test_should_convert_named_list_to_yaml <-
function() {
  assert_equal("foo: bar\n", as.yaml(list(foo="bar")))

  x <- list(foo=1:10, bar=c("junk", "test"))
  y <- yaml.load(as.yaml(x))
  assert_equal(x$foo, y$foo)
  assert_equal(x$bar, y$bar)

  x <- list(foo=1:10, bar=list(foo=11:20, bar=letters[1:10]))
  y <- yaml.load(as.yaml(x))
  assert_equal(x$foo, y$foo)
  assert_equal(x$bar$foo, y$bar$foo)
  assert_equal(x$bar$bar, y$bar$bar)

  # nested lists
  x <- list(foo = list(a = 1, b = 2), bar = list(b = 4))
  y <- yaml.load(as.yaml(x))
  assert_equal(x$foo$a, y$foo$a)
  assert_equal(x$foo$b, y$foo$b)
  assert_equal(x$bar$b, y$bar$b)
}

test_should_convert_data_frame_to_yaml <-
function() {
  x <- data.frame(a=1:10, b=letters[1:10], c=11:20)
  y <- as.data.frame(yaml.load(as.yaml(x)))
  assert_equal(x$a, y$a)
  assert_equal(x$b, y$b)
  assert_equal(x$c, y$c)

  x <- as.yaml(x, column.major = FALSE)
  y <- yaml.load(x)
  assert_equal(5L,  y[[5]]$a)
  assert_equal("e", y[[5]]$b)
  assert_equal(15L, y[[5]]$c)
}

test_should_convert_empty_nested_list <-
function() {
  x <- list(foo=list())
  assert_equal("foo: []\n", as.yaml(x))
}

test_should_convert_empty_nested_data_frame <-
function() {
  x <- list(foo=data.frame())
  assert_equal("foo: {}\n", as.yaml(x))
}

test_should_convert_empty_nested_vector <-
function() {
  x <- list(foo=character())
  assert_equal("foo: []\n", as.yaml(x))
}

test_should_convert_list_as_omap <-
function() {
  x <- list(a=1:2, b=3:4)
  expected <- "!omap\n- a:\n  - 1\n  - 2\n- b:\n  - 3\n  - 4\n"
  assert_equal(expected, as.yaml(x, omap=TRUE))
}

test_should_convert_nested_lists_as_omap <-
function() {
  x <- list(a=list(c=list(e=1L, f=2L)), b=list(d=list(g=3L, h=4L)))
  expected <- "!omap\n- a: !omap\n  - c: !omap\n    - e: 1\n    - f: 2\n- b: !omap\n  - d: !omap\n    - g: 3\n    - h: 4\n"
  assert_equal(expected, as.yaml(x, omap=TRUE))
}

test_should_load_omap <-
function() {
  x <- yaml.load(as.yaml(list(a=1:2, b=3:4, c=5:6, d=7:8), omap=TRUE))
  assert_equal(c("a", "b", "c", "d"), names(x))
}

test_should_convert_numeric_correctly <-
function() {
  assert_equal("- 1.0\n- 5.0\n- 10.0\n- 15.0\n", as.yaml(c(1, 5, 10, 15)))
}

test_multiline_string <-
function() {
  assert_equal("|-\n  foo\n  bar\n", as.yaml("foo\nbar"))
  assert_equal("- foo\n- |-\n  bar\n  baz\n", as.yaml(c("foo", "bar\nbaz")))
  assert_equal("foo: |-\n  foo\n  bar\n", as.yaml(list(foo = "foo\nbar")))
  assert_equal("a:\n- foo\n- bar\n- |-\n  baz\n  quux\n", as.yaml(data.frame(a = c('foo', 'bar', 'baz\nquux'))))
}

test_function <-
function() {
  x <- function() { runif(100) }
  expected <- "!expr |\n  function ()\n  {\n      runif(100)\n  }\n"
  result <- as.yaml(x)
  assert_equal(expected, result)
}

test_list_with_unnamed_items <-
function() {
  x <- list(foo=list(list(x = 1L, y = 2L), list(x = 3L, y = 4L)))
  expected <- "foo:
- x: 1
  y: 2
- x: 3
  y: 4
"
  result <- as.yaml(x)
  assert_equal(expected, result)
}

test_should_escape_pound_signs_in_strings <-
function() {
  result <- as.yaml("foo # bar")
  assert_equal("'foo # bar'\n", result)
}

test_should_convert_null <-
function() {
  assert_equal("~\n...\n", as.yaml(NULL))
}

test_explicit_linesep <- function() {
  result <- as.yaml(c('foo', 'bar'), line.sep = "\n")
  assert_equal("- foo\n- bar\n", result)

  result <- as.yaml(c('foo', 'bar'), line.sep = "\r\n")
  assert_equal("- foo\r\n- bar\r\n", result)

  result <- as.yaml(c('foo', 'bar'), line.sep = "\r")
  assert_equal("- foo\r- bar\r", result)
}

test_custom_indent <- function() {
  result <- as.yaml(list(foo=list(bar=list('foo', 'bar'))), indent = 3)
  assert_equal("foo:\n   bar:\n   - foo\n   - bar\n", result)
}

test_bad_indent <- function() {
  result <- try(as.yaml(list(foo=list(bar=list('foo', 'bar'))), indent = 0))
  assert(inherits(result, "try-error"))
}

source("test_runner.r")
