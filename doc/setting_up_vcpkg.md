# Building applications using vcpkg for C SDK

This document describes how to setup vcpkg to build applications using Microsoft Azure IoT device C SDK. It demonstrates building and running a C language SDK sample for Windows, Linux, and Mac.

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

- More information on building the C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md).

## Setup C SDK vcpkg for Linux or Mac development environment

- Open a command prompt and run the following commands:

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git vcpkg_new
    cd vcpkg_new/
    ./bootstrap-vcpkg.sh
    ./vcpkg install azure-iot-sdk-c
    cd ..
    ```
- Enter the C SDK telemetry sample directory.

    ```bash
    git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
    cd azure-iot-sdk-c/iothub_client/samples/iothub_ll_telemetry_sample/linux/
    mkdir cmake
    cd cmake
    cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg_new/scripts/buildsystems/vcpkg.cmake
    cmake --build .
    ./iothub_ll_telemetry_sample
    ```

- More information on building the C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md).
