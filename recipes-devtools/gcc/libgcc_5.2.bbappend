
#ERROR: QA Issue: Endiannes did not match (0 to 1) 
INSANE_SKIP_${PN}-dev_libc-baremetal_mips += "arch"

#ERROR: QA Issue: non -staticdev package contains static .a library
INSANE_SKIP_${PN}-dev_libc-baremetal_mips += "staticdev"
