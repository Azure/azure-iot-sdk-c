sudo npm install -g iotz
sudo iotz update
sudo rm -R -f mxchip
git clone https://github.com/obastemur/mxchip_az3166_firmware.git mxchip
rsync -avz --existing ./ mxchip/mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/
cp -r provisioning_client/inc/azure_prov_client/internal mxchip/mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/inc/azure_prov_client/
cd mxchip
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/DICE/DiceSha256.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotDerEnc.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotEcc.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/deps/RIoT/Reference/RIoT/Core/RIoTCrypt/RiotSha256.c
git checkout -- mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c
cp ../c-utility/src/base64.c mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/c-utility/src/base64_utility.c
cp ../c-utility/src/base32.c mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/c-utility/src/base32_utility.c
sudo iotz init mbed
sudo iotz compile
cd ..
