as.yaml <-
function(x, ...) {
  UseMethod("as.yaml", x)
}

as.yaml.list <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, omap = FALSE, ...) {
  line.sep <- match.arg(line.sep)
  doc <- .as.yaml.internal.list(x, line.sep, indent, pre.indent, omap, ...)
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
  .as.yaml.internal.data.frame(x, line.sep, indent, pre.indent, column.major, ...)
}

as.yaml.default <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, ...) {
  line.sep <- match.arg(line.sep)
  .as.yaml.internal.default(x, line.sep, indent, pre.indent, ...)
}

.as.yaml.internal <-
function(x, ...) {
  UseMethod(".as.yaml.internal", x)
}

.as.yaml.internal.list <-
function(x, line.sep, indent, pre.indent, omap = FALSE, ...) {
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep="")
  if (length(x) == 0) {
    return(paste(pre.indent.str, "[]", sep=""))
  }

  x.names <- names(x)
  is.map <- (!is.null(x.names) && length(x.names) == length(x))
  if (!is.map) {
    stop("can only handle fully-named lists (length(names(x)) must equal names(x))")
  }

  omap.str <- ifelse(omap, "- ", "");

  retval <- vector("list", length(x))

  for (i in 1:length(x.names)) {
    tmp.pre.indent <- pre.indent + ifelse(omap, 2, 1)
    tmp <- .as.yaml.internal(x[[i]], line.sep = line.sep, indent = indent, pre.indent = tmp.pre.indent, omap = omap, ...)

    # this is wasteful
    len <- length(strsplit(tmp, line.sep)[[1]])

    if (len == 1 && !is.list(x[[i]])) {
      retval[[i]] <- paste(pre.indent.str, omap.str, x.names[i], ": ", tmp[[1]], sep = "")
    }
    else {
      retval[[i]] <- paste(pre.indent.str, omap.str, x.names[i], ":", line.sep, tmp, sep = "")
    }
  }

  paste(retval, collapse = line.sep)
}

.as.yaml.internal.data.frame <-
function(x, line.sep, indent, pre.indent, column.major, ...) {
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep = "")
  if (nrow(x) == 0) {
    return(paste(pre.indent.str, "[]", sep=""))
  }

  x.names <- names(x)
  retval  <- vector("list", ifelse(column.major, ncol(x), nrow(x)))
  if (column.major) {
    for (i in 1:ncol(x)) {
      tmp <- .as.yaml.internal.default(x[[i]], line.sep, indent, pre.indent + 1, ...)
      retval[[i]] <- paste(pre.indent.str, x.names[i], ":", line.sep, tmp, sep = "")
    }
  }
  else {
    for (i in 1:nrow(x)) {
      tmp <- .as.yaml.internal.list(as.list(x[i,]), line.sep, indent, pre.indent + 1, ...)

      # strip off leading indent and add a hyphen
      retval[[i]] <- paste(pre.indent.str, sub("^\\s+", "- ", tmp), sep = "")
    }
  }

  paste(retval, collapse = line.sep)
}

.as.yaml.internal.default <-
function(x, line.sep, indent, pre.indent, ...) {
  if (length(x) == 0) {
    return("[]")
  }

  retval <- vector("list", length(x))
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse = "", sep = "")

  if (length(x) == 1) {
    retval[[1]] <- as.character(x)
  }
  else {
    for (i in 1:length(x)) {
      retval[[i]] <- paste(pre.indent.str, "- ", x[[i]], sep = "")
    }
  }

  paste(retval, collapse = line.sep)
}
