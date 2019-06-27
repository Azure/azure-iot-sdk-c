#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

chmod -R 777 $PWD/drop
# we need to cp because of something weird with artifacts
mkdir -p $PWD/drop/copy/cmake/iothub_client/tests
cp -r $PWD/drop/source_artifacts/cmake/iothub_client/tests/ $PWD/drop/copy/cmake/iothub_client/

sudo setcap cap_net_raw,cap_net_admin+ep \
$PWD/drop/source_artifacts/cmake/iothub_client/tests/iothubclient_mqtt_e2e/iothubclient_mqtt_e2e_exe \
cap_net_raw,cap_net_admin+ep $PWD/drop/source_artifacts/cmake/iothub_client/tests/iothubclient_amqp_e2e/iothubclient_amqp_e2e_exe \
cap_net_raw,cap_net_admin+ep $PWD/drop/source_artifacts/cmake/iothub_client/tests/iothubclient_mqtt_ws_e2e/iothubclient_mqtt_ws_e2e_exe \
cap_net_raw,cap_net_admin+ep $PWD/drop/source_artifacts/cmake/iothub_client/tests/iothubclient_amqp_ws_e2e/iothubclient_amqp_ws_e2e_exe \
cap_net_raw,cap_net_admin+ep $PWD/drop/source_artifacts/cmake/iothub_client/tests/iothubclient_uploadtoblob_e2e/iothubclient_uploadtoblob_e2e_exe 


$PWD/drop/copy/cmake/iothub_client/tests/iothubclient_mqtt_e2e/iothubclient_mqtt_e2e_exe
$PWD/drop/copy/cmake/iothub_client/tests/iothubclient_amqp_e2e/iothubclient_amqp_e2e_exe
$PWD/drop/copy/cmake/iothub_client/tests/iothubclient_mqtt_ws_e2e/iothubclient_mqtt_ws_e2e_exe
$PWD/drop/copy/cmake/iothub_client/tests/iothubclient_amqp_ws_e2e/iothubclient_amqp_ws_e2e_exe
$PWD/drop/copy/cmake/iothub_client/tests/iothubclient_uploadtoblob_e2e/iothubclient_uploadtoblob_e2e_exe
