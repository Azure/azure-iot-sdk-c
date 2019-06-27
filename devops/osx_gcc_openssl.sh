#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

set -e

sw_vers
gcc --version
openssl version
curl --version

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root"/cmake"

CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake .. -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl -Duse_openssl:bool=ON -Drun_unittests:bool=ON
cmake --build . -- --jobs=$CORES
ctest -j 8 -C "debug" -VV --output-on-failure
popd
