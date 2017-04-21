#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e
script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root/cmake/linux_network_e2e

echo running ${script_dir}/{rt_container.sh} $*

docker run --rm \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v${build_root}:${build_root} \
  -e IOTHUB_CONNECTION_STRING \
  -e IOTHUB_DEVICE_CONN_STR \
  -e IOTHUB_E2E_X509_CERT \
  -e IOTHUB_E2E_X509_PRIVATE_KEY \
  -e IOTHUB_E2E_X509_THUMBPRINT \
  -e IOTHUB_EVENTHUB_CONNECTION_STRING \
  -e IOTHUB_PARTITION_COUNT \
  --network="TestNat" \
  --privileged \
  -it \
  jenkins-network-e2e \
  /bin/bash ${script_dir}/rt_container.sh $*

  
