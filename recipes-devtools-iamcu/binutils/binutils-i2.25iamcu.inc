LIC_FILES_CHKSUM = "\
    file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552\
    file://COPYING.LIB;md5=9f604d8a4f8e74f4f5140845a21b6674\
    file://COPYING3;md5=d32239bcb673463ab874e80d47fae504\
    file://COPYING3.LIB;md5=6a6a8e020838b23406c81b19c1d46df6\
    file://gas/COPYING;md5=d32239bcb673463ab874e80d47fae504\
    file://include/COPYING;md5=59530bdf33659b29e73d4adb9f9f6552\
    file://include/COPYING3;md5=d32239bcb673463ab874e80d47fae504\
    file://libiberty/COPYING.LIB;md5=a916467b91076e631dd8edb7424769c7\
    file://bfd/COPYING;md5=d32239bcb673463ab874e80d47fae504\
    "

#def binutils_branch_version(d):
#    pvsplit = d.getVar('PV', True).split('.')
#    return pvsplit[0] + "_" + pvsplit[1]

#BINUPV = "${@binutils_branch_version(d)}"

PV = "i2.25+gitiamcu"

SRC_URI = "\
     file://iamcu-binutils-225-7018c03.tar.7z \
     file://add-arch-iamcu.patch \
     "
S  = "${WORKDIR}/iamcu-binutils-225-7018c03"

EXTRA_OECONF += " --with-python=no"

#do_configure_prepend () {
#        rm -rf ${S}/gdb ${S}/libdecnumber ${S}/readline ${S}/sim
#}

#do_configure_iamcu_prepend () {
#        rm -rf ${S}/sim
#}

CONFIG_SHELL = "/bin/bash"

sysroot_stage_all_append() {
	sysroot_stage_dir ${D}${exec_prefix}/extlib ${SYSROOT_DESTDIR}${exec_prefix}/extlib
}

#INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
#INHIBIT_PACKAGE_STRIP = "1"

