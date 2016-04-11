SUMMARY = "PLY is an implementation of lex and yacc parsing tools for Python."
HOMEPAGE = "http://www.dabeaz.com/ply/"
LICENSE = "DABAEZ"
SECTION = "devel/python"
SRCNAME = "ply"

LIC_FILES_CHKSUM = "file://README.md;beginline=1;endline=31;md5=9db48a26f7031363876fe7685a51ac4f"

SRCREV = "d776a2ece6c12bf8f8b6a0e65b48546ac6078765"
SRC_URI = "git://github.com/dabeaz/ply.git;protocol=https"

S = "${WORKDIR}/git"

inherit setuptools

RDEPENDS_${PN} = "python"

FILES_${PN} = " \
   ${libdir}/${PYTHON_DIR}/site-packages \
  "

BBCLASSEXTEND += "nativesdk"
