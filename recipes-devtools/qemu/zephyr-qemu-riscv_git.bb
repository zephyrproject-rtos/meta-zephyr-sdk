
DEPENDS = "glib-2.0 zlib pixman gnutls dtc"
LICENSE = "GPLv2"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
LIC_FILES_CHKSUM = "file://COPYING;md5=441c28d2cf86e15a37fa47e15a72fbac \
                    file://COPYING.LIB;endline=24;md5=c04def7ae38850e7d3ef548588159913"

SRCREV = "b12cfa344681687c7adbc5fa66590182df3748b9"
SRC_URI = "git://github.com/riscv/riscv-qemu.git;protocol=https;nobranch=1"

BBCLASSEXTEND = "native nativesdk"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

S = "${WORKDIR}/git"

inherit autotools pkgconfig


QEMUS_BUILT = "riscv32-softmmu "
QEMU_FLAGS = "--disable-docs  --disable-sdl --disable-debug-info  --disable-cap-ng \
  --disable-libnfs --disable-libusb --disable-libiscsi --disable-usb-redir --disable-linux-aio\
  --disable-guest-agent --disable-libssh2 --disable-vnc-png  --disable-seccomp \
  --disable-uuid --disable-tpm  --disable-numa --disable-glusterfs \
  --disable-virtfs --disable-xen --disable-curl --disable-attr --disable-curses\
  "

do_configure() {
    ${S}/configure ${QEMU_FLAGS} --target-list="${QEMUS_BUILT}" --prefix=${prefix}  \
        --sysconfdir=${sysconfdir} --libexecdir=${libexecdir} --localstatedir=${localstatedir}
}

do_install_append() {
    # Remove additional files...
    find  ${D}/${SDKPATHNATIVE}/usr/share/qemu -type f -exec rm -f {} +

    # Remove files installed by zephyr-qemu
    rm  ${D}/${SDKPATHNATIVE}/usr/libexec/qemu-bridge-helper
    rm  ${D}/${SDKPATHNATIVE}/usr/bin/qemu-nbd
    rm  ${D}/${SDKPATHNATIVE}/usr/bin/ivshmem-client
    rm  ${D}/${SDKPATHNATIVE}/usr/bin/qemu-img
    rm  ${D}/${SDKPATHNATIVE}/usr/bin/qemu-io
    rm  ${D}/${SDKPATHNATIVE}/usr/bin/ivshmem-server
}

FILES_${PN} = " \
   /opt/zephyr-sdk \
  "

INSANE_SKIP_${PN} = "already-stripped"



