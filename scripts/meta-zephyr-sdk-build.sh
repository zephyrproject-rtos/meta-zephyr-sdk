#!/bin/bash
#
# Copyright (C) 2015-2016, Intel Corporation.
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#


curdir=$(pwd)
mkdir -p downloads
export DL_DIR=$curdir/downloads

TOOLCHAINS=$curdir/poky/meta-zephyr-sdk/scripts/toolchains

if [ ! -d $TOOLCHAINS ] ; then
	mkdir -p $TOOLCHAINS
fi

rm -rf $TOOLCHAINS/*

cd poky

# setconf_var, i.e. "MACHINE","qemuarm",$localconf
setconf_var()
{
	sed -i "/^$1=/d" $3
	echo "$1=\"$2\"" >> $3
	echo $1="$2"
}

header ()
{
echo ""
echo "########################################################################"
echo "    $1"
echo "########################################################################"
}

newbuild()
{
	cd $curdir/poky
	source oe-init-build-env $1
	
	# Create bblayers.conf
	bblayers=conf/bblayers.conf

	echo "# LAYER_CONF_VERSION is increased each time build/conf/bblayers.conf" > $bblayers
	echo "# changes incompatibly" >> $bblayers
	echo "LCONF_VERSION = \"6\"" >> $bblayers
	echo "" >> $bblayers
	echo "BBPATH = \"\${TOPDIR}\"" >> $bblayers
	echo "BBFILES ?= \"\"" >> $bblayers
	echo "BBLAYERS ?= \" \\" >> $bblayers
	echo "  $curdir/poky/meta \\" >> $bblayers
	echo "  $curdir/poky/meta-yocto \\" >> $bblayers
	echo "  $curdir/poky/meta-yocto-bsp \\" >> $bblayers
	echo "  $curdir/poky/meta-zephyr-sdk \\" >> $bblayers
	echo "  \" " >> $bblayers
	echo "BBLAYERS_NON_REMOVABLE ?= \" \\" >> $bblayers
	echo "  $curdir/poky/meta \\" >> $bblayers
	echo "  $curdir/poky/meta-yocto \\" >> $bblayers
	echo "  \" " >> $bblayers

	# Common values for all builds
	localconf=conf/local.conf
	setconf_var "DL_DIR" "$curdir/downloads" $localconf
	setconf_var "SDKMACHINE" "i686" $localconf
	setconf_var "DISTRO" "zephyr-sdk" $localconf
}

##############################################################################
# 32 bit build
##############################################################################
header "Building Zephyr host tools..."
newbuild build-zephyr-tools  > /dev/null
setconf_var "MACHINE" "qemux86" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake hosttools-tarball -c clean  > /dev/null
bitbake hosttools-tarball
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
echo "Building additional host tools...done"

# build ARM toolchain
header "Building ARM toolchain..."
newbuild build-zephyr-arm  > /dev/null
setconf_var "MACHINE" "qemuarm" $localconf
setconf_var "TCLIBC" "baremetal" $localconf
setconf_var "TOOLCHAIN_TARGET_TASK_append" " newlib" $localconf
setconf_var "TUNE_FEATURES" "armv7m cortexm3" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake meta-toolchain -c clean  > /dev/null
bitbake meta-toolchain 
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
header "Building ARM toolchain...done"

# build x86 toolchain
header "Building x86 toolchain..."
newbuild build-zephyr-x86 > /dev/null
setconf_var "MACHINE" "qemux86" $localconf
setconf_var "TCLIBC" "baremetal" $localconf
setconf_var "TOOLCHAIN_TARGET_TASK_append" " newlib" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake meta-toolchain -c clean  > /dev/null
bitbake meta-toolchain 
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
header "Building x86 toolchain...done"

#  build MIPS toolchain
header "Building MIPS toolchain..."
newbuild build-zephyr-mips  > /dev/null
setconf_var "MACHINE" "qemumips" $localconf
setconf_var "TCLIBC" "baremetal" $localconf
setconf_var "TOOLCHAIN_TARGET_TASK_append" " newlib" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake meta-toolchain -c clean  > /dev/null
bitbake meta-toolchain 
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
header "Building MIPS toolchain...done"

# build ARC toolchain...
header "Building ARC toolchain..."
newbuild build-zephyr-arc  > /dev/null
setconf_var "MACHINE" "arc" $localconf
setconf_var "TCLIBC" "baremetal" $localconf
setconf_var "TOOLCHAIN_TARGET_TASK_append" " newlib" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake meta-toolchain -c clean  > /dev/null
bitbake meta-toolchain 
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
header "Building ARC toolchain...done"

#  build IAMCU toolchain
header "Building IAMCU  toolchain..."
newbuild build-zephyr-iamcu  > /dev/null
setconf_var "MACHINE" "iamcu" $localconf
setconf_var "TCLIBC" "baremetal" $localconf
setconf_var "TOOLCHAIN_TARGET_TASK_append" " newlib" $localconf
rm -f ./tmp/deploy/sdk/*.sh 
bitbake meta-toolchain -c clean  > /dev/null
bitbake meta-toolchain 
[ $? -ne 0 ] && echo "Error(s) encountered during bitbake." && exit 1
cp ./tmp/deploy/sdk/*.sh $TOOLCHAINS
[ $? -ne 0 ] && exit 1
header "Building IAMCU toolchain...done"

# Pack it together ...

cd $curdir/poky/meta-zephyr-sdk/scripts
header "Creating SDK..."
./make_zephyr_sdk.sh 
[ $? -ne 0 ] && echo "Error(s) encountered during SDK creation." && exit 1


