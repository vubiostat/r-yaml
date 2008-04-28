as.yaml <- 
function(x, ...) {
  UseMethod("as.yaml", x)
}

as.yaml.list <- 
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, ...) {
  if (length(x) == 0)  return("[]")
  x.names <- names(x)
  is.map <- (!is.null(x.names) && length(x.names) == length(x))
  if (!is.map) {
    stop("can only handle fully-named lists (length(names(x)) must equal names(x))")
  }
  line.sep <- match.arg(line.sep)

  retval <- vector("list", length(x))
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep="")

  for (i in 1:length(x.names)) {
    tmp <- as.yaml(x[[i]], line.sep = line.sep, indent = indent, pre.indent = pre.indent + 1)

    # this is wasteful
    len <- length(strsplit(tmp, line.sep)[[1]])

    if (len == 1) 
      retval[[i]] <- paste(pre.indent.str, x.names[i], ": ", tmp[[1]], sep = "")
    else 
      retval[[i]] <- paste(pre.indent.str, x.names[i], ":", line.sep, tmp, sep = "")
  }

  paste(retval, collapse = line.sep)
}

as.yaml.data.frame <-
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, column.major = TRUE, ...) {
  if (nrow(x) == 0)  return("[]")
  x.names <- names(x)
  retval  <- vector("list", ifelse(column.major, ncol(x), nrow(x)))
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse="", sep = "")
  line.sep <- match.arg(line.sep)
  if (column.major) {
    for (i in 1:ncol(x)) {
      tmp <- as.yaml.default(x[[i]], line.sep, indent, pre.indent + 1)
      retval[[i]] <- paste(pre.indent.str, x.names[i], ":", line.sep, tmp, sep = "")
    }
  }
  else {
    for (i in 1:nrow(x)) {
      tmp <- as.yaml.list(as.list(x[i,]), line.sep, indent, pre.indent + 1)

      # strip off leading indent and add a hyphen
      retval[[i]] <- paste(pre.indent.str, sub("^\\s+", "- ", tmp), sep = "")
    }
  }

  paste(retval, collapse = line.sep)
}

as.yaml.default <- 
function(x, line.sep = c("\n", "\r\n"), indent = 2, pre.indent = 0, ...) {
  if (length(x) == 0)  return("[]")
  retval <- vector("list", length(x))
  pre.indent.str <- paste(rep(" ", pre.indent * indent), collapse = "", sep = "")
  line.sep <- match.arg(line.sep)

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
