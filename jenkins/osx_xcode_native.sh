#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#

set -e

sw_vers
xcodebuild -version

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root"/cmake"

CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
TEST_JOB_COUNT=16
echo "Building with $CORES cores and running $TEST_JOB_COUNT tests in parallel"

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder

curl_path=/usr/local/Cellar/curl/7.61.0/lib
if [ -d $curl_path ]; then
    export DYLD_LIBRARY_PATH="$curl_path:$DYLD_LIBRARY_PATH"
else
    echo "ERROR: Could not find curl in path $curl_path."
    echo "Make sure curl is installed through brew in such path, or update this script with the correct one."
    exit 1
fi

cmake .. -Drun_e2e_tests=ON -G Xcode
cmake --build . -- --jobs=$CORES
ctest -j $TEST_JOB_COUNT -C "debug" -VV --output-on-failure
popd
