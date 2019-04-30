#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# Print version
cat /etc/*release | grep VERSION*
gcc --version

back_compat_root=$(cd "$(dirname "$0")/../.." && pwd)
echo "back compat root $back_compat_root"
cd $back_compat_root
git clone https://github.com/Azure/azure-iot-c-back-compat.git

back_compat_folder=$back_compat_root"/cmake/back_compat"


build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root
echo "sdk build root $build_root"

build_folder=$build_root"/cmake/sdk"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)


rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $build_root -Drun_e2e_tests=ON
make install --jobs=$CORES
popd

# Now run back compat
cd $back_compat_root
cd "azure-iot-c-back-compat"
mkdir -p $back_compat_folder
pushd $back_compat_folder
cmake $back_compat_root
make --jobs=$CORES

ctest -j $CORES --output-on-failure
popd
