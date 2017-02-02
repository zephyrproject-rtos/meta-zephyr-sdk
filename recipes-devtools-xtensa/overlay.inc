# Xtensa configuration overlay is an archive of source files specific for the
# generated CPU with separate subdirectories for toolchain components
# (binutils/, gcc/, gdb/, newlib/).

xtensa_overlay = "https://github.com/jcmvbkbc/xtensa-toolchain-build/raw/master/overlays/xtensa_dc233c.tar.gz;name=overlay"

SRC_URI += "${xtensa_overlay}"
SRC_URI[overlay.md5sum] = "15a0e613303005a9c0bea85695ada02f"
SRC_URI[overlay.sha256sum] = "a76052c0c71d4025b117dcfc18ca7e189ac495a7645fa98c1bdaddca15cce815"
