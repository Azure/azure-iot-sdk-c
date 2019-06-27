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

sdk_build_folder=$build_root"/cmake/install_deps"

# Build the SDK
rm -rf $sdk_build_folder
mkdir -p $sdk_build_folder
pushd $sdk_build_folder
cmake $build_root
make install --jobs=$CORES
popd

# Now use the deps
rm -rf $sdk_build_folder
mkdir -p $sdk_build_folder
pushd $sdk_build_folder
cmake $build_root -Duse_installed_dependencies=ON
make --jobs=$CORES
popd
