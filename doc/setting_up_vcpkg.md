# Building applications using vcpkg for C SDK

This document describes how to setup vcpkg to build applications using Microsoft Azure IoT device C SDK. It demonstrates building and running a C language SDK sample for Windows, Linux, and Mac.

## C SDK vcpkg support

The C SDK uses vcpkg primarily for LTS releases. The most recent LTS release can be installed using the syntax: `vcpkg install azure-iot-sdk-c` and sets the following CMake flags:

```
    -Dskip_samples=ON
    -Duse_installed_dependencies=ON
    -Duse_default_uuid=ON
    -Dbuild_as_dynamic=OFF
    -Duse_edge_modules=ON
    -Dwarnings_as_errors=OFF
```

The `use-prov-client` feature uses the syntax: `vcpkg install azure-iot-sdk-c[use-prov-client]` and adds the following CMake flags:
```
    -Dhsm_type_symm_key=ON
    -Duse_prov_client=ON
```

The `public-preview` feature allows you to build and install the azure-iot-sdk-c from a provided public-preview hash. To access this feature, use the syntax: `vcpkg install azure-iot-sdk-c[public-preview]`. The following CMake flags will be set:

```
    -Dskip_samples=ON
    -Duse_installed_dependencies=ON
    -Duse_default_uuid=ON
    -Dbuild_as_dynamic=OFF
    -Duse_edge_modules=ON
    -Dwarnings_as_errors=OFF
```

There are no other features available.

> NOTE: If your application requires specific CMake flags to be set, please build and install directly from the source code.  See [devbox_update.md](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md) for further information.

The most recent portfile.cmake and vcpkg.json files for the azure-iot-sdk-c vcpkg port can be found at [vcpkg/ports/azure-iot-sdk-c](https://github.com/microsoft/vcpkg/tree/master/ports/azure-iot-sdk-c).


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
