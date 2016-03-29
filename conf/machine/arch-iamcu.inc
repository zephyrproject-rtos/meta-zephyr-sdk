DEFAULTTUNE ?= "iamcu"

TARGET_OS_iamcu = "elfiamcu"
TARGET_VENDOR_iamcu = "-poky"
TARGET_SYS_iamcu = "i586-poky-elfiamcu"

TUNE_ARCH = "iamcu"
TUNE_PKGARCH = "iamcu"
TUNE_FEATURES_tune-iamcu = "iamcu"
TUNE_CCARGS = "-miamcu -m32"

TUNEVALID[iamcu] = "Intel MCU"
