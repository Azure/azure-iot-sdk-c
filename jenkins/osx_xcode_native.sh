#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -x # Set trace on
set -o errexit # Exit if command failed
set -o nounset # Exit if variable not set
set -o pipefail # Exit if pipe failed

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

cmake .. -Drun_e2e_tests=ON -G Xcode -DCMAKE_BUILD_TYPE=Debug
cmake --build . -- --jobs=$CORES
popd
