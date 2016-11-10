########################################################################
#
# ARC specific
#
########################################################################
LIC_FILES_CHKSUM_arc = "file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552 \
                    file://COPYING3.LIB;md5=6a6a8e020838b23406c81b19c1d46df6 \
                    file://COPYING3;md5=d32239bcb673463ab874e80d47fae504 \
                    file://COPYING.LIB;md5=9f604d8a4f8e74f4f5140845a21b6674 \
                    file://COPYING.NEWLIB;md5=fced02ba02d66f274d4847d27e80af74 \
                    file://newlib/libc/posix/COPYRIGHT;md5=103468ff1982be840fdf4ee9f8b51bbf \
                    file://newlib/libc/sys/linux/linuxthreads/LICENSE;md5=73640207fbc79b198c7ffd4ad4d97aa0"

#Tag arc-2016.03
SRCREV_arc = "e4da0f88abe8dd2a0b947bcf7cb8b3736ab94f33"
SRC_URI_arc = "git://github.com/foss-for-synopsys-dwc-arc-processors/newlib.git;branch=arc-2.3"
SRC_URI_arc += "file://assert-fiprintf.patch"
S_arc  = "${WORKDIR}/git"

EXTRA_OECONF_append_arc = " --enable-multilib "
TUNE_CCARGS_arc := " -nostdlib -mno-sdata "

# ERROR: QA Issue: Architecture did not match (195 to 93)
INSANE_SKIP_${PN}_arc += " arch "
INSANE_SKIP_${PN}_arc += " staticdev "

########################################################################
#
# IAMCU specific
#
########################################################################
FILESEXTRAPATHS_prepend_iamcu := "${THISDIR}/${PN}:"
LIC_FILES_CHKSUM_iamcu = " "

SRC_URI_iamcu = "file://iamcu_newlib_220-01638bd.tar.7z"
S_iamcu = "${WORKDIR}/iamcu_newlib_220-01638bd"
PATH_prepend_iamcu = "${STAGING_DIR_NATIVE}${bindir_native}/i586${TARGET_VENDOR}-${TARGET_OS}:"

# IAMCU specific settings
TUNE_CCARGS_iamcu := " -nostdlib -miamcu -m32"

EXTRA_OECONF_append_iamcu = " \
	--disable-multilib --enable-newlib-global-atexit \
	--disable-newlib-fvwrite-in-streamio \
	--disable-newlib-unbuf-stream-opt --enable-lite-exit \
	"

