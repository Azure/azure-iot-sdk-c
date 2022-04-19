#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.

# Only for testing E2E behaviour !!! 
TEST_CORES=16

# Refresh dynamic libs to link to
sudo ldconfig

ctest --overwrite MemoryCheckCommandOptions="--leak-check=full --error-exitcode=100" -T MemCheck --no-compress-output -C "Debug" -V -j $TEST_CORES --schedule-random
