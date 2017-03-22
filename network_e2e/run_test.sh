#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e
script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root/cmake/linux_network_e2e

groupmod -g $(stat -c "%g" /var/run/docker.sock) docker

cd $build_folder/network_e2e/tests/iothubclient_badnetwork_e2e
export E2E_PROTOCOL=$1
ctest --output-on-failure -j 1 



