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

cp ../c-utility/inc/azure_c_shared_utility/azure_base64.h mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/c-utility/inc/azure_c_shared_utility/azure_base64.h
cp ../c-utility/src/azure_base64.c mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/c-utility/src/azure_base64.c
cp ../c-utility/src/base32.c mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/c-utility/src/base32_utility.c
cp ../provisioning_client/adapters/hsm_client_riot.c mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c

function fixMURenames
{
    inputfile=${1}
    tempfile='./fixMUren.txt'

    echo "Fixing MU_ renaming in $inputfile"

    perl -pe's/(?<!MU_)(ENUM_TO_STRING|DEFINE_ENUM_STRINGS)/MU_$1/g;' $inputfile > $tempfile
    mv $tempfile $inputfile
}

function muteX509MakeRootCertError
{
    echo "Muting the X509MakeRootCert error"

    inputfile='mbed-iot-devkit-sdk/cores/arduino/azure-iot-sdk-c/provisioning_client/adapters/hsm_client_riot.c'
    tempfile='./muteX509MakeRootCertError.txt'

    perl -pe's/X509MakeRootCert(\(\&der_ctx, tbs_sig\))?/0/g;' $inputfile > $tempfile
    mv $tempfile $inputfile
}

fixMURenames mbed-iot-devkit-sdk/libraries/AzureIoT/src/DevKitMQTTClient.cpp
fixMURenames mbed-iot-devkit-sdk/libraries/AzureIoT/src/DevkitDPSClient.cpp

muteX509MakeRootCertError

sudo iotz init mbed
sudo iotz compile

cd ..
