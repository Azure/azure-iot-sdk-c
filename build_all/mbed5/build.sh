@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

sudo npm install -g iotz
sudo iotz update

sudo rm -R -f mxchip
git clone -b rajeev/latest_mbed https://github.com/massand/mxchip_az3166_firmware.git mxchip
rsync -avz --existing ./ mxchip/mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/

cd mxchip
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/DICE/DiceSha256.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotDerEnc.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotEcc.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotSha256.c
sudo iotz init mbed
sudo iotz compile

cd ..
