.onLoad <- function(libname, pkgName) {
  env <- environment(sys.function())
  setYAMLenv(env)
}

.onUnload <- function(libpath) {
  library.dynam.unload("yaml", libpath)
}
