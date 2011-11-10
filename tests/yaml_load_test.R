source("test_helper.r")

test_should_not_return_named_list <- function() {
  x <- yaml.load("hey: man\n123: 456\n", FALSE)
  assert_equal(2L, length(x))
  assert_equal(2L, length(attr(x, "keys")))

  x <- yaml.load("- dude\n- sup\n- 1.2345", FALSE)
  assert_equal(3L, length(x))
  assert_equal(0L, length(attr(x, "keys")))
  assert_equal("sup", x[[2]])

  x <- yaml.load("dude:\n  - 123\n  - sup", FALSE)
  assert_equal(1L, length(x))
  assert_equal(1L, length(attr(x, "keys")))
  assert_equal(2L, length(x[[1]]))
}

test_should_handle_conflicts <- function() {
  x <- try(yaml.load("hey: buddy\nhey: guy"))
  assert(inherits(x, "try-error"))
}

test_should_return_named_list <- function() {
  x <- yaml.load("hey: man\n123: 456\n", TRUE)
  assert_equal(2L, length(x))
  assert_equal(2L, length(names(x)))
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
}

test_shows_error_with_tag_type_conflicts <- function() {
  x <- try(yaml.load("!!str [1, 2, 3]"))
  assert(inherits(x, "try-error"))

  x <- try(yaml.load("!!str {foo: bar}"))
  assert(inherits(x, "try-error"))
}

test_should_not_collapse_sequences <- function() {
  x <- yaml.load("- [1, 2]\n- 3\n- [4, 5]")
  assert_equal(list(1:2, 3L, 4:5), x)
}

test_should_merge_named_maps <- function() {
  x <- yaml.load("foo: bar\n<<: {baz: boo}", TRUE)
  print(x)
  assert_equal(2L, length(x))
  assert_equal("bar", x$foo)
  assert_equal("boo", x$baz)

  x <- yaml.load("foo: bar\n<<: [{quux: quux}, {foo: doo}, {foo: junk}, {baz: blah}, {baz: boo}]", TRUE)
  assert_equal(3L, length(x))
  assert_equal("blah", x$baz)
  assert_equal("bar", x$foo)
  assert_equal("quux", x$quux)

  x <- yaml.load("foo: bar\n<<: {foo: baz}\n<<: {foo: quux}")
  assert_equal(1L, length(x))
  assert_equal("bar", x$foo)

  x <- yaml.load("<<: {foo: baz}\n<<: {foo: quux}\nfoo: bar")
  assert_equal(1L, length(x))
  assert_equal("baz", x$foo)
}

test_should_merge_unnamed_maps <- function() {
  x <- yaml.load("foo: bar\n<<: {baz: boo}", FALSE)
  assert_equal(2L, length(x))
  assert_equal(list("foo", "baz"), attr(x, 'keys'))
  assert_equal("bar", x[[1]])
  assert_equal("boo", x[[2]])

  x <- yaml.load("foo: bar\n<<: [{quux: quux}, {foo: doo}, {baz: boo}]", FALSE)
  assert_equal(3L, length(x))
  assert_equal(list("foo", "quux", "baz"), attr(x, 'keys'))
  assert_equal("bar", x[[1]])
  assert_equal("quux", x[[2]])
  assert_equal("boo", x[[3]])
}

test_should_fail_on_duplicate_keys_with_merge <- function() {
  x <- try(yaml.load("foo: bar\nfoo: baz\n<<: {foo: quux}", TRUE))
  assert(inherits(x, "try-error"))
}

test_should_handle_weird_merges <- function() {
  x <- try(yaml.load("foo: bar\n<<: [{leet: hax}, blargh, 123]", TRUE))
  assert(inherits(x, "try-error"))

  x <- try(yaml.load("foo: bar\n<<: [123, blargh, {leet: hax}]", TRUE))
  assert(inherits(x, "try-error"))

  x <- try(yaml.load("foo: bar\n<<: junk", TRUE))
  assert(inherits(x, "try-error"))
}

test_should_handle_syntax_errors <- function() {
  x <- try(yaml.load("[whoa, this is some messed up]: yaml?!: dang"))
  assert(inherits(x, "try-error"))
}

test_should_handle_null_type <- function() {
  x <- yaml.load("~")
  assert_equal(NULL, x)
}

#test_should_handle_binary_type <- function() {
#  x <- yaml.load("!!binary 0b101011")
#  assert_equal("0b101011", x)
#}

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
  assert_equal(15L, x)
}

test_should_handle_int_oct_type <- function() {
  x <- yaml.load("015")
  assert_equal(13L, x)
}

#test_should_handle_int_base60_type <- function() {
#  x <- yaml.load("1:20")
#  assert_equal("1:20", x)
#}

test_should_handle_int_type <- function() {
  x <- yaml.load("31337")
  assert_equal(31337L, x)
}

test_should_handle_explicit_int_type <- function() {
  x <- yaml.load("!!int 31337")
  assert_equal(31337L, x)
}

#test_should_handle_float_base60_type <- function() {
#  x <- yaml.load("1:20.5")
#  assert_equal("1:20.5", x)
#}

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

