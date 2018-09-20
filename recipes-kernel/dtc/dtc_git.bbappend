SRCREV = "1e4a0928f3b3b827824222572e551a60935607e3"
PV = "1.4.7+git"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_install () {
        oe_runmake install NO_PYTHON=1
}

FILES_${PN}-misc_append = "${bindir}/fdtoverlay ${bindir}/fdtput ${bindir}/fdtget"
