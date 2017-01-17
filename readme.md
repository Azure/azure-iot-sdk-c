# Microsoft Azure IoT SDKs and libraries for C

This repository contains the following:
* **Microsoft Azure IoT Hub device SDK for C** to connect devices running C code to Azure IoT Hub
* **Microsoft Azure IoT Hub service SDK for C** to interface with an Azure IoT Hub service instance from a back-end C application
* **Serializer library for C** to help serialize and deserialize data on your device.

The C SDKs and library code:
* Is written in ANSI C (C99) to maximize code portability and broad platform compatibility.
* Avoids compiler extensions.
* In the device client SDK, the library exposes a platform abstraction layer to isolate OS dependencies (threading and mutual exclusion mechanisms, communications protocol e.g. HTTP). Refer to our [porting guide][c-porting-guide] for more information.

To find SDKs in other languages for Azure IoT, please refer to the [azure-iot-sdks][azure-iot-sdks] repository.

## Developing applications for Azure IoT

Visit [Azure IoT Dev Center][iot-dev-center] to learn more about developing applications for Azure IoT.

## How to clone the repository

The repository is using [GitHub Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) for its dependencies. In order to automatically clone these submodules, you need to use the --recursive option as described here:

```
git clone --recursive https://github.com/Azure/azure-iot-sdk-c.git 
```

## How to use the Azure IoT SDKs for C

