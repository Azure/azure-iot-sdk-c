#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

#install nodejs
sudo curl -sL https://deb.nodesource.com/setup_13.x | sudo -E bash -
sudo apt-get install nodejs -y

# Print version
cat /etc/*release | grep VERSION*
gcc --version
curl --version
node --version
npm --version

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

build_folder=$build_root"/cmake/wolfssl"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $build_root -Duse_wolfssl=ON -Duse_openssl=OFF -Ddont_use_uploadtoblob=ON -Drun_e2e_tests=ON
make --jobs=$CORES

ctest -j $CORES --output-on-failure

popd