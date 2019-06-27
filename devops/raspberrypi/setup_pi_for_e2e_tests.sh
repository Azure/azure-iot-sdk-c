#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

###################################################################################################
################################### PI E2E TEST SETUP GUIDE #######################################
###################################################################################################
# This is a guide to setting up you raspberry pi to run E2E tests. Essentially all this stuff is 
# creating the proper toolchain including upgrading cURL to the latest version so that the
# C Client Library can be able to use cURL without any issues like running out of memory. 
# 
# TO RUN THIS SCRIPT:
# Get a Raspberry Pi and flash the SD card with Stretch. 
# This script has been tested to work on a brand new Install of Stretch. Don't use it if you care
# about if your Stretch Image becomes unusable or not. Because it's messing with the curl stuff, 
# it might make things unusable... but it also probably won't. Still be cautious.   
###################################################################################################
###################################################################################################

MYDIR="$(dirname "$0")"

# ENABLE SSH BY DEFAULT 
if ! grep -Fxq "systemctl start ssh" /etc/rc.local
then
    echo 'Enabling automatic start of ssh on bootup...'
    #insert systemctl start ssh onto the 19th line of /etc/rc.local
    sudo sed -i '19i\systemctl start ssh' /etc/rc.local
    # code if not found
fi
# END ENABLE SSH BY DEFAULT

# GET NECESSARY PACKAGES
sudo apt-get update -y
sudo apt-get install -y vim git build-essential pkg-config git cmake libssl-dev uuid-dev valgrind

###################################################################################################
# DOWNLOAD AND INSTALL LINUX AGENT
# FOR WALKTHROUGH: https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/v2-linux?view=azure-devops
# NOTE: You need a Azure DevOps PAT Token. Described in the link above. 
###################################################################################################
cd ~
wget https://vstsagentpackage.azureedge.net/agent/2.150.0/vsts-agent-linux-arm-2.150.0.tar.gz
mkdir myagent && cd myagent
if [[ $? -ne 0 ]]; then
    echo "myagent already created, overwriting"
    rm -r myagent && mkdir myagent && cd myagent
fi
tar zxvf ~/vsts-agent-linux-arm-2.150.0.tar.gz
# Config will prompt for name of pool and PAT. 
./config.sh
./run.sh
cd ~ 

# ###################################################################################################
# # CREATE SYSTEMD SERVICE: devops
# # Installing the agent as a systemd service.
# # This needs a manual step of creating the devops-enviroment file in /etc/
# # Look at the template devops-environment to figure out the necessary credentials
# ###################################################################################################


sudo cp devops.service /lib/systemd/system/devops.service
mkdir -p "/home/pi/devops-logs" && touch "/home/pi/devops-logs/devopsagent.log"

# This step redirects the journalctl logs for devops.service to a user log file.
echo "CREATE DEVOPSAGENT.CONF"
touch /etc/rsyslog.d/devopsagent.conf
echo "if $programname == '<your program identifier>' then /home/pi/devops-logs/devopsagent.log" >> /etc/rsyslog.d/devopsagent.conf
echo "& stop" >> /etc/rsyslog.d/devopsagent.conf
sudo chown root:adm /home/pi/devops-logs/devopsagent.log

echo "NOTE: USER INPUT NECESSARY!"
echo "add credentials to devops-environment, then copy to /etc/devops-environment"
echo "Once you have added /etc/devops-environment file, run:"
echo "sudo systemctl daemon-reload"

read -r -p "Have you added your credentials to the devops_service.sh file? [y/N] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]
then
    echo "copying devops_service.sh to /home/pi/myagent/"
    cp $MYDIR/devops_service.sh /home/pi/myagent/devops_service.sh
else
    echo "Before you can run the daemon you must update your credentials."
    echo "Once the credentials have been updated then copy devops_service into myagent:"
    echo 'cp $MYDIR/devops_service.sh /home/pi/myagent/devops_service.sh'
fi


###################################################################################################
# INSTALL NEWEST VERSION OF CURL AND POINT TO THE NEW CURL LIBRARY
###################################################################################################
if ! grep -Fxq "LD_LIBRARY_PATH" ~/.bashrc
then 
    echo 'export LD_LIBRARY_PATH'
    echo 'export LD_LIBRARY_PATH=/usr/local/lib' >> ~/.bashrc
fi

printf 'Setting variables by calling bashrc'
source ~/.bashrc
[ $? -eq 0 ] || { echo "bashrc source failed"; exit 1; }

# Install curl new version
wget https://curl.haxx.se/download/curl-7.64.1.tar.gz

tar -xzvf curl-7.64.1.tar.gz
cd curl_source/curl-7.64.1/
./configure --without-zlib --with-ssl
make -j
sudo make install

sudo ln -sf /usr/local/bin/curl /usr/bin/curl

echo "Now you can run the cross compiled E2E tests!"


# ###################################################################################################
# # UNCOMMENT THE BELOW LINES AND COMMENT OUT THE CURL INSTALL LINES ABOVE IF YOU WANT TO
# # COMPILE THE SDK ON THE DEVICE AND RUN THE E2E TESTS
# ###################################################################################################
# # check if CURL_ROOT is already a variable set in bashrc...
# if ! grep -Fxq "CURL_ROOT" ~/.bashrc
# then
#     printf 'Exporting variable CURL_ROOT...'
#     echo 'export CURL_ROOT=/home/pi/curl_install' >> ~/.bashrc 
#     source ~/.bashrc
#     # code if not found
# fi

# if ! grep -Fxq "LD_LIBRARY_PATH" ~/.bashrc
# then 
#     echo 'export LD_LIBRARY_PATH'
#     echo 'export LD_LIBRARY_PATH=$CURL_ROOT/lib' >> ~/.bashrc
# fi

# printf 'Setting variables by calling bashrc'
# source ~/.bashrc
# [ $? -eq 0 ] || { echo "bashrc source failed"; exit 1; }


# # BUILD AND INSTALL NEW CURL TO CURL_ROOT
# # We are commenting this out because it's useful info to have 
# # but for the E2E tests it works when CURL is installed directly on the device.
# wget https://curl.haxx.se/download/curl-7.64.1.tar.gz
# mkdir $CURL_ROOT
# mkdir curl_source
# tar -C curl_source -xzvf curl-7.64.1.tar.gz
# cd curl_source/curl-7.64.1/
# ./configure --prefix=$CURL_ROOT --disable-shared --without-zlib --with-ssl --enable-static
# make -j
# sudo make install

# # END GET NECESARY PACKAGES 

# # BUILD THE SDK
# cd ~
# git clone https://github.com/Azure/azure-iot-sdk-c
# git submodule update --init
# cd azure-iot-sdk-c
# if [ -d cmake ]   # for file "if [-f /home/rama/file]" 
# then 
#     echo "cmake present"
#     rm -rf cmake 
# else
#     echo "cmake not present"
# fi
# mkdir cmake && cd cmake
# cmake -Drun_unittests=ON -Drun_e2e_tests=ON -DCMAKE_BUILD_TYPE=Debug -DCURL_LIBRARY=$CURL_ROOT/lib/libcurl.a -DCURL_INCLUDE_DIR=$CURL_ROOT/include  ..
# cd iothub_client/tests/iothubclient_mqtt_e2e
# cmake --build .
# sudo -E su
# setcap cap_net_raw,cap_net_admin+ep iothubclient_mqtt_e2e_exe
# ./iothubclient_mqtt_e2e_exe
# # END RUN THE E2E TEST


