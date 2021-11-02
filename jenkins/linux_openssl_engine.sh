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
curl --version

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
cmake . -Bcmake -Duse_openssl:BOOL=ON -Drun_e2e_tests:BOOL=ON -Drun_e2e_openssl_engine_tests:BOOL=ON -Drun_valgrind:BOOL=ON
cd cmake

make --jobs=$CORES

# Configure OpenSSL with PKCS#11 Engine and SoftHSM
# 1. Create new test token.
#softhsm2-util --delete-token --token test-token > /dev/null
softhsm2-util --init-token --slot 0 --label test-token --pin 1234 --so-pin 4321
# 2. Convert key from PKCS#1 to PKCS#8
echo $IOTHUB_E2E_X509_PRIVATE_KEY_BASE64 | base64 --decode > test-key.pem
openssl pkcs8 -topk8 -inform PEM -outform PEM -nocrypt -in test-key.pem -out test-key.p8
# 3. Import private key into the token.
softhsm2-util --pin 1234 --import ./test-key.p8 --token test-token --id b000 --label test-privkey
rm test-key.pem
# 4. (Test) List keys associated with slot (should see the private key listed)
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so -l -p 1234 --token test-token --list-objects
