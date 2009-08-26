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

source("test_runner.r")
