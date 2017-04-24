#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e
export script_dir=$(cd "$(dirname "$0")" && pwd)
export build_root=$(cd "${script_dir}/.." && pwd)
export build_folder=$build_root/cmake/linux_network_e2e

# build our docker image
docker build -t jenkins-network-e2e:latest ${script_dir}/Dockerfile.Linux

# create the network that our tests will bring down and up
# TODO; conditional creation

set +e
docker network rm TestNat
set -e
docker network create --subnet=10.1.0.0/24 TestNat

# build our docker-compose file
rm -rf compose.linux/docker-compose.yml
envsubst < "compose.linux/template.yml" > "compose.linux/docker-compose.yml"

# build our code
rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder

cmake \
    -Drun_valgrind:BOOL=1 \
    -DcompileOption_C:STRING=  \
    -Drun_e2e_tests:BOOL=ON \
    -Duse_wsio:BOOL=ON \
    -Drun_unittests:BOOL=ON \
    -Dbuild_network_e2e:BOOL=ON \
    $build_root

cd network_e2e/tests
make -j

popd 

 


