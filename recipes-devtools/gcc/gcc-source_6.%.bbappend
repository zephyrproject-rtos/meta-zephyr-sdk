FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://0043-arm-multilib.patch"
SRC_URI += "file://nios2-multilib.patch"

EXTRA_OECONF_append = " --enable-plugin "
