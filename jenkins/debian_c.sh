#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# Print version
grep VERSION /etc/*release

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

build_folder=$build_root"/cmake/debian_linux"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake -Drun_valgrind:BOOL=ON $build_root -Drun_unittests:BOOL=ON
make --jobs=$CORES

#use doctored openssl
export LD_LIBRARY_PATH=/usr/local/ssl/lib
ctest -j $CORES --output-on-failure
export LD_LIBRARY_PATH=

popd
