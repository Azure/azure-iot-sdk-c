#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# Print version
cat /etc/*release | grep VERSION*
gcc --version

# back out of jenkins folder
echo "this is where we start "pwd
cd ..
# save sdk root
sdk_root=$(pwd)
echo $sdk_root && ls

cd ..
git clone https://github.com/Azure/azure-iot-c-back-compat.git --recursive

back_compat_folder=$sdk_root"../azure-iot-c-back-compat/cmake"
ls
build_root="$sdk_root"
cd $build_root
echo "sdk build root $build_root"

build_folder=$build_root"/cmake/sdk"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -rf $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $build_root -Drun_e2e_tests=ON
make install --jobs=$CORES
popd

# Now run back compat
cd $sdk_root
cd "azure-iot-c-back-compat"
mkdir -p "cmake"
pushd $back_compat_folder

# By default back compat uses the installed components
cmake "$sdk_root/azure-iot-c-back-compat"
make --jobs=$CORES

ctest -j $CORES --output-on-failure
popd
