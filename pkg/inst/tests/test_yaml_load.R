context("yaml.load")

test_that("test should not return named list", {
  x <- yaml.load("hey: man\n123: 456\n", FALSE)
  expect_equal(2L, length(x))
  expect_equal(2L, length(attr(x, "keys")))

  x <- yaml.load("- dude\n- sup\n- 1.2345", FALSE)
  expect_equal(3L, length(x))
  expect_equal(0L, length(attr(x, "keys")))
  expect_equal("sup", x[[2]])

  x <- yaml.load("dude:\n  - 123\n  - sup", FALSE)
  expect_equal(1L, length(x))
  expect_equal(1L, length(attr(x, "keys")))
  expect_equal(2L, length(x[[1]]))
})

test_that("test should handle conflicts", {
  expect_that(yaml.load("hey: buddy\nhey: guy"), throws_error());
})

test_that("test should return named list", {
  x <- yaml.load("hey: man\n123: 456\n", TRUE)
  expect_equal(2L, length(x))
  expect_equal(2L, length(names(x)))
  expect_equal(c("123", "hey"), sort(names(x)))
  expect_equal("man", x$hey)
})

test_that("test should type uniform sequences", {
  x <- yaml.load("- 1\n- 2\n- 3")
  expect_equal(1:3, x)

  x <- yaml.load("- yes\n- no\n- yes")
  expect_equal(c(TRUE, FALSE, TRUE), x)

  x <- yaml.load("- 3.1\n- 3.14\n- 3.141")
  expect_equal(c(3.1, 3.14, 3.141), x)

  x <- yaml.load("- hey\n- hi\n- hello")
  expect_equal(c("hey", "hi", "hello"), x)
})

test_that("test shows error with tag type conflicts", {
  expect_that(yaml.load("!!str [1, 2, 3]"), throws_error())
  expect_that(yaml.load("!!str {foo: bar}"), throws_error())
})

test_that("test should not collapse sequences", {
  x <- yaml.load("- [1, 2]\n- 3\n- [4, 5]")
  expect_equal(list(1:2, 3L, 4:5), x)
})

test_that("test should merge named maps", {
  x <- yaml.load("foo: bar\n<<: {baz: boo}", TRUE)
  expect_equal(2L, length(x))
  expect_equal("bar", x$foo)
  expect_equal("boo", x$baz)

  x <- yaml.load("foo: bar\n<<: [{quux: quux}, {foo: doo}, {foo: junk}, {baz: blah}, {baz: boo}]", TRUE)
  expect_equal(3L, length(x))
  expect_equal("blah", x$baz)
  expect_equal("bar", x$foo)
  expect_equal("quux", x$quux)

  x <- yaml.load("foo: bar\n<<: {foo: baz}\n<<: {foo: quux}")
  expect_equal(1L, length(x))
  expect_equal("bar", x$foo)

  x <- yaml.load("<<: {foo: baz}\n<<: {foo: quux}\nfoo: bar")
  expect_equal(1L, length(x))
  expect_equal("baz", x$foo)
})

test_that("test should merge unnamed maps", {
  x <- yaml.load("foo: bar\n<<: {baz: boo}", FALSE)
  expect_equal(2L, length(x))
  expect_equal(list("foo", "baz"), attr(x, 'keys'))
  expect_equal("bar", x[[1]])
  expect_equal("boo", x[[2]])

  x <- yaml.load("foo: bar\n<<: [{quux: quux}, {foo: doo}, {baz: boo}]", FALSE)
  expect_equal(3L, length(x))
  expect_equal(list("foo", "quux", "baz"), attr(x, 'keys'))
  expect_equal("bar", x[[1]])
  expect_equal("quux", x[[2]])
  expect_equal("boo", x[[3]])
})

test_that("test should fail on duplicate keys with merge", {
  expect_that(yaml.load("foo: bar\nfoo: baz\n<<: {foo: quux}", TRUE),
    throws_error())
})

test_that("test should handle weird merges", {
  expect_that(yaml.load("foo: bar\n<<: [{leet: hax}, blargh, 123]", TRUE),
    throws_error())

  expect_that(yaml.load("foo: bar\n<<: [123, blargh, {leet: hax}]", TRUE),
    throws_error())

  expect_that(yaml.load("foo: bar\n<<: junk", TRUE),
    throws_error())
})

test_that("test should handle syntax errors", {
  expect_that(yaml.load("[whoa, this is some messed up]: yaml?!: dang"),
    throws_error())
})

test_that("test should handle null type", {
  x <- yaml.load("~")
  expect_equal(NULL, x)
})

#test_should_handle_binary_type <- function() {
#  x <- yaml.load("!!binary 0b101011")
#  expect_equal("0b101011", x)
#}

