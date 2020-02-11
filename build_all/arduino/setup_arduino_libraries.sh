#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
echo $NEWTIN | sudo -S rm -r azure-iot-pal-arduino
git clone https://github.com/Azure/azure-iot-pal-arduino.git 

cd azure-iot-pal-arduino
git submodule update --init --recursive
rsync -avz --existing ./ sdk/

# check out pipeline branch
cd sdk
echo "in sdk now"
ls
git pull origin master
git checkout $SDK_BRANCH_NAME
git submodule update --init --recursive
cd ../build_all
git submodule status

python3 make_sdk.py -o $ARDUINO_LIBRARY_DIR

# input WIFI_SSID, WIFI_PWD, and IOTHUB_CONNECTION_DEVICE_STRING into iot_config.h

cd $ARDUINO_LIBRARY_DIR/AzureIoTHub/examples/iothub_ll_telemetry_sample/
echo $NEWTIN | sudo -S sed -i 's/your-wifi-name/'$WIFI_SSID'/g' iot_configs.h
echo $NEWTIN | sudo -S sed -i 's/your-wifi-pwd/'$WIFI_PWD'/g' iot_configs.h
echo $NEWTIN | sudo -S sed -i 's@your-iothub-DEVICE-connection-string@'$IOTHUB_CONNECTION_DEVICE_STRING'@g' iot_configs.h
