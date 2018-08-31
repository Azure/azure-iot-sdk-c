# Remote Monitoring solution accelerator sample

This sample illustrates how to connect to the Remote Monitoring solution accelerator to:

- Report device capabilities
- Send telemetry
- Respond to methods, including a long-running firmware update method

For more information about this sample, see:

- [Connect your device to the Remote Monitoring solution accelerator (Windows)](https://docs.microsoft.com/azure/iot-accelerators/iot-accelerators-connecting-devices)
- [Connect your device to the Remote Monitoring solution accelerator (Linux)](https://docs.microsoft.com/azure/iot-accelerators/iot-accelerators-connecting-devices-linux)

## Build and run in Windows

This sample includes a Visual Studio 2017 solution file that uses NuGet to install the prerequisite packages. To build the application using Visual Studio 2017:

1. Clone or download this repository to your Windows machine.
1. Download the latest parson.c and parson.h files from https://github.com/kgabis/parson and save them in the sample folder alongside the RemoteMonitoring.c file.
1. Open the RemoteMonitoringClient.sln file in the Windows folder in Visual Studio 2017.
1. Add your device connection string to to the RemoteMonitoring.c file.
1. Build and run the the RemoteMonitoringClient project.

## Build and run in Linux

This sample includes a CMakeLists.txt file that you can use to build the application. It uses the azure-iot-sdk-c-dev package so you don't need to build the SDK. To build the application on an Ubuntu machine:

1. Clone or download this repository to your Ubuntu machine.
1. Download the latest parson.c and parson.h files from https://github.com/kgabis/parson and save them in the sample folder alongside the RemoteMonitoring.c file.
1. Follow these instructions to install the *azure-iot-sdk-c-dev* package: [Use apt-get to create a C device client project on Ubuntu](../../../doc/ubuntu_apt-get_sample_setup.md).
1. Add your device connection string to to the RemoteMonitoring.c file.
1. Navigate to the linux folder in the sample folder.
1. Run the following commands to build the sample:

    ```
    mkdir cmake
    cd cmake
    cmake ..
    make
    ```
1. To run the sample:

    ```bash
    ./RemoteMonitoringClient
    ```