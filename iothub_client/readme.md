# Microsoft Azure IoT device SDK for C

This folder contains the following:

* **The Azure IoT device SDK for C** ([prepackaged](#aptgetpackage) or [to compile](#compile)) to easily and securely connect devices to the Microsoft Azure IoT Hub service.
* [**Samples**](#samples) showing how to use the SDK

## C Device SDK Features

* Sends event data to Azure IoT based services.
* Maps server commands to device functions.
* Buffers data when the network connection is down.
* Implements configurable retry logic.
* Batches messages to improve communication efficiency.
* Supports pluggable transport protocols: HTTPS, AMQP and MQTT.
* Supports pluggable serialization methods.

Azure IoT device SDK for C can be used with a broad range of OS platforms and devices.
For a list of tested configurations [click here][device-catalog].

---

## Prepackaged C SDK for Platform Specific Development

<a name="cocoapods"></a>

### CocoaPods for Apple devices

The IoT Hub Device SDK is available as [CocoaPods](https://cocoapods.org/) for Mac and iOS device development.
Details for how to use the CocoaPods are available [here](/iothub_client/samples/ios).

   **iOS Limitations**

  - Authentication is limited to SAS keys on iOS. No certificate-based authentication is officially supported.
  - The Device Provisioning Client is not supported on iOS. Only the Azure IoT Hub device client is supported.

  For a more complete **iOS experience** including the two missing features above, please see our sample [native Swift library](https://github.com/Azure-Samples/azure-sdk-for-c-swift) built on top of the Embedded C SDK.

<a name="vcpkgpackage"></a>

### Vcpkg packages for Windows devices

When developing for Windows devices, you can leverage the Vcpkg package manager in order to easily reference the Microsoft Azure IoTHub C SDK libraries in your projects.
The Windows [samples in this repository][samples] show how to use the azure-iot-sdk-c Vcpkg package along with its dependencies in your C project.
To install Microsoft Azure IoTHub vcpkg, follow the instructions at [Setup C SDK vcpkg for Windows development environment](/doc/setting_up_vcpkg.md#setup-c-sdk-vcpkg-for-windows-development-environment)

<a name="arduino"></a>

### Arduino IDE library

If you are developing on Arduino, you can leverage the Azure IoT library available in the Arduino IDE library manager.
You can find the list of supported boards as well as the instructions for using the library on Arduino devices in the [azure-iot-arduino GitHub repository](https://aka.ms/arduino) directly.

<a name="compile"></a>

## Compiling the C Device SDK

In order to compile the C SDK on your own, you will need to install a set of tools depending on the platform you are doing your development on and the one you are targeting.  You will also need to clone the current repository.
Detailed instructions can be found below for each platforms:

* [Setting up a Windows development environment](../doc/devbox_setup.md#windows)
* [Setting up a Linux development environment](../doc/devbox_setup.md#linux)
* [Setting up a Mac OS X development environment](../doc/devbox_setup.md#macos)

<a name="samples"></a>

## CMake

The C device SDK uses [CMake](https://cmake.org/) for compiler independent configuration and generates native build files and workspaces that can be used in the compiler environment of your choice.

* [SDK CMake integration with your application](../doc/how_to_use_azure_iot_sdk_c_with_cmake.md)

## Samples

The repository contains a set of simple samples that will help you get started.
You can find a list of these samples with instructions on how to run them [here][samples]. 
In addition to the simple samples found in the current repository, you can find detailed instructions for the certified for Azure IoT devices in our online [catalog][device-catalog]

## Read more

* [Azure IoT Hub documentation][iot-hub-documentation]
* [Prepare your development environment to use the Azure IoT device SDK for C][devbox-setup]
* [Setup IoT Hub][setup-iothub]
* [Azure IoT device SDK for C tutorial][c-sdk-intro]
* [How to port the C libraries to other OS platforms][c-porting-guide]
* [C SDKs API reference][c-api-reference]


[iot-dev-center]: http://azure.com/iotdev
[iot-hub-documentation]: https://docs.microsoft.com/azure/iot-hub/
[device-catalog]: https://catalog.azureiotsuite.com
[devbox-setup]: ../doc/devbox_setup.md
[setup-iothub]: https://aka.ms/howtocreateazureiothub
[c-sdk-intro]: https://azure.microsoft.com/documentation/articles/iot-hub-device-sdk-c-intro/
[c-porting-guide]: https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md
[c-cross-compile]: ../doc/SDK_cross_compile_example.md
[c-api-reference]: https://azure.github.io/azure-iot-sdk-c
[samples]: ./samples/
