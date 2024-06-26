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

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
cmake . -Bcmake -Duse_c_ares:BOOL=ON -Drun_e2e_tests:BOOL=ON -Ddont_use_uploadtoblob:BOOL=ON
cd cmake

make --jobs=$CORES
