VERSION = $(shell cat VERSION)

SRCS =  pkg/src/yaml_private.h \
	pkg/src/writer.c \
	pkg/src/scanner.c \
	pkg/src/dumper.c \
	pkg/src/emitter.c \
	pkg/src/implicit.re \
	pkg/src/reader.c \
	pkg/src/parser.c \
	pkg/src/api.c \
	pkg/src/r-ext.h \
	pkg/src/yaml.h \
	pkg/src/loader.c \
	pkg/src/r-ext.c \
	pkg/src/Makevars \
	pkg/man/as.yaml.Rd \
	pkg/man/yaml.load.Rd \
	pkg/inst/THANKS \
	pkg/inst/CHANGELOG \
	pkg/DESCRIPTION.brew \
	pkg/COPYING \
	pkg/R/yaml.load.R \
	pkg/R/zzz.R \
	pkg/R/yaml.load_file.R \
	pkg/R/as.yaml.R \
	pkg/NAMESPACE \
	pkg/tests/as_yaml_test.R \
	pkg/tests/test_runner.r \
	pkg/tests/yaml_load_file_test.R \
	pkg/tests/test_helper.r \
	pkg/tests/yaml_load_test.R \
	pkg/tests/test.yml

BUILD_SRCS = build/src/yaml_private.h \
	     build/src/writer.c \
	     build/src/scanner.c \
	     build/src/dumper.c \
	     build/src/emitter.c \
	     build/src/implicit.c \
	     build/src/reader.c \
	     build/src/parser.c \
	     build/src/api.c \
	     build/src/r-ext.h \
	     build/src/yaml.h \
	     build/src/loader.c \
	     build/src/r-ext.c \
	     build/src/Makevars \
	     build/man/as.yaml.Rd \
	     build/man/yaml.load.Rd \
	     build/inst/THANKS \
	     build/inst/CHANGELOG \
	     build/inst/implicit.re \
	     build/DESCRIPTION \
	     build/COPYING \
	     build/R/yaml.load.R \
	     build/R/zzz.R \
	     build/R/yaml.load_file.R \
	     build/R/as.yaml.R \
	     build/NAMESPACE \
	     build/tests/as_yaml_test.R \
	     build/tests/test_runner.r \
	     build/tests/yaml_load_file_test.R \
	     build/tests/test_helper.r \
	     build/tests/yaml_load_test.R \
	     build/tests/test.yml

all: $(BUILD_SRCS)
	R CMD COMPILE CFLAGS="-g3" build/src/*.c
	R CMD SHLIB build/src/*.o -o build/src/yaml.so

check: $(BUILD_SRCS)
	R CMD check -o `mktemp -d` build

yaml_$(VERSION).tar.gz: $(BUILD_SRCS)
	R CMD build build
	R CMD check -o `mktemp -d` $@

build/DESCRIPTION: pkg/DESCRIPTION.brew
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
	rm -fr yaml_$(VERSION).tar.gz build
