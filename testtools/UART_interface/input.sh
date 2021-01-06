#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# basic setup of input script for python, execution steps are as follows:
# 1. wait until device flashing complete, then send help
# 2. set the IoT Hub Connection String
# 3. call exit, resetting the device to ensure mxchip connects to the set IoT Hub
# 4. call SERIAL_TASK, typically send_telemetry
# 5. call version, to log versions of software on device

echo -e "help\r\nset_wifissid $WIFI_SSID\r\nset_wifipwd $WIFI_PWD\r\nset_az_iothub $IOTHUB_CONNECTION_STRING\r\nexit\r\n$SERIAL_TASK\r\n\r\nversion" | cat > input.txt
