SRCREV = "e54388015af1fb4bf04d0bca99caba1074d9cc42"
PV = "1.4.6"

do_install () {
        oe_runmake install NO_PYTHON=1
}

FILES_${PN}-misc_append = "${bindir}/fdtoverlay ${bindir}/fdtput ${bindir}/fdtget"