test_that("test should handle bool yes type", {
  x <- yaml.load("yes")
  expect_equal(TRUE, x)
})

test_that("test should handle bool no type", {
  x <- yaml.load("no")
  expect_equal(FALSE, x)
})

test_that("test should handle int hex type", {
  x <- yaml.load("0xF")
  expect_equal(15L, x)
})

test_that("test should handle int oct type", {
  x <- yaml.load("015")
  expect_equal(13L, x)
})

#test_should_handle_int_base60_type <- function() {
#  x <- yaml.load("1:20")
#  expect_equal("1:20", x)
#}

test_that("test should handle int type", {
  x <- yaml.load("31337")
  expect_equal(31337L, x)
})

test_that("test should handle explicit int type", {
  x <- yaml.load("!!int 31337")
  expect_equal(31337L, x)
})

#test_should_handle_float_base60_type <- function() {
#  x <- yaml.load("1:20.5")
#  expect_equal("1:20.5", x)
#}

test_that("test should handle float nan type", {
  x <- yaml.load(".NaN")
  expect_true(is.nan(x))
})

test_that("test should handle float inf type", {
  x <- yaml.load(".inf")
  expect_equal(Inf, x)
})

test_that("test should handle float neginf type", {
  x <- yaml.load("-.inf")
  expect_equal(-Inf, x)
})

test_that("test should handle float type", {
  x <- yaml.load("123.456")
  expect_equal(123.456, x)
})

#test_should_handle_timestamp_iso8601_type <- function() {
#  x <- yaml.load("!timestamp#iso8601 2001-12-14t21:59:43.10-05:00")
#  expect_equal("2001-12-14t21:59:43.10-05:00", x)
#}

#test_should_handle_timestamp_spaced_type <- function() {
#  x <- yaml.load("!timestamp#spaced 2001-12-14 21:59:43.10 -5")
#  expect_equal("2001-12-14 21:59:43.10 -5", x)
#}

#test_should_handle_timestamp_ymd_type <- function() {
#  x <- yaml.load("!timestamp#ymd 2008-03-03")
#  expect_equal("2008-03-03", x)
#}

#test_should_handle_timestamp_type <- function() {
#  x <- yaml.load("!timestamp 2001-12-14t21:59:43.10-05:00")
#  expect_equal("2001-12-14t21:59:43.10-05:00", x)
#}

test_that("test should handle alias", {
  x <- yaml.load("- &foo bar\n- *foo")
  expect_equal(c("bar", "bar"), x)
})

test_that("test should handle str type", {
  x <- yaml.load("lickety split")
  expect_equal("lickety split", x)
})

test_that("test should handle a bad anchor", {
  x <- yaml.load("*blargh")
  expected <- "_yaml.bad-anchor_"
  class(expected) <- "_yaml.bad-anchor_"
  expect_equal(expected, x)
})

