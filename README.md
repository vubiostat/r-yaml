R YAML package
==============

The R [YAML](http://yaml.org) package implements the
[libyaml](http://pyyaml.org/wiki/LibYAML) YAML parser and emitter for R.

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

1. Download the appropriate zip file or tar.gz file from Github
2. Unzip the file and change directories into the yaml directory
2. Run `R CMD INSTALL pkg`

### Git

1. Download the source via git: `git clone https://github.com/viking/r-yaml.git yaml`
2. (optional) Run `R CMD check yaml/pkg` to make sure everything is OK.
3. Run `R CMD INSTALL yaml/pkg` (as root if necessary).

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

### Additional documentation

For more information, run `help(package='yaml')` or `example('yaml-package')`
for some examples.
