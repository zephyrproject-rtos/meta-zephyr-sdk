require recipes-devtools-xtensa/overlay.inc

# Copy gdb overlay files into the source tree before applying patches.

do_overlay() {
	cp -a ${WORKDIR}/gdb/* ${S}
}

addtask do_overlay after do_unpack before do_patch