test_that("test should use custom null handler", {
  x <- yaml.load("~", handlers=list("null"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom binary handler", {
  x <- yaml.load("!binary 0b101011", handlers=list("binary"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom bool yes handler", {
  x <- yaml.load("yes", handlers=list("bool#yes"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom bool no handler", {
  x <- yaml.load("no", handlers=list("bool#no"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom int hex handler", {
  x <- yaml.load("0xF", handlers=list("int#hex"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom int oct handler", {
  x <- yaml.load("015", handlers=list("int#oct"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom int base60 handler", {
  x <- yaml.load("1:20", handlers=list("int#base60"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom int handler", {
  x <- yaml.load("31337", handlers=list("int"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom float base60 handler", {
  x <- yaml.load("1:20.5", handlers=list("float#base60"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom float nan handler", {
  x <- yaml.load(".NaN", handlers=list("float#nan"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom float inf handler", {
  x <- yaml.load(".inf", handlers=list("float#inf"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom float neginf handler", {
  x <- yaml.load("-.inf", handlers=list("float#neginf"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom float handler", {
  x <- yaml.load("123.456", handlers=list("float#fix"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use custom timestamp iso8601 handler", {
  x <- yaml.load("2001-12-14t21:59:43.10-05:00", handlers=list("timestamp#iso8601"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

#test_should_use_custom_timestamp_spaced_handler <- function() {
#  x <- yaml.load('!"timestamp#spaced" 2001-12-14 21:59:43.10 -5', handlers=list("timestamp#spaced"=function(x) { "argh!" }))
#  expect_equal("argh!", x)
#}

test_that("test should use custom timestamp ymd handler", {
  x <- yaml.load("2008-03-03", handlers=list("timestamp#ymd"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should NOT use custom merge handler", {
  x <- yaml.load("foo: &foo\n  bar: 123\n  baz: 456\n\njunk:\n  <<: *foo\n  bah: 789", handlers=list("merge"=function(x) { "argh!" }))
  expect_equal(list(foo=list(bar=123, baz=456), junk=list(bar=123, baz=456, bah=789)), x)
})

test_that("test should use custom str handler", {
  x <- yaml.load("lickety split", handlers=list("str"=function(x) { "argh!" }))
  expect_equal("argh!", x)
})

test_that("test should use handler for weird type", {
  x <- yaml.load("!viking pillage", handlers=list(viking=function(x) { paste(x, "the village") }))
  expect_equal("pillage the village", x)
})

test_that("test should use custom seq handler", {
  x <- yaml.load("- 1\n- 2\n- 3", handlers=list(seq=function(x) { as.integer(x) + 3L }))
  expect_equal(4:6, x)
})

test_that("test should use custom map handler", {
  x <- yaml.load("foo: bar", handlers=list(map=function(x) { x$foo <- paste(x$foo, "yarr"); x }))
  expect_equal("bar yarr", x$foo)
})

test_that("test should use custom typed seq handler", {
  x <- yaml.load("!foo\n- 1\n- 2", handlers=list(foo=function(x) { as.integer(x) + 1L }))
  expect_equal(2:3, x)
})

test_that("test should use custom typed map handler", {
  x <- yaml.load("!foo\nuno: 1\ndos: 2", handlers=list(foo=function(x) { x$uno <- "uno"; x$dos <- "dos"; x }))
  expect_equal(list(uno="uno", dos="dos"), x)
})

# NOTE: this works, but R_tryEval doesn't return when called non-interactively
#test_should_handle_a_bad_handler <- function() {
#  x <- yaml.load("foo", handlers=list(str=function(x) { blargh }))
#  str(x)
#}

test_that("test should load empty documents", {
  x <- yaml.load("")
  expect_equal(NULL, x)
  x <- yaml.load("# this document only has\n   # wickedly awesome comments")
  expect_equal(NULL, x)
})

test_that("test should load omap", {
  x <- yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- bar:\n  - 3\n  - 4")
  expect_equal(2L, length(x))
  expect_equal(c("foo", "bar"), names(x))
  expect_equal(1:2, x$foo)
  expect_equal(3:4, x$bar)
})

test_that("test should load omap without named", {
  x <- yaml.load("--- !omap\n- 123:\n  - 1\n  - 2\n- bar:\n  - 3\n  - 4", FALSE)
  expect_equal(2L, length(x))
  expect_equal(list(123L, "bar"), attr(x, "keys"))
  expect_equal(1:2, x[[1]])
  expect_equal(3:4, x[[2]])
})

test_that("test should error when named omap has duplicate key", {
  expect_that(yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- foo:\n  - 3\n  - 4"),
    throws_error())
})

test_that("test should error when unnamed omap has duplicate key", {
  expect_that(yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- foo:\n  - 3\n  - 4", FALSE),
    throws_error())
})

test_that("test should error when omap is invalid", {
  expect_that(yaml.load("--- !omap\nhey!"),
    throws_error())

  expect_that(yaml.load("--- !omap\n- sup?"),
    throws_error())
})

test_that("test should convert expressions", {
  x <- yaml.load("!expr |\n  function() \n  {\n    'hey!'\n  }")
  expect_equal("function", class(x))
  expect_equal("hey!", x())
})

test_that("test should error for bad expressions", {
  expect_that(yaml.load("!expr |\n  1+"),
    throws_error())
})

# NOTE: this works, but R_tryEval doesn't return when called non-interactively
#test_should_error_for_expressions_with_eval_errors <- function() {
#  x <- try(yaml.load("!expr |\n  1 + non.existent.variable"))
#  assert(inherits(x, "try-error"))
#}

test_that("test maps should be ordered", {
  x <- yaml.load("{a: 1, b: 2, c: 3}")
  expect_equal(c('a', 'b', 'c'), names(x))
})

test_that("test handles recursive illegal anchor", {
  x <- yaml.load('&foo {foo: *foo}')
})

test_that("test sets named properly for aliases", {
  x <- yaml.load('{foo: &foo {one: 1, two: 2}, bar: *foo}')
  x$foo$one <- 'uno'
  expect_equal(1L, x$bar$one)
})

test_that("test anchor issue", {
  x <- yaml.load('{foo: &foo {one: 1}, bar: &bar {one: 1}, baz: *foo, quux: *bar}')
})

test_that("test should not convert quoted string", {
  x <- yaml.load("'12345'")
  expect_equal("12345", x)
})
