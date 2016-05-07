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

# Master build
if [ ! -d "poky" ]; then
  git clone http://git.yoctoproject.org/git/poky
else
  echo "Repo already cloned..."
#  exit 1
fi

# Checkout the commit known to build...
cd poky
# Security fixes
git checkout ae57ea03c6a41f2e3b61e0c157e32ca7df7b3c4b

if [ ! -d "meta-zephyr-sdk" ]; then
  git clone https://gerrit.zephyrproject.org/r/p/meta-zephyr-sdk.git
  cd meta-zephyr-sdk
  git checkout tags/0.7.5
  cd ..
  echo "Patching poky in: $PWD"
  # patches created by git diff --no-prefix
  for i in ./meta-zephyr-sdk/poky-patches/*.patch;
    do
      patch -s -p0 < $i;
    done
fi






