# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
sudo rm -R -f mxchip

git clone -b $MXCHIP_REPO_SOURCE_BRANCH https://github.com/massand/devkit-mbedos5-getstarted.git mxchip
cd mxchip
git submodule update --init --recursive
rsync -avz --existing ./ mbed-iot-devkit-sdk/components/azure-iot-sdk-c/

sudo npm install -g iotz
sudo iotz update

#sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c
#sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/DICE/DiceSha256.c
#sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotDerEnc.c
#sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotEcc.c
#sudo git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotSha256.c

sudo iotz init mbed
sudo iotz compile

cd ..
