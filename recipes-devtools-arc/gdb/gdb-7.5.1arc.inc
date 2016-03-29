LICENSE = "GPLv2 & GPLv3 & LGPLv2 & LGPLv3"
LIC_FILES_CHKSUM = "file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552 \
		    file://COPYING3;md5=d32239bcb673463ab874e80d47fae504 \
		    file://COPYING3.LIB;md5=6a6a8e020838b23406c81b19c1d46df6 \
		    file://COPYING.LIB;md5=9f604d8a4f8e74f4f5140845a21b6674"

SRC_URI[md5sum] = "8f8ced422fe462a00e0135a643544f17"
SRC_URI[sha256sum] = "d282508cb7df0cb8b2cf659032ce1bede7b5725796e3ac90f3cd9d65844a65f2"

PV = "7.5.1+gitarc"

SRCREV = "4260149b11beba5da09c1610e284e1634279238c"
SRC_URI = "git://github.com/foss-for-synopsys-dwc-arc-processors/binutils-gdb.git;branch=arc-7.5-dev \
           file://docfix.patch"

S = "${WORKDIR}/git"
#EXTRA_OECONF_remove_arc = "--disable-sim"
EXTRA_OECONF_remove_arc = "--disable-multilib"
#EXTRA_OECONF_append_arc = " --enable-sim "
