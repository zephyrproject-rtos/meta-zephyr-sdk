do_install_append_libc-baremetal_mipsXXX () {
	# There is some confusion regarding endian validation, so delete all "el"
	# libraries/startup files.
	# The actual error is:
	# mips-poky-elf-objdump: <...>tmpp/work/mips32r2-poky-elf/libgcc/4.9.3-r0/packages-split/libgcc-dev/usr/lib/mips-poky-elf/4.9.3/soft-float/el/crtbegin.o: 
	# File format is ambiguous 
	# mips-poky-elf-objdump: Matching formats: elf32-littlemips elf32-tradlittlemips

	rm -R ${D}${libdir}/${TARGET_SYS}/${BINV}/el 
	rm -R ${D}${libdir}/${TARGET_SYS}/${BINV}/soft-float/el 
}

#ERROR: QA Issue: Endiannes did not match (0 to 1) 
INSANE_SKIP_${PN}-dev_libc-baremetal_mips += "arch"

#ERROR: QA Issue: non -staticdev package contains static .a library
INSANE_SKIP_${PN}-dev_libc-baremetal_mips += "staticdev"
