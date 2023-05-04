# Using vcpkg for the Azure IoT C SDK to build device applications

This document describes how to setup vcpkg to build device applications using Microsoft Azure IoT C SDK. It demonstrates building and running a C language device SDK sample for Windows, Linux, and Mac.

## Azure IoT C SDK vcpkg support

The Azure IoT C SDK uses [vcpkg](https://github.com/microsoft/vcpkg) primarily for LTS releases. The most recent LTS release can be installed using the syntax: `vcpkg install azure-iot-sdk-c` and builds using the following CMake flags:

```
    -Dskip_samples=ON
    -Duse_installed_dependencies=ON
    -Duse_default_uuid=ON
    -Dbuild_as_dynamic=OFF
    -Duse_edge_modules=ON
    -Dwarnings_as_errors=OFF
```

There are no other configuration options via vcpkg available.

> NOTE: If your application requires specific CMake flags not shown above, please build and install directly from the source code.  See [devbox_setup.md](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md) for further information.

The most recent [portfile.cmake](https://github.com/microsoft/vcpkg/blob/master/ports/azure-iot-sdk-c/portfile.cmake) and [vcpkg.json](https://github.com/microsoft/vcpkg/blob/master/ports/azure-iot-sdk-c/vcpkg.json) files for the Azure IoT C SDK can be found under [vcpkg/ports/azure-iot-sdk-c](https://github.com/microsoft/vcpkg/tree/master/ports/azure-iot-sdk-c).


## Setup Azure IoT C SDK vcpkg for Windows development environment

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
        .\vcpkg.exe install azure-iot-sdk-c:x64-windows
        popd

        # Open the Azure IoT C SDK telemetry sample solution for Visual Studio.
        git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
        pushd .\azure-iot-sdk-c\iothub_client\samples\iothub_ll_telemetry_sample\windows\
        start .\iothub_ll_telemetry_sample.sln
        ```

    - Option 2: Use the NuGet package manager in Visual Studio.

        ```powershell
        # Install azure-iot-sdk-c package
        .\vcpkg.exe install azure-iot-sdk-c:x64-windows

        # Export nuget package locally
        .\vcpkg.exe export azure-iot-sdk-c:x64-windows --nuget
        popd

        # Open the Azure IoT C SDK telemetry sample solution for Visual Studio.
        git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
        pushd .\azure-iot-sdk-c\iothub_client\samples\iothub_ll_telemetry_sample\windows\
        start .\iothub_ll_telemetry_sample.sln
        ```

        - With the sample solution open in Visual Studio, go to Tools->NuGet Package Manager->Package Manager Console and paste:

        ```
        Install-Package <package name provided from vcpkg export nuget command> -Source "C:\vcpkg_new"
        ```

- Hit F5 to build and run.

- More information on building the Azure IoT C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md).

## Setup Azure IoT C SDK vcpkg for Linux or Mac development environment

- Open a command prompt and run the following commands:

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git vcpkg_new
    cd vcpkg_new/
    ./bootstrap-vcpkg.sh
    ./vcpkg install azure-iot-sdk-c
    cd ..
    ```
- Enter the Azure IoT C SDK telemetry sample directory.

    ```bash
    git clone https://github.com/Azure/azure-iot-sdk-c.git # Skip if already have a cloned repo
    cd azure-iot-sdk-c/iothub_client/samples/iothub_ll_telemetry_sample/linux/
    mkdir cmake
    cd cmake
    cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg_new/scripts/buildsystems/vcpkg.cmake
    cmake --build .
    ./iothub_ll_telemetry_sample
    ```

- More information on building the Azure IoT C SDK and samples can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md).
