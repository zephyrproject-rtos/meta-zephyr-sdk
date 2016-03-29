require recipes-devtools-arc/gcc/gcc-4.8arc.inc
require recipes-devtools/gcc/libgcc.inc

#RDEPENDS_${PN}-dev_libc-baremetal = ""

#RDEPENDS_${PN}-dev_libc-baremetal = "libgcc-dev"

#FILES_${PN}_append_libc-baremetal = "\
#	/usr/lib/arc-poky-elf/${BINV}/norm/* \
#	/usr/lib/arc-poky-elf/${BINV}/arc600/* \
#	/usr/lib/arc-poky-elf/${BINV}/arc601/* \
#	/usr/lib/arc-poky-elf/${BINV}/em/* \
#	/usr/lib/arc-poky-elf/${BINV}/hs/* \
#"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
COMPATIBLE_MACHINE = "arc"

#EXTRA_OECONF_append_arc = " --disable-multilib"
EXTRA_OECONF_append_arc = " --enable-multilib"

# ERROR: QA Issue: Architecture did not match (195 to 93) 
INSANE_SKIP_${PN}-dev += "arch"

# ERROR: QA Issue: non -staticdev package contains static .a library
INSANE_SKIP_${PN}-dev += "staticdev"
