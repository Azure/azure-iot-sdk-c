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


<a name="aptgetpackage"></a>

### apt-get packages for Linux devices

To make it simpler to use the IoT Hub device SDK on Linux, we have created [apt-get packages][apt-get-packages] that are published on the Launchpad platform.

At this point you can use the packages on Ubuntu 16.04 and 18.04 using the following CPU architectures: amd64, arm64, armhf and i386.

[Here][apt-get-instructions] you can find a detailed guide on how to install the packages to develop your device application.

If you are working with a device running a Linux distribution not supporting these packages, then you will need to compile the SDK following the instructions [below](#compile).

<a name="cocoapods"></a>

### CocoaPods for Apple devices

The IoT Hub Device SDK is available as [CocoaPods](https://cocoapods.org/) for Mac and iOS device development.
Details for how to use the CocoaPods are available [here](/iothub_client/samples/ios).

<a name="vcpkgpackage"></a>

### Vcpkg packages for Windows devices

When developing for Windows devices, you can leverage the Vcpkg package manager in order to easily reference the Microsoft Azure IoTHub C SDK libraries in your projects.
The Windows [samples in this repository][samples] show how to use the azure-iot-sdk-c Vcpkg package along with its dependencies in your C project.
To install Microsoft Azure IoTHub vcpkg, follow the instructions at [Setup C SDK vcpkg for Windows development environment](/doc/setting_up_vcpkg.md#setup-c-sdk-vcpkg-for-windows-development-environment)

<a name="mbed"></a>

### mbed library for mbed Developer Workspace

For developers creating device applications on the [mbed](http://mbed.org) platform, we have published a library and samples that will get you started in minutes witH Azure IoT Hub. This library and the samples have been tested with the following boards:

* Freescale FRDMK64-F
* Renesas GR-PEACH
* SADE.IO GSM Gateway

To use the samples and the Azure IoT device SDK library in your mbed applications, here are the basic steps:

* Prepare your device as instructed by the device manufacturer to connect it to the mbed development environment
* In the [mbed Developer Workspace](https://developer.mbed.org/compiler/) click **Import** on the main menu. Then click the **Click here to import from URL** link next to the mbed globe logo.
* In the popup window, enter the link for the sample code you want to try (you can find Azure IoT Hub samples [here](https://developer.mbed.org/users/AzureIoTClient/code/)).
* Adapt the code to use the right credentials for your device, and click **Compile** to generate the binary for your board.
* Download the binary to your device and run.

You can find detailed instructions for each of the tested devices in the Azure IoT [device catalog][device-catalog]:

* [Freescale FRDMK64-F](../doc/mbed_get_started.md)
* [Renesas GR-PEACH](https://catalog.azureiotsuite.com/details?title=GR_Peach-_-Renesas-Electronics-RZA1H-on-board&source=home-page)
* [SADE.IO GSM Gateway](https://catalog.azureiotsuite.com/details?title=SADE-IoT-Cloud-Family-_-GSM-Gateway&source=home-page)

<a name="arduino"></a>

### Arduino IDE library

If you are developing on Arduino, you can leverage the Azure IoT library available in the Arduino IDE library manager.
You can find the list of supported boards as well as the instructions for using the library on Arduino devices in the [azure-iot-arduino GitHub repository](https://github.com/azure/azure-iot-arduino) directly.

<a name="compile"></a>

## Compiling the C Device SDK

In order to compile the C SDK on your own, you will need to install a set of tools depending on the platform you are doing your development on and the one you are targeting.  You will also need to clone the current repository.
Detailed instructions can be found below for each platforms:

* [Setting up a Windows development environment](../doc/devbox_setup.md#windows)
* [Setting up a Linux development environment](../doc/devbox_setup.md#linux)
* [Setting up a Mac OS X development environment](../doc/devbox_setup.md#macos)
* [Cross compile the C device SDK (targeting Raspbian and using Ubuntu as host)](../doc/SDK_cross_compile_example.md)

<a name="samples"></a>

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
* [Cross compilation example][c-cross-compile]
* [C SDKs API reference][c-api-reference]


[iot-dev-center]: http://azure.com/iotdev
[iot-hub-documentation]: https://docs.microsoft.com/en-us/azure/iot-hub/
[device-catalog]: https://catalog.azureiotsuite.com
[devbox-setup]: ../doc/devbox_setup.md
[setup-iothub]: https://aka.ms/howtocreateazureiothub
[c-sdk-intro]: https://azure.microsoft.com/documentation/articles/iot-hub-device-sdk-c-intro/
[c-porting-guide]: https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md
[c-cross-compile]: ../doc/SDK_cross_compile_example.md
[c-api-reference]: https://azure.github.io/azure-iot-sdk-c
[apt-get-instructions]: ../doc/ubuntu_apt-get_sample_setup.md
[apt-get-packages]: https://launchpad.net/~aziotsdklinux/+archive/ubuntu/ppa-azureiot
[samples]: ./samples/
