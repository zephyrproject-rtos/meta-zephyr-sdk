# meta-zephyr-sdk

Build Zephyr SDK
================

You will need to install all the packages required to build Yocto,
makeself and p7zip-full.

  Yocto Requirements:
    http://www.yoctoproject.org/docs/current/yocto-project-qs/yocto-project-qs.html#packages

In order to build the SDK from source you need to first clone
meta-zephyr-sdk and then from the parent directory:

 1. Run clone scripts. This script will clone poky and apply patches.
    ./meta-zephyr-sdk/scripts/meta-zephyr-sdk-clone.sh
 2. Build SDK using script. This script will build all the tools.
    ./meta-zephyr-sdk/scripts/meta-zephyr-sdk-build.sh

  Note: if you want, you can use enviroment variables to indicate your
  poky and meta-zephyr-sdk location.
    SDK_SOURCE: meta-zephyr-sdk location. Default $PWD/meta-zephyr-sdk
    META_POKY_SOURCE: poky location. Default $PWD/poky
