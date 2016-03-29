require recipes-devtools-iamcu/gcc/gcc-4.sc.iamcu.inc
require recipes-devtools/gcc/libgcc.inc

#RDEPENDS_${PN}-dev_libc-baremetal = ""

COMPATIBLE_MACHINE = "iamcu"
EXTRA_OECONF_append_iamcu = " --host=i586-poky-elfiamcu"
EXTRA_OECONF_remove_iamcu = "--enable-libstdcxx-pch"
EXTRA_OECONF_remove_iamcu = "--enable-target-optspace"
EXTRA_OECONF_remove_iamcu = "--disable-nls"
EXTRA_OECONF_remove_iamcu = "--with-demangler-in-ld"
EXTRA_OECONF_remove_iamcu = "--disable-libquadmath"
EXTRA_OECONF_remove_iamcu = "--disable-multilib" 
EXTRA_OECONF_remove_iamcu = "--disable-libitm"
EXTRA_OECONF_remove_iamcu = "--disable-libatomic"
EXTRA_OECONF_remove_iamcu = "--disable-libs"
EXTRA_OECONF_remove_iamcu = "--disable-libssp"
CONFIGUREOPTS_remove_iamcu = "--disable-silent-rules"
CONFIGUREOPTS_remove_iamcu = "--disable-dependency-tracking"

#configure:5925: WARNING: unrecognized options: --with-libtool-sysroot, --enable-languages

