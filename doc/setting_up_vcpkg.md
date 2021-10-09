# Building applications using vcpkg for C SDK

This document describes how to setup vcpkg to build applications using Microsoft Azure IoT device SDK for C. It demonstrates building and running a C SDK sample for Windows, Linux, and Mac.

## Setup C SDK vcpkg for Windows development environment

- Open PowerShell and run the following commands:

    ```powershell
    git clone https://github.com/Microsoft/vcpkg.git vcpkg_new
    pushd .\vcpkg_new\
    .\bootstrap-vcpkg.bat
    ```

    - Option 1: Install using Visual Studio integration.

        ```powershell
        # Install azure-iot-sdk-c package with integration
        .\vcpkg.exe integrate install
        .\vcpkg.exe install azure-iot-sdk-c
        popd

        # Open the C SDK telemetry sample solution for Visual Studio.
        git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
        pushd .\azure-iot-sdk-c\iothub_client\samples\iothub_ll_telemetry_sample\windows\
        start .\iothub_ll_telemetry_sample.sln
        ```

    - Option 2: Use the NuGet package manager in Visual Studio.

        ```powershell
        # Install azure-iot-sdk-c package
        .\vcpkg.exe install azure-iot-sdk-c

        # Export nuget package locally
        .\vcpkg.exe export azure-iot-sdk-c --nuget
        popd

        # Open the C SDK telemetry sample solution for Visual Studio.
        git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
        pushd .\azure-iot-sdk-c\iothub_client\samples\iothub_ll_telemetry_sample\windows\
        start .\iothub_ll_telemetry_sample.sln
        ```

        - With the sample solution open in Visual Studio, go to Tools->NuGet Package Manager->Package Manager Console and paste:

        ```
        Install-Package <package name provided from vcpkg export nuget command> -Source "C:\vcpkg_new"
        ```

- Hit F5 to build and run.

- More information on building the C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/devbox_setup.md).

## Setup C SDK vcpkg for Linux or Mac development environment

- Open a command prompt and run the following commands:

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git vcpkg_new
    cd vcpkg_new/
    ./bootstrap-vcpkg.sh
    ./vcpkg install azure-iot-sdk-c
    cd ..
    ```
- Enter the existing C SDK telemetry sample directory.

    ```bash
    git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
    cd azure-iot-sdk-c/iothub_client/samples/iothub_ll_telemetry_sample/linux/
    ```

- Open the `CMakeLists.txt` file and replace the current contents with the following:

    ```bash
    #Copyright (c) Microsoft. All rights reserved.
    #Licensed under the MIT license. See LICENSE file in the project root for full license information.

    cmake_minimum_required(VERSION 3.5)

    cmake_policy(SET CMP0074 NEW)

    project(telemetry_sample)

    if(WIN32)
        message(FATAL_ERROR "This CMake file only supports Linux builds!")
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

- Run the following commands:

    ```bash
    mkdir cmake
    cd cmake
    cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg_new/scripts/buildsystems/vcpkg.cmake
    cmake --build .
    ./iothub_ll_telemetry_sample
    ```

- More information on building the C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/devbox_setup.md).
