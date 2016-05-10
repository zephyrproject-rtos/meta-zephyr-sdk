
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "libusb-compat hidapi-libusb"
RDEPENDS_${PN} = "libusb1"

# The various arc files are based on the commit e781e73a39bc5c845b73dc96b751d867278a7583
# of https://github.com/foss-for-synopsys-dwc-arc-processors/openocd

SRC_URI = " \
	git://git.code.sf.net/p/openocd/code;tag=v0.9.0 \
	file://ISSM_QuarkSE.patch \
	file://d089da2b.patch \
	file://Makefile_am.patch \
	file://target_c.patch \
	file://x86_32_common_c.patch \
	file://x86_32_common_h.patch \
	file://lakemont_c.patch \
	file://lakemont_h.patch \
	file://zephyr-sdk-09.patch \
	file://arc_jtag.c \
	file://arc_jtag.h \
	file://arc32.c \
	file://arc32.h \
	file://arc_mem.c \
	file://arc_mem.h \
	file://arc_mntr.c \
	file://arc_mntr.h \
	file://arc_regs.c \
	file://arc_regs.h \
	file://arc_ocd.c \
	file://arc_ocd.h \
	file://arc_dbg.c \
	file://arc_dbg.h \
	file://arc_quark.c \
	file://quark_se.c \
	file://hs.tcl \
	file://em.tcl \
	file://common.tcl \
	file://v2.tcl \
	file://arc32.tcl \
	file://arcompact.tcl \
	"

S = "${WORKDIR}/git"

inherit pkgconfig autotools gettext

BBCLASSEXTEND += "nativesdk"

EXTRA_OECONF = "--enable-ftdi --enable-cmsis-dap --enable-jlink --enable-stlink --disable-doxygen-html "

do_configure() {
    cp ${WORKDIR}/arc32.* ${S}/src/target
    cp ${WORKDIR}/quark_se.c ${S}/src/target
    cp ${WORKDIR}/arc_*.* ${S}/src/target
    cp ${WORKDIR}/*.tcl ${S}/tcl/cpu/arc
    cd ${S} 
    export ALL_PROXY="${ALL_PROXY}"
    export GIT_PROXY_COMMAND=${GIT_PROXY_COMMAND}
    ./bootstrap
    oe_runconf ${EXTRA_OECONF}
}

do_install() {
    cd ${S}
    oe_runmake DESTDIR=${D} install
    if [ -e "${D}${infodir}" ]; then
      rm -Rf ${D}${infodir}
    fi
    if [ -e "${D}${mandir}" ]; then
      rm -Rf ${D}${mandir}
    fi
    if [ -e "${D}${bindir}/.debug" ]; then
      rm -Rf ${D}${bindir}/.debug
    fi
}

