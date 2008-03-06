source("test_helper.r")

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

test_should_handle_null_type <- function() {
  x <- yaml.load("~")
  assert_equal(NULL, x)
}

test_should_handle_binary_type <- function() {
  x <- yaml.load("!binary 0b101011")
  assert_equal("0b101011", x)
}

test_should_handle_bool_yes_type <- function() {
  x <- yaml.load("yes")
  assert_equal(TRUE, x)
}

test_should_handle_bool_no_type <- function() {
  x <- yaml.load("no")
  assert_equal(FALSE, x)
}

test_should_handle_int_hex_type <- function() {
  x <- yaml.load("0xF")
  assert_equal(15, x)
}

test_should_handle_int_oct_type <- function() {
  x <- yaml.load("015")
  assert_equal(13, x)
}

test_should_handle_int_base60_type <- function() {
  x <- yaml.load("1:20")
  assert_equal("1:20", x)
}

test_should_handle_int_type <- function() {
  x <- yaml.load("31337")
  assert_equal(31337, x)
}

test_should_handle_float_base60_type <- function() {
  x <- yaml.load("1:20.5")
  assert_equal("1:20.5", x)
}

test_should_handle_float_nan_type <- function() {
  x <- yaml.load(".NaN")
  assert_nan(x)
}

test_should_handle_float_inf_type <- function() {
  x <- yaml.load(".inf")
  assert_equal(Inf, x)
}

test_should_handle_float_neginf_type <- function() {
  x <- yaml.load("-.inf")
  assert_equal(-Inf, x)
}

test_should_handle_float_type <- function() {
  x <- yaml.load("123.456")
  assert_equal(123.456, x)
}

test_should_handle_timestamp_iso8601_type <- function() {
  x <- yaml.load("!timestamp#iso8601 2001-12-14t21:59:43.10-05:00")
  assert_equal("2001-12-14t21:59:43.10-05:00", x)
}

test_should_handle_timestamp_spaced_type <- function() {
  x <- yaml.load("!timestamp#spaced 2001-12-14 21:59:43.10 -5")
  assert_equal("2001-12-14 21:59:43.10 -5", x)
}

test_should_handle_timestamp_ymd_type <- function() {
  x <- yaml.load("!timestamp#ymd 2008-03-03")
  assert_equal("2008-03-03", x)
}

test_should_handle_timestamp_type <- function() {
  x <- yaml.load("!timestamp 2001-12-14t21:59:43.10-05:00")
  assert_equal("2001-12-14t21:59:43.10-05:00", x)
}

test_should_handle_merge_type <- function() {
  x <- yaml.load("- &foo bar\n- *foo")
  assert_equal(c("bar", "bar"), x)
}

test_should_handle_str_type <- function() {
  x <- yaml.load("lickety split")
  assert_equal("lickety split", x)
}

test_should_handle_a_bad_anchor <- function() {
  x <- yaml.load("*blargh")
  print(x)
}

test_should_use_custom_null_handler <- function() {
  x <- yaml.load("~", handlers=list("null"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_binary_handler <- function() {
  x <- yaml.load("!binary 0b101011", handlers=list("binary"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_bool_yes_handler <- function() {
  x <- yaml.load("yes", handlers=list("bool#yes"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_bool_no_handler <- function() {
  x <- yaml.load("no", handlers=list("bool#no"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_int_hex_handler <- function() {
  x <- yaml.load("0xF", handlers=list("int#hex"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_int_oct_handler <- function() {
  x <- yaml.load("015", handlers=list("int#oct"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_int_base60_handler <- function() {
  x <- yaml.load("1:20", handlers=list("int#base60"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_int_handler <- function() {
  x <- yaml.load("31337", handlers=list("int"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_float_base60_handler <- function() {
  x <- yaml.load("1:20.5", handlers=list("float#base60"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_float_nan_handler <- function() {
  x <- yaml.load(".NaN", handlers=list("float#nan"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_float_inf_handler <- function() {
  x <- yaml.load(".inf", handlers=list("float#inf"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_float_neginf_handler <- function() {
  x <- yaml.load("-.inf", handlers=list("float#neginf"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_float_handler <- function() {
  x <- yaml.load("123.456", handlers=list("float"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_timestamp_iso8601_handler <- function() {
  x <- yaml.load("2001-12-14t21:59:43.10-05:00", handlers=list("timestamp#iso8601"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_timestamp_spaced_handler <- function() {
  x <- yaml.load("!timestamp#spaced 2001-12-14 21:59:43.10 -5", handlers=list("timestamp#spaced"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_timestamp_ymd_handler <- function() {
  x <- yaml.load("2008-03-03", handlers=list("timestamp#ymd"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_NOT_use_custom_merge_handler <- function() {
  x <- yaml.load("foo: &foo\n  bar: 123\n  baz: 456\n\njunk:\n  <<: *foo\n  bah: 789", handlers=list("merge"=function(x) { "argh!" }))
  assert_lists_equal(list(foo=list(bar=123, baz=456), junk=list(bar=123, baz=456, bah=789)), x)
}

test_should_use_custom_str_handler <- function() {
  x <- yaml.load("lickety split", handlers=list("str"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_NOT_use_custom_bad_anchor_handler <- function() {
  x <- yaml.load("*blargh", handlers=list("anchor#bad"=function(x) { "argh!" }))
  assert_equal("_yaml.bad-anchor_", x)
}

test_should_use_handler_for_weird_type <- function() {
  x <- yaml.load("!viking pillage", handlers=list(viking=function(x) { paste(x, "the village") }))
  assert_equal("pillage the village", x)
}

test_should_use_custom_seq_handler <- function() {
  x <- yaml.load("- 1\n- 2\n- 3", handlers=list(seq=function(x) { x + 3 }))
  assert_equal(4:6, x)
}

test_should_use_custom_map_handler <- function() {
  x <- yaml.load("foo: bar", handlers=list(map=function(x) { x$foo <- paste(x$foo, "yarr"); x }))
  assert_equal("bar yarr", x$foo)
}

source("test_runner.r")
