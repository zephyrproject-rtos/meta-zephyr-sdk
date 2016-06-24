
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "libusb-compat"
RDEPENDS_${PN} = "libusb1"

SRC_URI = " \
    git://repo.or.cz/openocd.git \
	file://ISSM_QuarkSE.patch \
	file://zephyr-sdk.patch \
	"

SRCREV = "94d64ccaebd3df17f5873c076fc08ca97088cb1e"
S = "${WORKDIR}/git"

inherit pkgconfig autotools gettext

BBCLASSEXTEND += "nativesdk"

EXTRA_OECONF = "--enable-ftdi --disable-doxygen-html --datadir=${datadir}/openocd-legacy"

do_configure() {
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
    mv ${D}${bindir}/openocd ${D}${bindir}/openocd-legacy
    mv ${D}${datadir}/openocd-legacy/openocd/* ${D}${datadir}/openocd-legacy
    rm -rf ${D}${datadir}/openocd-legacy/openocd
}

FILES_${PN} = " \
   /opt/zephyr-sdk \
  "
