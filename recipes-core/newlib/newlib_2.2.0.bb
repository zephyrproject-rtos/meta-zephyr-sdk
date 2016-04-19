HOMEPAGE = "https://sourceware.org/newlib/"
SUMMARY = "C library for embedded systems"
DESCRIPTION = "Newlib is a conglomeration of several library parts, all under free software licenses that make them easily usable on embedded products."

LICENSE = "GPLv2 & LGPLv3 & GPLv3 & LGPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552 \
                    file://COPYING3.LIB;md5=6a6a8e020838b23406c81b19c1d46df6 \
                    file://COPYING3;md5=d32239bcb673463ab874e80d47fae504 \
                    file://COPYING.LIBGLOSS;md5=d15bfccaf2c20f3287951bd3f768db5f \
                    file://COPYING.LIB;md5=9f604d8a4f8e74f4f5140845a21b6674 \
                    file://COPYING.NEWLIB;md5=27039641b800547bbcea82a8a5b707ad \
                    file://newlib/libc/posix/COPYRIGHT;md5=103468ff1982be840fdf4ee9f8b51bbf \
                    file://newlib/libc/sys/linux/linuxthreads/LICENSE;md5=73640207fbc79b198c7ffd4ad4d97aa0"

SRC_URI = "file://newlib-2.2.0.tar.gz"
S = "${WORKDIR}/newlib-2.2.0"

DEPENDS = "flex-native bison-native m4-native"
DEPENDS_remove = "virtual/libc virtual/${TARGET_PREFIX}compilerlibs"
PACKAGES = "${PN}"

# x86 specific settings
#NEWLIB_HOST_x86 ?= "i586-poky-elf"
TUNE_CCARGS_x86 := " -nostdlib"

# MIPS specific settings
#NEWLIB_HOST_mips ?= "mips-poky-elf"
#TUNE_CCARGS_mips := "-mabi=32 -march=mips32r2 -nostdlib"
TUNE_CCARGS_mips := "-nostdlib"

# ARM specific settings
#TUNE_CCARGS_arm := "-mthumb -mcpu=cortex-m3 -march=armv7-m -nostdlib"
#NEWLIB_HOST_arm ?= "armv5-poky-eabi"
TUNE_CCARGS_arm := "-nostdlib"

# NIOS2 specific settings
TUNE_CCARGS_nios2 := " -nostdlib"

# This will determine the name of the folder with libc as well.
NEWLIB_HOST = "${TARGET_SYS}"

CFLAGS += " -DMISSING_SYSCALL_NAMES "

# Specify any options you want to pass to the configure script using EXTRA_OECONF:
EXTRA_OECONF = " --enable-languages=c \
    --host=${NEWLIB_HOST} \
    --with-newlib --with-gnu-as --with-gnu-ld -v \
"

do_configure () {
    # If we're being rebuilt due to a dependency change, we need to make sure
    # everything is clean before we configure and build -- if we haven't previously
    # built this will fail and be ignored.
    make distclean || :
    export CC_FOR_TARGET="${CC}"
    ${S}/configure ${EXTRA_OECONF}
}

do_install () {
    oe_runmake 'DESTDIR=${D}' install

    # Delete standards.info, configure.info
    rm -rf ${D}/usr/share/
}

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

FILES_${PN} = "/usr/${NEWLIB_HOST}"

INSANE_SKIP_${PN} += " staticdev"
INSANE_SKIP_${PN}-dev += " staticdev"

