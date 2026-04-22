#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.

# Only for testing E2E behaviour !!! 
TEST_CORES=16
VALGRIND_CORES=4
E2E_VALGRIND_CORES=2

# Refresh dynamic libs to link to
sudo ldconfig

# Run non-valgrind tests with full parallelism
ctest -T test --no-compress-output -C "Debug" -V -j $TEST_CORES --schedule-random -E "_(valgrind|helgrind|drd)$"

# Run valgrind/helgrind/drd unit tests (non-E2E) with moderate parallelism
ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_CORES --schedule-random -R "_(valgrind|helgrind|drd)$" -E "e2e"

# Run E2E valgrind/helgrind/drd tests one tool at a time with reduced
# parallelism to avoid SSL connection storms from too many concurrent
# TLS handshakes under instrumentation.
ctest -T test --no-compress-output -C "Debug" -V -j $E2E_VALGRIND_CORES --schedule-random -R "e2e_valgrind$"
ctest -T test --no-compress-output -C "Debug" -V -j $E2E_VALGRIND_CORES --schedule-random -R "e2e_helgrind$"
ctest -T test --no-compress-output -C "Debug" -V -j $E2E_VALGRIND_CORES --schedule-random -R "e2e_drd$"
