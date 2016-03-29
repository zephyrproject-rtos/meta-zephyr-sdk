DEFAULTTUNE ?= "arc"

TUNE_ARCH = "arc"
TUNE_PKGARCH = "arc"
TUNE_FEATURES_tune-arc = "littleendian"
#This does not work for multilib
#TUNE_CCARGS = "-mARCv2EM -mav2em"
TUNE_CCARGS = ""

#TUNEVALID[bigendian] = "Big endian mode"
TUNEVALID[littleendian] = "Little endian mode"
#TUNE_CCARGS .= "${@bb.utils.contains("TUNE_FEATURES", "bigendian", " -EB", " -EL", d)}"
#TUNE_ARCH = "${@bb.utils.contains("TUNE_FEATURES", "bigendian", "arceb", "arc", d)}"
