
R YAML package
==============

The R [YAML](http://yaml.org) package implements the
[libyaml](http://pyyaml.org/wiki/LibYAML) YAML parser and emitter for R.

## Table of Contents

* [What is YAML?](#what-is-yaml)
* [Installation](#installation)
* [Usage](#usage)
  * [yaml.load](#yamlload)
  * [yaml.load_file](#yamlload_file)
  * [read_yaml](#read_yaml)
  * [as.yaml](#asyaml)
  * [write_yaml](#write_yaml)
* [Development](#development)

## What is YAML?

YAML is a human-readable markup language. With it, you can create easily
readable documents that can be consumed by a variety of programming languages.

### Examples

Hash of baseball teams per league:

    american:
    - Boston Red Sox
    - Detroit Tigers
    - New York Yankees
    national:
    - New York Mets
    - Chicago Cubs
    - Atlanta Braves

Data dictionary specification:

    - field: ID
      description: primary identifier
      type: integer
      primary key: yes
    - field: DOB
      description: date of birth
      type: date
      format: yyyy-mm-dd
    - field: State
      description: state of residence
      type: string

## Installation

### CRAN

You can install this package directly from CRAN by running (from within R):
`install.packages('yaml')`

### Zip/Tarball

1. Download the appropriate zip file or tar.gz file from the
[Github releases](https://github.com/viking/r-yaml/releases) page.
2. Run `R CMD INSTALL <filename>`

### Git (via devtools)

1. Install the `devtools` package from CRAN.
2. In R, run the following:
    ```R
    library(devtools)
    install_github('viking/r-yaml')
    ```

## Usage

The `yaml` package provides three functions: `yaml.load`, `yaml.load_file` and
`as.yaml`.

### yaml.load

`yaml.load` is the YAML parsing function. It accepts a YAML document as a
string. Here's a simple example that parses a YAML sequence:

    x <- "
    - 1
    - 2
    - 3
    "
    yaml.load(x)  #=> [1] 1 2 3

#### Scalars

A YAML scalar is the basic building block of YAML documents. Example of a YAML
document with one element:

    1.2345

In this case, the scalar "1.2345" is typed as a `float` (or numeric) by the
parser.  `yaml.load` would return a numeric vector of length 1 for this
document.

    yaml.load("1.2345")  #=> [1] 1.2345

#### Sequences

A YAML sequence is a list of elements. Here's an example of a simple YAML
sequence:

    - this
    - is
    - a
    - simple
    - sequence
    - of
    - scalars

If you pass a YAML sequence to `yaml.load`, a couple of things can happen. If
all of the elements in the sequence are uniform, `yaml.load` will return a
vector of that type (i.e. character, integer, real, or logical). If the
elements are _not_ uniform, `yaml.load` will return a list of the elements.

#### Maps

A YAML map is a list of paired keys and values, or hash, of elements. Here's
an example of a simple YAML map:

    one: 1
    two: 2
    three: 3
    four: 4

Passing a map to `yaml.load` will produce a named list by default. That is,
keys are coerced to strings. Since it is possible for the keys of a YAML map
to be almost anything (not just strings), you might not want `yaml.load` to
return a named list. If you want to preserve the data type of keys, you can
pass `as.named.list = FALSE` to `yaml.load`. If `as.named.list` is FALSE,
`yaml.load` will create a `keys` attribute for the list it returns instead of
coercing the keys into strings.

#### Handlers

`yaml.load` has the capability to accept custom handler functions. With
handlers, you can customize `yaml.load` to do almost anything you want.
Example of handler usage:

    integer.handler <- function(x) { as.integer(x) + 123 }
    yaml.load("123", handlers = list(int = integer.handler))  #=> [1] 246

Handlers are passed to `yaml.load` through the `handlers` argument. The
`handlers` argument must be a named list of functions, where each name is the
YAML type that you want to be handled by your function. The functions you
provide must accept one argument and must return an R object.

Handler functions will be passed a string or list, depending on the original
type of the object. In the example above, `integer.handler` was passed the
string "123".

##### Sequence handlers

Custom sequence handlers will be passed a list of objects. You can then
convert the list into whatever you want and return it. Example:

    sequence.handler <- function(x) {
      tmp <- as.numeric(x)
      tmp / 5
    }
    string <- "
    - foo
    - bar
    - 123
    - 4.567
    "
    yaml.load(string, handlers = list(seq = sequence.handler))  #=> [1]      NA      NA 24.6000  0.9134

##### Map handlers

Custom map handlers work much in the same way as custom list handlers. A map
handler function is passed a named list, or a list with a `keys` attribute
(depending on the value of `as.named.list`). Example:

    string <- "
    a:
    - 1
    - 2
    b:
    - 3
    - 4
    "
    yaml.load(string, handlers = list(map = function(x) { as.data.frame(x) }))

Returns:

      a b
    1 1 3
    2 2 4

### yaml.load_file

`yaml.load_file` does the same thing as `yaml.load`, except it reads a file
from a connection. For example:

    x <- yaml.load_file("Data/document.yml")

This function takes the same arguments as `yaml.load`, with the exception that
the first argument is a filename or a connection.

### read_yaml

The `read_yaml` function is a convenience function that works similarly to
functions in the [readr package](https://cran.r-project.org/package=readr). You
can use it instead of `yaml.load_file` if you prefer.

### as.yaml

`as.yaml` is used to convert R objects into YAML strings. Example `as.yaml`
usage:

    x <- as.yaml(1:5)
    cat(x, "\n")

Output from above example:

    - 1
    - 2
    - 3
    - 4
    - 5

#### Notable arguments

##### indent

You can control the number of spaces used to indent by setting the `indent`
option. By default, `indent` is 2.

For example:

    cat(as.yaml(list(foo = list(bar = 'baz')), indent = 3))

Outputs:

    foo:
       bar: baz

##### indent.mapping.sequence

By default, sequences that are within a mapping context are not indented.

For example:

    cat(as.yaml(list(foo = 1:10)))

Outputs:

    foo:
    - 1
    - 2
    - 3
    - 4
    - 5
    - 6
    - 7
    - 8
    - 9
    - 10

If you want sequences to be indented in this context, set the `indent.mapping.sequence` option to `TRUE`.

For example:

    cat(as.yaml(list(foo = 1:10), indent.mapping.sequence=TRUE))

Outputs:
```
foo:
  - 1
  - 2
  - 3
  - 4
  - 5
  - 6
  - 7
  - 8
  - 9
  - 10
```

##### column.major

The `column.major` option determines how a data frame is converted into YAML.
By default, `column.major` is TRUE.

Example of `as.yaml` when `column.major` is TRUE:

    x <- data.frame(a=1:5, b=6:10)
    y <- as.yaml(x, column.major = TRUE)
    cat(y, "\n")

Outputs:

    a:
    - 1
    - 2
    - 3
    - 4
    - 5
    b:
    - 6
    - 7
    - 8
    - 9
    - 10

Whereas:

    x <- data.frame(a=1:5, b=6:10)
    y <- as.yaml(x, column.major = FALSE)
    cat(y, "\n")

Outputs:

    - a: 1
      b: 6
    - a: 2
      b: 7
    - a: 3
      b: 8
    - a: 4
      b: 9
    - a: 5
      b: 10

##### handlers

You can specify custom handler functions via the `handlers` argument.
This argument must be a named list of functions, where the names are R object
class names (i.e., 'numeric', 'data.frame', 'list', etc).  The function(s) you
provide will be passed one argument (the R object) and can return any R object.
The returned object will be emitted normally.

#### Special features

##### Verbatim(-ish) text

Character vectors that have a class of `'verbatim'` will not be quoted in the
output YAML document except when the YAML specification requires it.  This
means that you cannot do anything that would result in an invalid YAML
document, but you can emit strings that would otherwise be quoted.  This is
useful for changing how logical vectors are emitted. For example:

```r
as.yaml(c(TRUE, FALSE), handlers = list(
  logical = function(x) {
    result <- ifelse(x, "true", "false")
    class(result) <- "verbatim"
    return(result)
  }
))
```

##### Quoted Strings

There are times you might need to ensure a string scalar is quoted.  Apply a
non-null attribute of "quoted" to the string you need quoted and it will come
out with double quotes around it.

```r
port_def <- "80:80"
attr(port_def, "quoted") <- TRUE
x <- list(ports = list(port_def))
as.yaml(x)
```

##### Custom tags

You can specify YAML tags for R objects by setting the `'tag'` attribute
to a character vector of length 1.  If you set a tag for a vector, the tag
will be applied to the YAML sequence as a whole, unless the vector has only 1
element.  If you wish to tag individual elements, you must use a list of
1-length vectors, each with a tag attribute.  Likewise, if you set a tag for
an object that would be emitted as a YAML mapping (like a data frame or a
named list), it will be applied to the mapping as a whole.  Tags can be used
in conjunction with YAML deserialization functions like
`yaml.load` via custom handlers, however, if you set an internal
tag on an incompatible data type (like `!seq 1.0`), errors will occur
when you try to deserialize the document.

### write_yaml

The `write_yaml` function is a convenience function that works similarly to
functions in the [readr package](https://cran.r-project.org/package=readr). It
calls `as.yaml` and writes the result to a file or a connection.

### Additional documentation

For more information, run `help(package='yaml')` or `example('yaml-package')`
for some examples.

## Development

There is a `Makefile` for use with
[GNU Make](https://www.gnu.org/software/make/) to help with development. There
are several `make` targets for building, debugging, and testing. You can run
these by executing `make <target-name>` if you have the `make` program
installed.

| Target name     | Description                                 |
| --------------- | ------------------------------------------- |
| `compile`       | Compile the source files                    |
| `check`         | Run CRAN checks                             |
| `gct-check`     | Run CRAN checks with gctorture              |
| `test`          | Run unit tests                              |
| `gdb-test`      | Run unit tests with gdb                     |
| `valgrind-test` | Run unit tests with valgrind                |
| `tarball`       | Create tarball suitable for CRAN submission |
| `all`           | Default target, runs `compile` and `test`   |

### Implicit tag discovery

The algorithm used whenever there is no YAML tag explicitly provided is located
in the [implicit.re](src/implicit.re) file. This file is used to create the
[implicit.c](src/implicit.c) file via the [re2c](http://re2c.org/) program. If
you want to change this algorithm, make your changes in `implicit.re`, not
`implicit.c`. The `make` targets will automatically update the C file as needed,
but you'll need to have the `re2c` program installed for it to work.

### VERSION file

The `VERSION` file is used to track the current version of the package.
Warnings are displayed if the `DESCRIPTION` and `CHANGELOG` files are not
properly updated when creating a tarball. This is to help prevent problems
during the CRAN submission process.
