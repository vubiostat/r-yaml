lib.loc <- "build/lib"
destdir <- "build/pkgs"

library(yaml, lib.loc = lib.loc)
options(repos = 'https://cran.r-project.org')

# many reverse dependencies depend on bioconductor packages
if (!requireNamespace("BiocManager"))
  install.packages("BiocManager")

all_cran <- sort(available.packages()[, "Package"])
all_bioc <- sort(BiocManager::available())

# discover and install any nested bioconductor dependencies
checked <- c("yaml")
bioc <- c()
rev_deps <- sort(tools::package_dependencies("yaml", reverse = TRUE)$yaml)
pkgs <- rev_deps
while (length(pkgs) > 0) {
  checked <- c(checked, pkgs)
  cran <- intersect(all_cran, pkgs)
  non_cran <- setdiff(pkgs, all_cran)
  bioc <- c(bioc, intersect(all_bioc, non_cran))
  unknown <- setdiff(non_cran, all_bioc)
  if (length(unknown) > 0) {
    cat("unknown (probably base packages): ", paste(unknown, collapse=", "), "\n", sep="")
  }

  deps <- unique(sort(unlist(tools::package_dependencies(cran))))
  pkgs <- unique(sort(setdiff(deps, checked)))
  cat("checked: ", length(checked), " pkgs: ", length(pkgs), " bioc: ", length(bioc), "\n", sep="")
}

if (length(bioc) > 0) {
  BiocManager::install(bioc)
}

dir.create(destdir, mode = "755")
install.packages(rev_deps, lib = lib.loc, destdir = destdir)
