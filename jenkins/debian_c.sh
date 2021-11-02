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
openssl version
curl --version

build_root=$(cd "$(dirname "$0")/.." && pwd)
cd $build_root

build_folder=$build_root"/cmake"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

if [ $CORES -eq 1 ]
then
    LOADLIMIT=$CORES
else
    LOADLIMIT=$(perl -e "print int($CORES * 0.8)")
fi

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake -Drun_valgrind:BOOL=ON $build_root -Drun_unittests:BOOL=ON

echo "Running: make --jobs=$CORES --max-load=$LOADLIMIT"
make --jobs=$CORES --max-load=$LOADLIMIT

popd
