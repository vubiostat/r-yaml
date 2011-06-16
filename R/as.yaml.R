as.yaml <-
function(x, ...) {
  UseMethod("as.yaml", x)
}

as.yaml.list <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, omap = FALSE, ...) {
  line.sep <- match.arg(line.sep)
  result <- .as.yaml.internal.list(x, line.sep, indent, pre.indent, omap, ...)
  doc <- result[['string']]
  if (omap) {
    paste("--- !omap", doc, sep=line.sep)
  }
  else {
    doc
  }
}

as.yaml.data.frame <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, column.major = TRUE, ...) {
  line.sep <- match.arg(line.sep)
  result <- .as.yaml.internal(x, line.sep, indent, pre.indent, column.major, ...)
  result[['string']]
}

as.yaml.default <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, ...) {
  line.sep <- match.arg(line.sep)
  result <- .as.yaml.internal(x, line.sep, indent, pre.indent, ...)
  result[['string']]
}

.as.yaml.internal <-
function(x, ...) {
  UseMethod(".as.yaml.internal", x)
}

.as.yaml.internal.list <-
function(x, line.sep, indent, pre.indent, omap = FALSE, ...) {
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep="")
  if (length(x) == 0) {
    return(c(string = paste(pre.indent.str, "[]", sep=""), length = 1))
  }

  retval <- vector("list", length(x))
  x.names <- names(x)
  if (is.null(x.names)) {
    # sequence
    for (i in 1:length(x)) {
      tmp.pre.indent <- pre.indent + 1
      tmp <- .as.yaml.internal(x[[i]], line.sep = line.sep, indent = indent, pre.indent = tmp.pre.indent, omap = omap, ...)

      if (tmp['length'] == 1 && !is.list(x[[i]])) {
        retval[[i]] <- paste(pre.indent.str, "- ", tmp['string'][[1]], sep = "")
      }
      else {
        retval[[i]] <- paste(pre.indent.str, "-", line.sep, tmp['string'], sep = "")
      }
    }
  }
  else if (length(x.names) == length(x)) {
    # map
    omap.str <- ifelse(omap, "- ", "");

    for (i in 1:length(x.names)) {
      omap.tag <- ifelse(omap && is.list(x[[i]]), " !omap", "");

      tmp.pre.indent <- pre.indent + ifelse(omap, 2, 1)
      tmp <- .as.yaml.internal(x[[i]], line.sep = line.sep, indent = indent, pre.indent = tmp.pre.indent, omap = omap, ...)

      if (tmp['length'] == 1 && !is.list(x[[i]])) {
        retval[[i]] <- paste(pre.indent.str, omap.str, x.names[i], ": ", tmp['string'][[1]], sep = "")
      }
      else {
        retval[[i]] <- paste(pre.indent.str, omap.str, x.names[i], ":", omap.tag, line.sep, tmp['string'], sep = "")
      }
    }
  }
  else {
    stop("list must either be fully-named (length(names(x)) == length(x)) or have no names")
  }

  c(string = paste(retval, collapse = line.sep), length = length(x))
}

.as.yaml.internal.data.frame <-
function(x, line.sep, indent, pre.indent, column.major, ...) {
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep = "")
  if (nrow(x) == 0) {
    return(c(string = paste(pre.indent.str, "[]", sep=""), length = 1))
  }

  x.names <- names(x)
  retval  <- vector("list", if (column.major) ncol(x) else nrow(x))
  rlength <- NULL
  if (column.major) {
    for (i in 1:ncol(x)) {
      tmp <- .as.yaml.internal(x[[i]], line.sep, indent, pre.indent + 1, ...)
      retval[[i]] <- paste(pre.indent.str, x.names[i], ":", line.sep, tmp['string'], sep = "")
    }
    rlength <- ncol(x)
  }
  else {
    for (i in 1:nrow(x)) {
      tmp <- .as.yaml.internal.list(as.list(x[i,]), line.sep, indent, pre.indent + 1, ...)

      # strip off leading indent and add a hyphen
      retval[[i]] <- paste(pre.indent.str, sub("^\\s+", "- ", tmp['string']), sep = "")
    }
    rlength <- nrow(x)
  }

  c(string = paste(retval, collapse = line.sep), length = rlength)
}

.as.yaml.internal.function <-
function(x, line.sep, indent, pre.indent, ...) {
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse = "", sep = "")
  result <- paste(pre.indent.str, "!expr ", .format.obj(paste(deparse(x), collapse="\n"), pre.indent.str, indent), sep = "")
  c(string = result, length = 1)
}

.as.yaml.internal.default <-
function(x, line.sep, indent, pre.indent, ...) {
  if (length(x) == 0) {
    return(c(string="[]", length = 1))
  }

  retval <- vector("list", length(x))
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse = "", sep = "")

  if (length(x) == 1) {
    retval[[1]] <- .format.obj(x, pre.indent.str, indent)
  }
  else {
    for (i in 1:length(x)) {
      retval[[i]] <- paste(pre.indent.str, "- ", .format.obj(x[[i]], pre.indent.str, indent), sep = "")
    }
  }

  c(string = paste(retval, collapse = line.sep), length = length(x))
}

.format.obj <-
function(obj, ...) {
  UseMethod(".format.obj", obj)
}

.format.obj.factor <-
function(obj, ...) {
  .format.obj.character(as.character(obj), ...)
}

.format.obj.character <-
function(obj, pre.indent.str, indent, ...) {
  if (regexpr("\n", obj)[1] >= 0) {
    # multiline string
    indent.str <- paste(pre.indent.str, paste(rep(" ", indent), collapse = ""), sep = "")
    paste("|\n", indent.str, gsub("\n", paste("\n", indent.str, sep = ""), obj), sep="")
  }
  else {
    obj
  }
}

.format.obj.default <-
function(obj, ...) {
  format(obj, nsmall=1)
}
