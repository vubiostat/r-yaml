source("test_helper.r")

test_should_convert_named_list_to_yaml <-
function() {
  assert_equal("foo: bar", as.yaml(list(foo="bar")))

  x <- list(foo=1:10, bar=c("sup", "g"))
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

  y <- yaml.load(as.yaml(x, column.major = FALSE))
  assert_equal(5,   y[[5]]$a)
  assert_equal("e", y[[5]]$b)
  assert_equal(15,  y[[5]]$c)
}

test_should_convert_empty_nested_list <-
function() {
  x <- list(foo=list())
  assert_equal("foo:\n  []", as.yaml(x))
}

test_should_convert_empty_nested_data_frame <-
function() {
  x <- list(foo=data.frame())
  assert_equal("foo:\n  []", as.yaml(x))
}

test_should_convert_empty_nested_vector <-
function() {
  x <- list(foo=c())
  assert_equal("foo: []", as.yaml(x))
}

test_should_convert_list_as_omap <-
function() {
  x <- list(a=1:2, b=3:4)
  expected <- "--- !omap\n- a:\n    - 1\n    - 2\n- b:\n    - 3\n    - 4"
  assert_equal(expected, as.yaml(x, omap=TRUE))
}

test_should_convert_nested_lists_as_omap <-
function() {
  x <- list(a=list(c=list(e=1L, f=2L)), b=list(d=list(g=3L, h=4L)))
  expected <- "--- !omap\n- a: !omap\n    - c: !omap\n        - e: 1\n        - f: 2\n- b: !omap\n    - d: !omap\n        - g: 3\n        - h: 4"
  assert_equal(expected, as.yaml(x, omap=TRUE))
}

test_should_load_omap <-
function() {
  x <- yaml.load(as.yaml(list(a=1:2, b=3:4, c=5:6, d=7:8), omap=TRUE))
  assert_equal(c("a", "b", "c", "d"), names(x))
}

test_should_convert_numeric_correctly <-
function() {
  assert_equal("1.0", as.yaml(1.0))
}

source("test_runner.r")
