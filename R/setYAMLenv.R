setYAMLenv <- function(where = topenv(parent.frame()))
    .Call("setYAMLenv", as.environment(where), PACKAGE = "yaml")
