#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
rpi_tools_repo="https://github.com/yungez/rpitools.git"
rpi_tools_root="$(cd "$(dirname "$0")/../.." && pwd)/rpitools"
rpi_csample_repo="https://github.com/Azure-Samples/iot-hub-c-raspberrypi-getting-started.git"
rpi_csample_root="$(cd "$(dirname "$0")/../.." && pwd)/iot-hub-c-raspberrypi-getting-started"
toolchain_file="$rpi_csample_root/.misc/toolchain-rpi.cmake"
azure_iot_sdk_build_root=$(cd "$(dirname "$0")/.." && pwd)

clone_repo ()
{
    if [ -d "$2" ]; then
        rm -fr $2
    fi
    echo git clone repo $1 to $2
    git clone --recursive -b $3 $1 $2
}

# step 1. clone raspberry pi build tools repo and raspberry pi c sample code repo
clone_repo $rpi_tools_repo $rpi_tools_root "master"
clone_repo $rpi_csample_repo $rpi_csample_root "develop"

# step 2 set RPI_ROOT env for cross compilation
export RPI_ROOT=$rpi_tools_root/raspbian-jessie-sysroot

# step 3 cross compile azure iot sdk C
cd $azure_iot_sdk_build_root

./build_all/linux/build.sh --toolchain-file $toolchain_file -cl --sysroot="$RPI_ROOT"
if [ $? -eq 0 ]; then
    echo azure iot sdk build succeeded!
else
    exit $?
fi

# step 4 cross compile raspberrypi c sample code
cd $rpi_csample_root

./.misc/build.sh --toolchain-file $toolchain_file -cl --sysroot="$RPI_ROOT" --azure-iot-sdk-c $iothub_sdks_build_root
if [ $? -eq 0 ]; then
    echo raspberry pi c sample code build succeeded!
else
    exit $?
fi

# cleanup local repos, if failure not cleanup for trouble shooting
cleanup ()
{
    rm -fr $rpi_tools_root
    rm -fr $rpi_csample_root
}
cleanup