#test_should_handle_timestamp_iso8601_type <- function() {
#  x <- yaml.load("!timestamp#iso8601 2001-12-14t21:59:43.10-05:00")
#  assert_equal("2001-12-14t21:59:43.10-05:00", x)
#}

#test_should_handle_timestamp_spaced_type <- function() {
#  x <- yaml.load("!timestamp#spaced 2001-12-14 21:59:43.10 -5")
#  assert_equal("2001-12-14 21:59:43.10 -5", x)
#}

#test_should_handle_timestamp_ymd_type <- function() {
#  x <- yaml.load("!timestamp#ymd 2008-03-03")
#  assert_equal("2008-03-03", x)
#}

#test_should_handle_timestamp_type <- function() {
#  x <- yaml.load("!timestamp 2001-12-14t21:59:43.10-05:00")
#  assert_equal("2001-12-14t21:59:43.10-05:00", x)
#}

test_should_handle_alias <- function() {
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
  x <- yaml.load("123.456", handlers=list("float#fix"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

test_should_use_custom_timestamp_iso8601_handler <- function() {
  x <- yaml.load("2001-12-14t21:59:43.10-05:00", handlers=list("timestamp#iso8601"=function(x) { "argh!" }))
  assert_equal("argh!", x)
}

#test_should_use_custom_timestamp_spaced_handler <- function() {
#  x <- yaml.load('!"timestamp#spaced" 2001-12-14 21:59:43.10 -5', handlers=list("timestamp#spaced"=function(x) { "argh!" }))
#  assert_equal("argh!", x)
#}

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
  x <- yaml.load("- 1\n- 2\n- 3", handlers=list(seq=function(x) { as.integer(x) + 3L }))
  assert_equal(4:6, x)
}

test_should_use_custom_map_handler <- function() {
  x <- yaml.load("foo: bar", handlers=list(map=function(x) { x$foo <- paste(x$foo, "yarr"); x }))
  assert_equal("bar yarr", x$foo)
}

test_should_use_custom_typed_seq_handler <- function() {
  x <- yaml.load("!foo\n- 1\n- 2", handlers=list(foo=function(x) { as.integer(x) + 1L }))
  assert_equal(2:3, x)
}

test_should_use_custom_typed_map_handler <- function() {
  x <- yaml.load("!foo\nuno: 1\ndos: 2", handlers=list(foo=function(x) { x$uno <- "uno"; x$dos <- "dos"; x }))
  assert_lists_equal(list(uno="uno", dos="dos"), x)
}

# NOTE: this works, but R_tryEval doesn't return when called non-interactively
#test_should_handle_a_bad_handler <- function() {
#  x <- yaml.load("foo", handlers=list(str=function(x) { blargh }))
#  str(x)
#}

test_should_load_empty_documents <- function() {
  x <- yaml.load("")
  assert_equal(NULL, x)
  x <- yaml.load("# this document only has\n   # wickedly awesome comments")
  assert_equal(NULL, x)
}

test_should_load_omap <- function() {
  x <- yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- bar:\n  - 3\n  - 4")
  assert_equal(2L, length(x))
  assert_equal(c("foo", "bar"), names(x))
  assert_equal(1:2, x$foo)
  assert_equal(3:4, x$bar)
}

test_should_load_omap_without_named <- function() {
  x <- yaml.load("--- !omap\n- 123:\n  - 1\n  - 2\n- bar:\n  - 3\n  - 4", FALSE)
  assert_equal(2L, length(x))
  assert_equal(list(123L, "bar"), attr(x, "keys"))
  assert_equal(1:2, x[[1]])
  assert_equal(3:4, x[[2]])
}

test_should_error_when_named_omap_has_duplicate_key  <- function() {
  x <- try(yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- foo:\n  - 3\n  - 4"))
  assert(inherits(x, "try-error"))
}

test_should_error_when_unnamed_omap_has_duplicate_key  <- function() {
  x <- try(yaml.load("--- !omap\n- foo:\n  - 1\n  - 2\n- foo:\n  - 3\n  - 4", FALSE))
  assert(inherits(x, "try-error"))
}

test_should_error_when_omap_is_invalid <- function() {
  x <- try(yaml.load("--- !omap\nhey!"))
  assert(inherits(x, "try-error"))

  x <- try(yaml.load("--- !omap\n- sup?"))
  assert(inherits(x, "try-error"))
}

test_should_convert_expressions <- function() {
  x <- yaml.load("!expr |\n  function() \n  {\n    'hey!'\n  }")
  assert_equal("function", class(x))
  assert_equal("hey!", x())
}

test_should_error_for_bad_expressions <- function() {
  x <- try(yaml.load("!expr |\n  1+"))
  assert(inherits(x, "try-error"))
}

# NOTE: this works, but R_tryEval doesn't return when called non-interactively
#test_should_error_for_expressions_with_eval_errors <- function() {
#  x <- try(yaml.load("!expr |\n  1 + non.existent.variable"))
#  assert(inherits(x, "try-error"))
#}

source("test_runner.r")
