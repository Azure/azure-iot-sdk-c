#!/usr/bin/env bash

# https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md#linux

sudo apt-get update -y
sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates
mkdir cmake
cd cmake
cmake ..
cmake --build . -- -j$(nproc)
