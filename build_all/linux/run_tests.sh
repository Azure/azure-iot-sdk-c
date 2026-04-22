#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.

# Parallelism settings
UT_CORES=16
E2E_CORES=16
VALGRIND_UT_CORES=4
VALGRIND_E2E_CORES=2

run_e2e=false
run_valgrind=false
run_helgrind=false
run_drd=false

for arg in "$@"; do
    case "$arg" in
        --e2e)       run_e2e=true ;;
        --valgrind)  run_valgrind=true ;;
        --helgrind)  run_helgrind=true ;;
        --drd)       run_drd=true ;;
        *)           echo "Unknown option: $arg"; exit 1 ;;
    esac
done

# If no instrumentation flags are set, run plain (non-valgrind) tests.
run_plain=true
if $run_valgrind || $run_helgrind || $run_drd; then
    run_plain=false
fi

# Refresh dynamic libs to link to
sudo ldconfig

if $run_plain; then
    if $run_e2e; then
        # Unit tests + E2E, no valgrind/helgrind/drd
        ctest -T test --no-compress-output -C "Debug" -V -j $E2E_CORES --schedule-random -E "_(valgrind|helgrind|drd)$"
    else
        # Unit tests only, no E2E, no valgrind/helgrind/drd
        ctest -T test --no-compress-output -C "Debug" -V -j $UT_CORES --schedule-random -E "_(valgrind|helgrind|drd)|e2e"
    fi
fi

if $run_valgrind; then
    # Unit tests under valgrind
    ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_UT_CORES --schedule-random -R "_valgrind$" -E "e2e"
    if $run_e2e; then
        # E2E tests under valgrind
        ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_E2E_CORES --schedule-random -R "e2e_valgrind$"
    fi
fi

if $run_helgrind; then
    # Unit tests under helgrind
    ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_UT_CORES --schedule-random -R "_helgrind$" -E "e2e"
    if $run_e2e; then
        # E2E tests under helgrind
        ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_E2E_CORES --schedule-random -R "e2e_helgrind$"
    fi
fi

if $run_drd; then
    # Unit tests under drd
    ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_UT_CORES --schedule-random -R "_drd$" -E "e2e"
    if $run_e2e; then
        # E2E tests under drd
        ctest -T test --no-compress-output -C "Debug" -V -j $VALGRIND_E2E_CORES --schedule-random -R "e2e_drd$"
    fi
fi
