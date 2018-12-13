This document describes how to prepare your development environment to use the Microsoft Azure IoT device SDK for C. It describes preparing a development environment in Windows using Visual Studio and in Linux.

### Setup C SDK vcpkg for Windows development environment

- Open a command prompt and run the following commands:

```Shell

# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git vcpkg_new

# Bootstrap and install Visual Studio integration
pushd vcpkg_new
.\bootstrap-vcpkg.bat
vcpkg integrate install

# Install azure-iot-sdk-c package
vcpkg install azure-iot-sdk-c

# Export nuget package locally
vcpkg export azure-iot-sdk-c --nuget

# Open C SDK sample (assuming repo was cloned as follows: git clone https://github.com/Azure/azure-iot-sdk-c.git azureiotsdk_sample)
pushd azureiotsdk_sample\iothub_client\samples\iothub_ll_c2d_sample\windows
start iothub_ll_c2d_sample.sln
```

- In Visual Studio, open Nuget package manager console
```Shell
Install-Package <package name provided by vcpkg export command> -Source "d:\d\vcpkg_new"
<hit F5 to build and run>
```

### Setup C SDK vcpkg for Linux development environment	
		
- Within iothub_client/samples/iothub_ll_telemetry_sample/linux/CMakeLists.txt, replace the current contents with this
```Shell
#Copyright (c) Microsoft. All rights reserved.	
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_ll_telemetry_sample
cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")
find_package(azure_iot_sdks REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${AZUREIOT_INC_FOLDER})

set(iothub_c_files
../iothub_ll_telemetry_sample.c
../../../../certs/certs.c
)

add_definitions(-DUSE_HTTP)
add_definitions(-DUSE_AMQP)
add_definitions(-DUSE_MQTT)

add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
include_directories("../../../../certs")

add_executable(iothub_ll_telemetry_sample ${iothub_c_files})
find_library(CURL NAMES curl-d curl)
target_link_libraries(iothub_ll_telemetry_sample
iothub_client_mqtt_transport
iothub_client_amqp_transport
iothub_client
parson
aziotsharedutil
umqtt
uuid

${CURL}
pthread
ssl
crypto
m
ZLIB::ZLIB
)
```

- Run following commands in the newly create cmake directory:
```Shell
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg_new/scripts/buildsystems/vcpkg.cmake
make
./iothub_ll_telemetry_sample
```
