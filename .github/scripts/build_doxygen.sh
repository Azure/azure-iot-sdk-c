#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo apt update
sudo apt install -y git cmake python flex bison

pushd ~
git clone -b Release_1_9_0 https://github.com/doxygen/doxygen.git
pushd doxygen
mkdir build
pushd build
cmake -G "Unix Makefiles" ..
make
sudo make install
popd
popd
popd

doxygen doc/Doxyfile
