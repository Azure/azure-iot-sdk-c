#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# Print version
cat /etc/*release | grep VERSION*
gcc --version

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

build_root=$(cd "$(dirname "$0")/.." && pwd)
clone_root=$(cd "$(dirname "$0")/../.." && pwd)
cd $build_root

echo "Build Root $build_root"

sdk_build_folder=$build_root"/cmake"

# Build the SDK
rm -rf $sdk_build_folder
mkdir -p $sdk_build_folder
pushd $sdk_build_folder
cmake $build_root -Duse_prov_client=ON
make install --jobs=$CORES
popd

cd "$clone_root"

echo "Clone the back compat repo"
git clone https://github.com/Azure/azure-iot-c-back-compat.git --recursive

back_compat_root="$clone_root/azure-iot-c-back-compat"
back_compat_build=$back_compat_root"/cmake"

# Now run back compat
rm -rf $back_compat_build
mkdir -p $back_compat_build
pushd $back_compat_build
cmake $back_compat_root
make --jobs=$CORES

ctest -j $CORES --output-on-failure
popd
