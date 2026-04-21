#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o pipefail # Exit if pipe failed.

# Only for testing E2E behaviour !!! 
TEST_CORES=16
VALGRIND_CORES=4

# Refresh dynamic libs to link to
sudo ldconfig

result=0

# Run non-valgrind tests with full parallelism
ctest -T test --no-compress-output -C "Debug" -V -j $TEST_CORES --schedule-random -E "_(valgrind|helgrind|drd)$" || result=$?

# Run valgrind/helgrind/drd tests with reduced parallelism to avoid
# SSL connection storms from too many concurrent TLS handshakes
ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_CORES --schedule-random -R "_(valgrind|helgrind|drd)$" || result=$?

exit $result
