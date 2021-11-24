#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -x # Set trace on
set -o errexit # Exit if command failed
set -o nounset # Exit if variable not set
set -o pipefail # Exit if pipe failed

# Print version
cat /etc/*release | grep VERSION*
gcc --version
curl --version

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

build_folder=$build_root"/cmake"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $build_root -Duse_mbedtls=ON -Duse_openssl=OFF -Ddont_use_uploadtoblob=ON -Drun_e2e_tests=ON
make --jobs=$CORES

popd
