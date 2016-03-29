# Exclude GDB, it is built as a part of binutils.

RDEPENDS_${PN}_iamcu = "\
    binutils-cross-canadian-${TRANSLATED_TARGET_ARCH} \
    gcc-cross-canadian-${TRANSLATED_TARGET_ARCH} \
    meta-environment-${MACHINE} \
    "
