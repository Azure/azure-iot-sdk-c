#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o pipefail # Exit if pipe failed.

usage() {
    echo "${0} OR ${0} [run_valgrind]" 1>&2
    exit 1
}

RUN_VALGRIND=${1:-""}

if [[ "$RUN_VALGRIND" == "run_valgrind" ]]; then
  echo "running valgrind"
elif [[ "$RUN_VALGRIND" != "" ]]; then
  usage
fi

# Set the default cores
MAKE_CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

echo "Initial MAKE_CORES=$MAKE_CORES"

# Make sure there is enough virtual memory on the device to handle more than one job  
MINVSPACE="1500000"

# Acquire total memory and total swap space setting them to zero in the event the command fails
MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
[ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
[ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0

let VSPACE=${MEMAR[0]}+${MEMAR[1]}

echo "VSPACE=$VSPACE"

if [ "$VSPACE" -lt "$MINVSPACE" ] ; then
  echo "WARNING: Not enough space.  Setting MAKE_CORES=1"
  MAKE_CORES=1
fi

echo "MAKE_CORES=$MAKE_CORES"
echo "Starting run..."
date
make --jobs=$MAKE_CORES
echo "completed run..."
date

# Only for testing E2E behaviour !!! 
TEST_CORES=16

if [[ "$RUN_VALGRIND" == "run_valgrind" ]] ;
then
  #use doctored openssl
  export LD_LIBRARY_PATH=/usr/local/ssl/lib
  ctest -j $TEST_CORES --output-on-failure --schedule-random
  export LD_LIBRARY_PATH=
else
  ctest -j $TEST_CORES -C "Debug" --output-on-failure --schedule-random
fi

