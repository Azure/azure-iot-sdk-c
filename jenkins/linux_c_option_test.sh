#!/bin/bash
#set -o pipefail
#
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root"/cmake/iot_option"

# Set the default cores
MAKE_CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

echo "Initial MAKE_CORES=$MAKE_CORES"

# Make sure there is enough virtual memory on the device to handle more than one job  
MINVSPACE="1500000"

# Acquire total memory and total swap space setting them to zero in the event the command fails
MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
[ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
[ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0

let VSPACE=${MEMAR[0]}+${MEMAR[1]}

if [ "$VSPACE" -lt "$MINVSPACE" ] ; then
echo "WARNING: Not enough space.  Setting MAKE_CORES=1"
MAKE_CORES=1
fi

declare -a arr=(
    "-Duse_http=OFF"
    "-Duse_amqp=OFF -Duse_http=OFF -Dno_logging=OFF -Ddont_use_uploadtoblob=ON"
    "-Duse_prov_client=ON -Dbuild_provisioning_service_client=OFF"
    "-Dbuild_as_dynamic=ON")

for item in "${arr[@]}"
do
    rm -r -f $build_folder
    mkdir -p $build_folder
    pushd $build_folder

    echo "$item"
    cmake $build_root "$item"

    make --jobs=$MAKE_CORES
done
popd
