#!/bin/bash
# Script to create the makefile to cross compile for host linux-armhf
# Assuming that the variable ARMHF_SYSROOT_DIR points to the system root
# of the host system with requiered libraries and headers.

repo_root=$(cd "$(dirname "$0")/../.." && pwd)

$(dirname "$0")/build.sh --toolchain-file cmake_toolchain_file_armhf_linux -cl --sysroot=$ARMHF_SYSROOT_DIR -cl -L${ARMHF_SYSROOT_DIR}/usr/lib/arm-linux-gnueabihf
