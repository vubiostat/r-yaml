VERSION = $(shell cat VERSION)

SRCS = src/yaml_private.h \
	src/yaml.h \
	src/r_ext.h \
	src/writer.c \
	src/scanner.c \
	src/dumper.c \
	src/emitter.c \
	src/implicit.re \
	src/reader.c \
	src/parser.c \
	src/api.c \
	src/loader.c \
	src/r_ext.c \
	src/r_emit.c \
	src/r_parse.c \
	src/Makevars \
	man/as.yaml.Rd \
	man/yaml.load.Rd \
	man/write_yaml.Rd \
	man/read_yaml.Rd \
	inst/THANKS \
	inst/CHANGELOG \
	tests/testthat.R \
	tests/testthat/test_yaml_load_file.R \
	tests/testthat/test_yaml_load.R \
	tests/testthat/test_as_yaml.R \
	tests/testthat/test_read_yaml.R \
	tests/testthat/test_write_yaml.R \
	tests/testthat/files/test.yml \
	DESCRIPTION \
	COPYING \
	LICENSE \
	R/yaml.load.R \
	R/zzz.R \
	R/yaml.load_file.R \
	R/as.yaml.R \
	R/read_yaml.R \
	R/write_yaml.R \
	NAMESPACE

BUILD_SRCS = build/src/yaml_private.h \
	build/src/yaml.h \
	build/src/r_ext.h \
	build/src/writer.c \
	build/src/scanner.c \
	build/src/dumper.c \
	build/src/emitter.c \
	build/src/implicit.c \
	build/src/reader.c \
	build/src/parser.c \
	build/src/api.c \
	build/src/loader.c \
	build/src/r_ext.c \
	build/src/r_emit.c \
	build/src/r_parse.c \
	build/src/Makevars \
	build/man/as.yaml.Rd \
	build/man/yaml.load.Rd \
	build/man/write_yaml.Rd \
	build/man/read_yaml.Rd \
	build/inst/THANKS \
	build/inst/CHANGELOG \
	build/inst/implicit.re \
	build/tests/testthat.R \
	build/tests/testthat/test_yaml_load_file.R \
	build/tests/testthat/test_yaml_load.R \
	build/tests/testthat/test_as_yaml.R \
	build/tests/testthat/test_read_yaml.R \
	build/tests/testthat/test_write_yaml.R \
	build/tests/testthat/files/test.yml \
	build/DESCRIPTION \
	build/COPYING \
	build/LICENSE \
	build/R/yaml.load.R \
	build/R/zzz.R \
	build/R/yaml.load_file.R \
	build/R/as.yaml.R \
	build/R/read_yaml.R \
	build/R/write_yaml.R \
	build/NAMESPACE

ifdef DEBUG
  CFLAGS = -g3 -Wall -Wextra -Wpedantic -DDEBUG
else
  CFLAGS = -g3 -Wall -Wextra -Wpedantic
endif

all: compile test

compile: $(BUILD_SRCS)
	R CMD COMPILE CFLAGS="$(CFLAGS)" build/src/*.c
	R CMD SHLIB build/src/*.o -o build/src/yaml.so

check: compile $(BUILD_SRCS)
	R CMD check -o `mktemp -d` build

gct-check: $(BUILD_SRCS)
	R CMD check --use-gct -o `mktemp -d` build

test: compile
	cd build; echo "library(devtools); test('.')" | R --vanilla

gdb-test: compile
	cd build; R -d gdb --vanilla -e "library(devtools); test('.')"

valgrind-test: compile
	cd build; R -d "valgrind --leak-check=full" -e "library(devtools); test('.')"

check-changelog: VERSION inst/CHANGELOG
	@if [ VERSION -nt inst/CHANGELOG ]; then echo -e "\033[31mWARNING: CHANGELOG has not been updated\033[0m"; fi

check-description: VERSION DESCRIPTION
	@if [ VERSION -nt DESCRIPTION ]; then echo -e "\033[31mWARNING: DESCRIPTION has not been updated\033[0m"; fi

tarball: yaml_$(VERSION).tar.gz check-changelog check-description

yaml_$(VERSION).tar.gz: $(BUILD_SRCS)
	R CMD build build
	check_dir=`mktemp -d`; echo Check directory: $$check_dir; R CMD check --as-cran -o "$$check_dir" $@

src/implicit.c: src/implicit.re
	cd $(dir $<); re2c -o $(notdir $@) --no-generation-date $(notdir $<)

build/inst/implicit.re: src/implicit.re
	mkdir -p $(dir $@)
	cp $< $@

build/%: %
	mkdir -p $(dir $@)
	cp $< $@

clean:
	rm -fr yaml_*.tar.gz build

.PHONY: all compile check gct-check test gdb-test clean valgrind-test check-changelog check-description tarball
