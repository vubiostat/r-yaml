source("test_helper.R")

test_should_not_return_named_list <- function() {
  x <- yaml.load("hey: man\n123: 456\n", FALSE)
  assert_equal(2, length(x))
  assert_equal(2, length(attr(x, "keys")))

  x <- yaml.load("- dude\n- sup\n- 1.2345", FALSE)
  assert_equal(3, length(x))
  assert_equal(0, length(attr(x, "keys")))
  assert_equal("sup", x[[2]])

  x <- yaml.load("dude:\n  - 123\n  - sup", FALSE)
  assert_equal(1, length(x))
  assert_equal(1, length(attr(x, "keys")))
  assert_equal(2, length(x[[1]]))
}

test_should_handle_conflicts <- function() {
  x <- yaml.load("hey: buddy\nhey: guy")
  assert_equal(1, length(x))
  assert_equal("buddy", x[[1]])
}

test_should_return_named_list <- function() {
  x <- yaml.load("hey: man\n123: 456\n", TRUE)
  assert_equal(2, length(x))
  assert_equal(2, length(names(x)))
  assert_equal(c("123", "hey"), sort(names(x)))
  assert_equal("man", x$hey)
}

test_should_type_uniform_sequences <- function() {
  x <- yaml.load("- 1\n- 2\n- 3")
  assert_equal(1:3, x)

  x <- yaml.load("- yes\n- no\n- yes")
  assert_equal(c(TRUE, FALSE, TRUE), x)

  x <- yaml.load("- 3.1\n- 3.14\n- 3.141")
  assert_equal(c(3.1, 3.14, 3.141), x)

  x <- yaml.load("- hey\n- hi\n- hello")
  assert_equal(c("hey", "hi", "hello"), x)

  x <- yaml.load("- [1, 2]\n- 3\n- [4, 5]")
  assert_equal(1:5, x)
}

test_should_merge_maps <- function() {
  x <- yaml.load("foo: bar\n<<: {baz: boo}", TRUE)
  assert_equal(2, length(x))
  assert_equal("boo", x$baz)
  assert_equal("foo", x$bar)

  x <- yaml.load("foo: bar\n<<: [{poo: poo}, {foo: doo}, {baz: boo}]", TRUE)
  assert_equal(3, length(x))
  assert_equal("boo", x$baz)
  assert_equal("bar", x$foo)
  assert_equal("poo", x$poo)
}

test_should_handle_weird_merges <- function() {
  x <- yaml.load("foo: bar\n<<: [{leet: hax}, blargh, 123]", T)
  assert_equal(3, length(x))
  assert_equal("hax", x$leet)
  assert_equal("blargh", x$`_yaml.merge_`)
}

test_should_handle_syntax_errors <- function() {
  x <- try(yaml.load("[whoa, this is some messed up]: yaml?!: dang"))
  assert(inherits(x, "try-error"))
}

source("test_runner.R")
