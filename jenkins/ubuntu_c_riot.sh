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

# -- C --
./build_all/linux/build.sh --use-hsmriot --run-unittests --run_valgrind --run-e2e-tests --run-sfc-tests --provisioning --use-edge-modules "$@" #-x 
[ $? -eq 0 ] || exit $?

