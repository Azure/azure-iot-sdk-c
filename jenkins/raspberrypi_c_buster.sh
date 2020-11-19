#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -x # Set trace on
set -o errexit # Exit if command failed
set -o nounset # Exit if variable not set
set -o pipefail # Exit if pipe failed

# assume directory is root of downloaded repo
mkdir cmake
cd cmake
ls -al

# Create a cmake toolchain file on the fly
echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake

echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc)" >> toolchain.cmake
echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-g++)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
ls -al

# Build the SDK. This will use the OpenSSL, cURL and uuid binaries that we built before
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -Duse_prov_client:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} -Drun_e2e_tests:BOOL=ON -Drun_unittests=:BOOL=ON ..
make -j 2
ls -al
