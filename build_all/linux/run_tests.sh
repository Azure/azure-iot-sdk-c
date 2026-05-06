#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.

# Parallelism settings
UT_CORES=16
# E2E tests are latency-sensitive: every test opens multiple AMQP+HTTPS
# connections to IoT Hub in parallel and asserts on MAX_CLOUD_TRAVEL_TIME
# (seconds). Microsoft-hosted Ubuntu agents are 4-vCPU VMs, so running the
# ~16 E2E binaries all at -j 16 oversubscribes CPU and network and causes
# per-request latency to spike above the assertion threshold
# (seen: service_client_update_twin taking 126s vs ~1s baseline, which
# blew MAX_CLOUD_TRAVEL_TIME in iothubclient_amqp_dt_e2e on the mbedTLS
# job). Keep E2E parallelism at 4 to match the hosted-agent core count.
E2E_CORES=4
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
    # NOTE: E2E tests are intentionally not run under drd.  drd's thread instrumentation adds
    # 20-50x performance overhead, which makes libcurl's TLS handshakes to Azure services fail
    # with "SSL connect error".  Each failed IoTHubDeviceMethod_Invoke then cascades through
    # its retry loop (~85s per retry), causing individual test suites to exceed 30+ minutes
    # and the overall pipeline to hit its timeout.  Thread-race detection for E2E scenarios
    # is still provided by the helgrind pass, which is the primary thread-safety tool.
fi
