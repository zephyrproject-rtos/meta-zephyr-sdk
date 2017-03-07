SUMMARY = "PyYAML is a YAML parser and emitter for the Python programming language."
HOMEPAGE = "http://pyyaml.org/wiki/PyYAML"
LICENSE = "MIT"
SECTION = "devel/python"
SRCNAME = "yaml"

LIC_FILES_CHKSUM = "file://LICENSE;md5=6015f088759b10e0bc2bf64898d4ae17"

SRC_URI[md5sum] = "4c129761b661d181ebf7ff4eb2d79950"
SRC_URI[sha256sum] = "592766c6303207a20efc445587778322d7f73b161bd994f227adaa341ba212ab"
SRC_URI = "http://pyyaml.org/download/pyyaml/PyYAML-3.12.tar.gz"

S = "${WORKDIR}/PyYAML-3.12"

inherit setuptools

RDEPENDS_${PN} = "python"

FILES_${PN} = " \
   ${libdir}/${PYTHON_DIR}/site-packages \
  "

BBCLASSEXTEND += "nativesdk"
