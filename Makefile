VERSION = $(shell cat VERSION)

SRCS = pkg/src/yaml_private.h \
	pkg/src/yaml.h \
	pkg/src/r_ext.h \
	pkg/src/writer.c \
	pkg/src/scanner.c \
	pkg/src/dumper.c \
	pkg/src/emitter.c \
	pkg/src/implicit.re \
	pkg/src/reader.c \
	pkg/src/parser.c \
	pkg/src/api.c \
	pkg/src/loader.c \
	pkg/src/r_ext.c \
	pkg/src/r_emit.c \
	pkg/src/r_parse.c \
	pkg/src/Makevars \
	pkg/man/as.yaml.Rd \
	pkg/man/yaml.load.Rd \
	pkg/inst/THANKS \
	pkg/inst/CHANGELOG \
	pkg/tests/testthat.R \
	pkg/tests/testthat/test_yaml_load_file.R \
	pkg/tests/testthat/test_yaml_load.R \
	pkg/tests/testthat/test_as_yaml.R \
	pkg/tests/testthat/test_read_yaml.R \
	pkg/tests/testthat/files/test.yml \
	pkg/DESCRIPTION.brew \
	pkg/COPYING \
	pkg/LICENSE \
	pkg/R/yaml.load.R \
	pkg/R/zzz.R \
	pkg/R/yaml.load_file.R \
	pkg/R/as.yaml.R \
	pkg/R/read_yaml.R \
	pkg/R/write_yaml.R \
	pkg/NAMESPACE

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
	build/inst/THANKS \
	build/inst/CHANGELOG \
	build/inst/implicit.re \
	build/tests/testthat.R \
	build/tests/testthat/test_yaml_load_file.R \
	build/tests/testthat/test_yaml_load.R \
	build/tests/testthat/test_as_yaml.R \
	build/tests/testthat/test_read_yaml.R \
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
  CFLAGS = -g3 -Wall -DDEBUG
else
  CFLAGS = -g3 -Wall
endif

all: compile test

compile: $(BUILD_SRCS)
	R CMD COMPILE CFLAGS="$(CFLAGS)" build/src/*.c
	R CMD SHLIB build/src/*.o -o build/src/yaml.so

check: compile $(BUILD_SRCS)
	R CMD check -o `mktemp -d` build

gct-check: $(BUILD_SRCS)
	R CMD check --use-gct -o `mktemp -d` build

test: compile check-changelog
	cd build; echo "library(devtools); test('.')" | R --vanilla

gdb-test: compile check-changelog
	cd build; R -d gdb --vanilla -e "library(devtools); test('.')"

valgrind-test: compile check-changelog
	cd build/tests; cat *.R | R --vanilla -d "valgrind --leak-check=full"

check-changelog: VERSION pkg/inst/CHANGELOG
	if [ VERSION -nt pkg/inst/CHANGELOG ]; then echo "\033[31mWARNING: Changelog has not been updated\033[0m"; fi

tarball: yaml_$(VERSION).tar.gz

yaml_$(VERSION).tar.gz: $(BUILD_SRCS)
	R CMD build build
	check_dir=`mktemp -d`; echo Check directory: $$check_dir; R CMD check --as-cran -o "$$check_dir" $@

build/DESCRIPTION: pkg/DESCRIPTION.brew VERSION
	mkdir -p $(dir $@)
	R --vanilla -e "VERSION <- '$(VERSION)'; library(brew); brew('$<', '$@');"

build/src/implicit.c: pkg/src/implicit.re
	mkdir -p $(dir $@)
	cd $(dir $<); re2c -o $(notdir $@) --no-generation-date $(notdir $<)
	mv $(dir $<)implicit.c $@

build/inst/implicit.re: pkg/src/implicit.re
	mkdir -p $(dir $@)
	cp $< $@

build/%: pkg/%
	mkdir -p $(dir $@)
	cp $< $@

clean:
	rm -fr yaml_*.tar.gz build

.PHONY: all compile check gct-check test gdb-test clean valgrind-test check-changelog tarball
