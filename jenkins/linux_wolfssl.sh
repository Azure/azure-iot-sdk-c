#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Print version
cat /etc/*release | grep VERSION*
gcc --version
curl --version

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

build_folder=$build_root"/cmake/wolfssl"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $build_root -Drun_unittests:BOOL=ON -Duse_wolfssl:BOOL=ON
make --jobs=$CORES

ctest -j $CORES -VV --output-on-failure

popd