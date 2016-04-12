SUMMARY = "BOSSA is a flash programming utility for Atmel's SAM family of flash-based ARM microcontrollers."
HOMEPAGE = "https://github.com/femtoio/BOSSA"

#
# Note: In order to build this recipe, you need wxWidgets libraries :
#     sudo apt-get install wx2.8-headers libwxgtk2.8-0 libwxgtk2.8-dev
#

LICENSE = "SHUMATECH"
LIC_FILES_CHKSUM = "file://LICENSE;md5=94411054a7f6921218ab9c05b6b4b15b"

DEPENDS = "readline"

PR = "r0"
SRCREV = "ce82a9fac4b916e359fb003ee4d19afc0ff01c90"
SRC_URI = "git://github.com/femtoio/BOSSA.git;protocol=https"

S = "${WORKDIR}/git"

do_compile() {
	oe_runmake bin/bossac bin/bossash
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${S}/bin/bossac ${D}${bindir}
	install -m 0755 ${S}/bin/bossash ${D}${bindir}
}

FILES_${PN} = "${bindir} "

BBCLASSEXTEND += "nativesdk"

