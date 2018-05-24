#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root"/cmake"

CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
TEST_JOB_COUNT=16
echo "Building with $CORES cores and running $TEST_JOB_COUNT tests in parallel"

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder

export DYLD_LIBRARY_PATH="/usr/local/Cellar/curl/7.58.0/lib:$DYLD_LIBRARY_PATH"

cmake .. -Drun_unittests:bool=ON -Drun_e2e_tests=ON -Drun_sfc_tests=ON -G Xcode
cmake --build . -- --jobs=$CORES
ctest -j $TEST_JOB_COUNT -C "debug" -V
popd
