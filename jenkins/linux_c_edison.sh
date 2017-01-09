#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
edison_csample_repo="https://github.com/Azure-Samples/iot-hub-c-edison-getting-started.git"
edison_csample_root="$(cd "$(dirname "$0")/../.." && pwd)/iot-hub-c-edison-getting-started"
toolchain_file="$edison_csample_root/.misc/toolchain-edison.cmake"
azure_iot_sdk_build_root=$(cd "$(dirname "$0")/.." && pwd)

clone_repo ()
{
    if [ -d "$2" ]; then
        rm -fr $2
    fi
    echo git clone repo $1 to $2
    git clone --recursive -b $3 $1 $2
}

# step 1. clone intel edison c sample code repo
clone_repo $edison_csample_repo $edison_csample_root "develop"

# step 2 set cross compilation tool chain
toolchain_root=/opt/poky-edison/1.7.2
toolchain_setup_script=$edison_csample_root/.misc/setup.sh
toolchain_env_script=$toolchain_root/environment-setup-core2-32-poky-linux
sysroot=$toolchain_root/sysroots/core2-32-poky-linux

source $toolchain_setup_script
source $toolchain_env_script

# step 3 cross compile azure iot sdk C
cd $azure_iot_sdk_build_root

./build_all/linux/build.sh --toolchain-file $toolchain_file -cl --sysroot=$sysroot
if [ $? -eq 0 ]; then
    echo azure iot sdk build succeeded!
else
    exit $?
fi

# step 4 cross compile edison c sample code
cd $edison_csample_root

./.misc/build.sh --toolchain-file $toolchain_file -cl --sysroot=$sysroot --azure-iot-sdk-c $azure_iot_sdk_build_root
if [ $? -eq 0 ]; then
    echo intel edison c sample code build succeeded!
else
    exit $?
fi

# cleanup local repos, if failure not cleanup for trouble shooting
cleanup ()
{
    rm -fr $edison_csample_root
}
cleanup
