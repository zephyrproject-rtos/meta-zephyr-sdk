SRCREV = "2e930b7f8f6421638869a04b00297034c42e1a82"
PV = "1.4.7+git"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_install () {
        oe_runmake install NO_PYTHON=1
}

FILES_${PN}-misc_append = "${bindir}/fdtoverlay ${bindir}/fdtput ${bindir}/fdtget"
