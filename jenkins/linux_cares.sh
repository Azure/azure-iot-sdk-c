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

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
cmake . -Bcmake -Duse_c_ares:BOOL=ON -Drun_e2e_tests:BOOL=ON
cd cmake

make --jobs=$CORES

ctest -j $CORES --output-on-failure
