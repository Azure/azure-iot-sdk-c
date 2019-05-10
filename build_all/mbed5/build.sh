# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
sudo rm -R -f mxchip
git clone -b rajeev/latest_mbed https://github.com/massand/mxchip_az3166_firmware.git mxchip
rsync -avz --existing ./ mxchip/mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/

sudo npm install -g iotz
sudo iotz update

cd mxchip
sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c
sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/DICE/DiceSha256.c
sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotDerEnc.c
sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotEcc.c
sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotSha256.c

sudo iotz init mbed
sudo iotz compile

cd ..
