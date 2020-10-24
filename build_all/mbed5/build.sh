# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
sudo rm -R -f mxchip

git clone -b $MXCHIP_REPO_SOURCE_BRANCH https://$DEVKIT_MBEDOS5_GETSTARTED_USER:$DEVKIT_MBEDOS5_GETSTARTED_CRED@azure-iot-sdks.visualstudio.com/azure-iot-sdk-c-build/_git/devkit-mbedos5-getstarted mxchip
cd mxchip

// update mbed-os and mbed-iot-devkit-sdk with credentials
echo $NEWTIN | sudo -S sed -i 's&eric&'$MBED_IOT_DEVKIT_SDK_USER:$MBED_IOT_DEVKIT_SDK_CRED'&g' .gitmodules
echo $NEWTIN | sudo -S sed -i 's&placehold&'$MBED_USER:$MBED_CRED'&g' .gitmodules

git submodule update --init
cd mbed-iot-devkit-sdk

// update devkit-azure-iot-sdk-c and mbed-az3166-driver with credentials
echo $NEWTIN | sudo -S sed -i 's&eric&'$DEVKIT_SDK_USER:$DEVKIT_SDK_CRED'&g' .gitmodules
echo $NEWTIN | sudo -S sed -i 's&mseng@&'$DEVKIT_USER:$DEVKIT_CRED@'&g' .gitmodules
echo $NEWTIN | sudo -S sed -i 's&placehold&'$AZ3166_USER:$AZ3166_CRED'&g' .gitmodules

git submodule update --init --recursive
cd ..
rsync -avz --existing ./ mbed-iot-devkit-sdk/components/azure-iot-sdk-c/

sudo npm install -g iotz
sudo iotz update

sudo iotz init mbed
sudo iotz compile

cd ..