* **Using packages and libraries**: the simplest way to use the Azure IoT SDKs is to use packages and libraries when available. The following are available:
  * On Linux (Ubuntu 14.04, 15.04, 15.10, 16.04): [Device SDK on apt-get](./iothub_client/readme.md#aptgetpackage)
  * On mbed:                                      [Device SDK library on MBED](./iothub_client/readme.md#mbed)
  * On Arduino:                                   [Device SDK library in the Arduino IDE](./iothub_client/readme.md#arduino)
  * On Windows:                                   [Device SDK on NuGet](./iothub_client/readme.md#nugetpackage)
* **Compiling the source code**: when no package or library is available for your platform or if you want to modify the SDKs code, or port the SDKs to a new platform, then you can leverage the build environement provided in the repository.
  * [Device SDK](./iothub_client/readme.md#compile)
  * [Service SDK](./iothub_service_client/readme.md#compile)

The API reference documentation for the C SDKs is [here][c-api-reference].

## samples

In the repository, you will find a set of simple samples that will help you get started:
* [Device SDK samples](./iothub_client/samples/)
* [Service SDK samples](./iothub_service_client/samples/)
* [Serializer library samples](./serializer/samples/)

## OS platforms and hardware compatibility

The IoT Hub device SDK for C can be used with a broad range of OS platforms and devices.
The minimum requirements are for the device platform to support the following:

- **Being capable of establishing an IP connection**: only IP-capable devices can communicate directly with Azure IoT Hub.
- **Support TLS**: required to establish a secure communication channel with Azure IoT Hub.
- **Support SHA-256** (optional): necessary to generate the secure token for authenticating the device with the service. Different authentication methods are available and not all require SHA-256.
- **Have a Real Time Clock or implement code to connect to an NTP server**: necessary for both establishing the TLS connection and generating the secure token for authentication.
- **Having at least 64KB of RAM**: the memory footprint of the SDK depends on the SDK and protocol used as well as the platform targeted. The smallest footprint is achieved targeting microcontrollers.

## Porting the Azure IoT device client SDK for C to new platforms

The C SDK is written in ANSI C (C99) to allow for it to run on a wide range of platforms.
In the repository you will find instructions and build tools to compile and run the device client SDK for C on Linux, Windows and microcontroller platforms (refer to the links above for more information on compiling the device client for C).
If you are considering porting the device client SDK for C to a new platform, check out the [porting guide][c-porting-guide] document.

## Contribution, feedback and issues

If you encounter any bugs, have suggestions for new features or if you would like to become an active contributor to this project please follow the instructions provided in the [contribution guidelines](CONTRIBUTING.md).

## Support

If you are having issues using one of the packages or using the Azure IoT Hub service that go beyond simple bug fixes or help requests that would be dealt within the issues section of this project, the Microsoft Customer Support team will try and help out on a best effort basis.
To engage Microsoft support, you can create a support ticket directly from the [Azure portal](https://ms.portal.azure.com/#blade/Microsoft_Azure_Support/HelpAndSupportBlade).
Escalated support requests for Azure IoT Hub SDKs development questions will only be available Monday thru Friday during normal coverage hours of 6 a.m. to 6 p.m. PST.
Here is what you can expect Microsoft Support to be able to help with:
- **SDKs issues**: If you are trying to compile and run the libraries on a supported platform, the Support team will be able to assist with troubleshooting or questions related to compiler issues and communications to and from the IoT Hub.  They will also try to assist with questions related to porting to an unsupported platform, but will be limited in how much assistance can be provided.  The team will be limited with trouble-shooting the hardware device itself or drivers and or specific properties on that device. 
- **IoT Hub / Connectivity Issues**: Communication from the device client to the Azure IoT Hub service and communication from the Azure IoT Hub service to the client.  Or any other issues specifically related to the Azure IoT Hub.
- **Portal Issues**: Issues related to the portal, that includes access, security, dashboard, devices, Alarms, Usage, Settings and Actions.
- **REST/API Issues**: Using the IoT Hub REST/APIs that are documented in the [documentation](https://docs.microsoft.com/en-us/rest/api/iothub/).

## Read more

* [Azure IoT Hub documentation][iot-hub-documentation]
* [Prepare your development environment to use the Azure IoT device SDK for C][devbox-setup]
* [Setup IoT Hub][setup-iothub]
* [Azure IoT device SDK for C tutorial][c-sdk-intro]
* [How to port the C libraries to other OS platforms][c-porting-guide]
* [Cross compilation example][c-cross-compile]
* [C SDKs API reference][c-api-reference]

--- 

## SDK folder structure

### /c-utility, /uamqp, /umqtt, /parson

These are git submodules that contain code, such as adapters and protocol implementations, shared with other projects. Note that some of them contain nested submodules.

### /blob

This folder contains client components that enable access to Azure blob storage.

### /certs

Contains certificates needed to communicate with Azure IoT Hub.

### /doc

This folder contains application development guides and device setup instructions.

### /build_all

This folder contains platform-specific build scripts for the client libraries and dependent components.

### /iothub_client

Contains Azure IoT Hub client components that provide the raw messaging capabilities of the library. Refer to the API documentation and samples for information on how to use it.

   * build: has one subfolder for each platform (e.g. Windows, Linux, Mbed). Contains makefiles, batch files, and solutions that are used to generate the library binaries.
   * devdoc: contains requirements, designs notes, manuals.
   * inc: public include files.
   * src: client libraries source files.
   * samples: contains simple samples.
   * tests: unit and end-to-end tests for source code.

### /serializer

Contains libraries that provide modeling and JSON serialization capabilities on top of the raw messaging library. These libraries facilitate uploading structured data and command and control for use with Azure IoT services. Refer to the API documentation and samples for information on how to use it.

   * build: has one subfolder for each platform (e.g. Windows, Linux, Mbed). Contains makefiles, batch files, and solutions that are used to generate the library binaries.
   * devdoc: contains requirements, designs notes, manuals.
   * inc: public include files.
   * src: client libraries source files.
   * samples: contains simple samples.
   * tests: unit tests and end-to-end tests for source code.

### /iothub_service_client

Contains libraries that enable interactions with the IoT Hub service to perform operations such as sending messages to devices and managing the device identity registry.

### /testtools

Contains tools that are currently used in testing the client libraries: Mocking Framework (micromock), Generic Test Runner (CTest), Unit Test Project Template, etc.

### /tools

Miscellaneous tools: compilembed, mbed_build, traceabilitytool (checks spec requirements vs code implementation).

---
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

[iot-dev-center]: http://azure.com/iotdev
[iot-hub-documentation]: https://docs.microsoft.com/en-us/azure/iot-hub/
[devbox-setup]: doc/devbox_setup.md
[setup-iothub]: https://aka.ms/howtocreateazureiothub
[c-sdk-intro]: https://azure.microsoft.com/documentation/articles/iot-hub-device-sdk-c-intro/
[c-porting-guide]: doc/porting_guide.md
[c-cross-compile]: doc/SDK_cross_compile_example.md
[c-api-reference]: http://azure.github.io/azure-iot-sdks/c/api_reference/index.html
