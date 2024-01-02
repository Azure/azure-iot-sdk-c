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

pushd $back_compat_root

# Now run back compat
rm -rf cmake
mkdir -p cmake
pushd cmake
cmake ..
make --jobs=$CORES

popd
