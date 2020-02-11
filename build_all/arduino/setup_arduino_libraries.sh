#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
echo $NEWTIN | sudo -S rm -r azure-iot-pal-arduino
git clone https://github.com/Azure/azure-iot-pal-arduino.git 

cd azure-iot-pal-arduino
git submodule update --init --recursive
rsync -avz --existing ./ sdk/
