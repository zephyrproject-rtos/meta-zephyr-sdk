require recipes-devtools/binutils/binutils.inc
require recipes-devtools-iamcu/binutils/binutils-i2.25iamcu.inc
require recipes-devtools/binutils/binutils-cross-canadian.inc

do_configure () {
	oe_runconf
}

do_install_append () {
	rm -f ${D}/${libdir}/../lib/libiberty*
}
