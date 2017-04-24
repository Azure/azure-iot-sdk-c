#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

# This script executes inside a container and runs an individual test in the context of a developer debugging session.

set -e
script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root/cmake/linux_network_e2e
ctest_options="-V -j 1"

groupmod -g $(stat -c "%g" /var/run/docker.sock) docker

cd $build_folder/network_e2e/tests/iothubclient_badnetwork_e2e
export E2E_PROTOCOL=$1
if  [ -z $2 ]	
then
	ctest $ctest_options -E ".*_(valgrind|helgrind|drd)"
elif [ $2 == 'valgrind' ]
then
	ctest $ctest_options -R ".*_valgrind"
elif [ $2 == 'helgrind' ]
then
	ctest -$ctest_options  -R  ".*_helgrind"
elif [ $2 == 'drd' ]
then
	ctest $ctest_options -R ".*_drd"
else
	echo 'usage: $0 [protocol] [valgrind|helgrind|drd]'
	exit 1
fi


 
