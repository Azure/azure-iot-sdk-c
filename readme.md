# Azure IoT C SDKs and Libraries

[![Build Status](https://azure-iot-sdks.visualstudio.com/azure-iot-sdks/_apis/build/status/c/integrate-into-repo-C)](https://azure-iot-sdks.visualstudio.com/azure-iot-sdks/_build/latest?definitionId=85)

This repository contains the following:
* **Azure IoT Hub Device C SDK** to connect devices running C code to Azure IoT Hub.
* **Azure IoT Hub Device Provisioning Service Client SDK** for enrolling devices with [Azure IoT Device Provisioning Services](https://docs.microsoft.com/azure/iot-dps/) and managing enrollments lists.
* **Azure IoT Hub Service C SDK** to interface with an Azure IoT Hub service instance from a server-side C application.
* **Serializer Library for C** to help serialize and deserialize data on your device.

## Packages and Libraries
  The simplest way to get started with the Azure IoT SDKs is to use the following packages and libraries:
  * Linux: [Device SDK on apt-get](./iothub_client/readme.md#aptgetpackage)
  * mbed:                                      [Device SDK library on MBED](./iothub_client/readme.md#mbed)
  * Arduino:                                   [Device SDK library in the Arduino IDE](./iothub_client/readme.md#arduino)
  * Windows:                                   [Device SDK on Vcpkg](./doc/setting_up_vcpkg.md#setup-c-sdk-vcpkg-for-windows-development-environment)
  * iOS:                                       [Device SDK on CocoaPod](https://cocoapods.org/pods/AzureIoTHubClient)

## Samples
  Here are a set of simple samples that will help you get started:
  * [Device SDK Samples](./iothub_client/samples/)
  * [Service SDK Samples](./iothub_service_client/samples/)
  * [Serializer Library Samples](./serializer/samples/)

## Compile the SDK

When no package or library is available for your platform or if you want to modify the SDK code, or port the SDK to a new platform, then you can leverage the build environment provided in the repository.
  * [Device SDK](./iothub_client/readme.md#compile)
  * [Service SDK](./iothub_service_client/readme.md#compile)

## SDK API Reference Documentation

The API reference documentation for the C SDKs can be found [here][c-api-reference].

## Other Azure IoT SDKs

To find Azure IoT SDKs in other languages, please refer to the [azure-iot-sdks][azure-iot-sdks] repository.

## Developing Azure IoT Applications

To learn more about building Azure IoT Applications, you can visit the [Azure IoT Dev Center][iot-dev-center].

## Key Features and Roadmap

### Device Client SDK
:heavy_check_mark: feature available  :heavy_multiplication_x: feature planned but not supported  :heavy_minus_sign: no support planned

| Features                                                                                                         | mqtt                | mqtt-ws             | amqp                     | amqp-ws                  | https                    | Description                                                                                                                                                                                                                                                                                                       |
|------------------------------------------------------------------------------------------------------------------|---------------------|---------------------|--------------------------|--------------------------|--------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [Authentication](https://docs.microsoft.com/azure/iot-hub/iot-hub-security-deployment)                     | :heavy_check_mark:  | :heavy_check_mark:* | :heavy_check_mark:       | :heavy_check_mark:*      | :heavy_check_mark:*      | Connect your device to IoT Hub securely with supported authentication, including private key, SASToken, X-509 Self Signed and Certificate Authority (CA) Signed.  *IoT Hub only supports X-509 CA Signed over AMQP and MQTT at the moment.                                                                        |
| [Send device-to-cloud message](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-d2c)     | :heavy_check_mark:* | :heavy_check_mark:* | :heavy_check_mark:*      | :heavy_check_mark:*      | :heavy_check_mark:*      | Send device-to-cloud messages (max 256KB) to IoT Hub with the option to add custom properties.  IoT Hub only supports batch send over AMQP and HTTPS only at the moment.  This SDK supports batch send over HTTP.  * Batch send over AMQP and AMQP-WS, and add system properties on D2C messages are in progress. |
| [Receive cloud-to-device messages](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-c2d) | :heavy_check_mark:* | :heavy_check_mark:* | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:       | Receive cloud-to-device messages and read associated custom and system properties from IoT Hub, with the option to complete/reject/abandon C2D messages.  *IoT Hub supports the option to complete/reject/abandon C2D messages over HTTPS and AMQP only at the moment.                                            |
| [Device Twins](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins)                     | :heavy_check_mark:* | :heavy_check_mark:* | :heavy_check_mark:*      | :heavy_check_mark:*      | :heavy_minus_sign:       | IoT Hub persists a device twin for each device that you connect to IoT Hub.  The device can perform operations like get twin tags, subscribe to desired properties.  *Send reported properties version and desired properties version are in progress.                                                            |
| [Direct Methods](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-direct-methods)                 | :heavy_check_mark:  | :heavy_check_mark:  | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_minus_sign:       | IoT Hub gives you the ability to invoke direct methods on devices from the cloud.  The SDK supports handler for method specific and generic operation.                                                                                                                                                            |
| [Upload file to Blob](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-file-upload)               | :heavy_minus_sign:  | :heavy_minus_sign:  | :heavy_minus_sign:       | :heavy_minus_sign:       | :heavy_check_mark:       | A device can initiate a file upload and notifies IoT Hub when the upload is complete.   File upload requires HTTPS connection, but can be initiated from client using any protocol for other operations.                                                                                                          |
| [Connection Status and Error reporting](https://docs.microsoft.com/rest/api/iothub/common-error-codes)     | :heavy_check_mark:* | :heavy_check_mark:* | :heavy_check_mark:*      | :heavy_check_mark:*      | :heavy_multiplication_x: | Error reporting for IoT Hub supported error code.  *This SDK supports error reporting on authentication and Device Not Found.                                                                                                                                                                                 |
| Retry policies                                                                                                   | :heavy_check_mark:* | :heavy_check_mark:* | :heavy_check_mark:*      | :heavy_check_mark:*      | :heavy_multiplication_x: | Retry policy for unsuccessful device-to-cloud messages have two options: no try, exponential backoff with jitter (default).   *Custom retry policy is in progress.                                                                                                                                              |
| Devices multiplexing over single connection                                                                      | :heavy_minus_sign:  | :heavy_minus_sign:  | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:       |                                                                                                                                                                                                                                                                                                                   |
| Connection Pooling - Specifying number of connections                                                            | :heavy_minus_sign:  | :heavy_minus_sign:  | :heavy_multiplication_x: | :heavy_multiplication_x: | :heavy_multiplication_x: |                                                                                                                                                                                                                                                                                                                   |


This SDK also contains options you can set and platform specific features.  You can find detail list in this [document](./doc/Iothub_sdk_options.md).


### Service Client SDK
:heavy_check_mark: feature available  :heavy_multiplication_x: feature planned but not supported  :heavy_minus_sign: no support planned

| Features                                                                                                      | C                  | Description                                                                                                                        |
|---------------------------------------------------------------------------------------------------------------|--------------------|------------------------------------------------------------------------------------------------------------------------------------|
| [Identity registry (CRUD)](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-identity-registry) | :heavy_check_mark:*                | Use your backend app to perform CRUD operation for individual device or in bulk.  This SDK supports CRUD operation on individual device with create device from ID/Key pair only.                                                    |
| [Cloud-to-device messaging](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-c2d)     | :heavy_check_mark: | Use your backend app to send cloud-to-device messages in AMQP and AMQP-WS, and set up cloud-to-device message receivers.           |
| [Direct Methods operations](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-direct-methods)   | :heavy_check_mark: | Use your backend app to invoke direct method on device.                                                                            |
| [Device Twins operations](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins)       | :heavy_check_mark:* | Use your backend app to perform device twin operations.  *Twin reported property update callback and replace twin are in progress. |
| [Query](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-query-language)                       | :heavy_multiplication_x: | Use your backend app to perform query for information.                                                                             |
| [Jobs](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-jobs)                                  | :heavy_multiplication_x:                | Use your backend app to perform job operation.                                                                                     |
| [File Upload](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-file-upload)                    | :heavy_multiplication_x:                | Set up your backend app to send file upload notification receiver.                                                                 |

### Provisioning client SDK
This repository contains [provisioning client SDK](./provisioning_client) for the [Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/).  

:heavy_check_mark: feature available  :heavy_multiplication_x: feature planned but not supported  :heavy_minus_sign: no support planned


| Features                    | mqtt               | mqtt-ws            | amqp               | amqp-ws            | https              | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-----------------------------|--------------------|--------------------|--------------------|--------------------|--------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| TPM Individual Enrollment   | :heavy_minus_sign: | :heavy_minus_sign: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | This SDK supports connecting your device to the Device Provisioning Service via [individual enrollment](https://docs.microsoft.com/azure/iot-dps/concepts-service#enrollment) using [Trusted Platform Module](https://docs.microsoft.com/azure/iot-dps/concepts-security#trusted-platform-module-tpm).  This [quickstart](https://docs.microsoft.com/azure/iot-dps/quick-create-simulated-device) reviews how to create a simulated device for individual enrollment with TPM. TPM over MQTT is currently not supported by the Device Provisioning Service.                                                                                                                                                                                                               |
| X.509 Individual Enrollment | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | This SDK supports connecting your device to the Device Provisioning Service via [individual enrollment](https://docs.microsoft.com/azure/iot-dps/concepts-service#enrollment) using [X.509 leaf certificate](https://docs.microsoft.com/azure/iot-dps/concepts-security#leaf-certificate).  This [quickstart](https://docs.microsoft.com/azure/iot-dps/quick-create-simulated-device-x509) reviews how to create a simulated device for individual enrollment with X.509. |
| X.509 Enrollment Group      | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | This SDK supports connecting your device to the Device Provisioning Service via [enrollment group](https://docs.microsoft.com/azure/iot-dps/concepts-service#enrollment) using [X.509 root certificate](https://docs.microsoft.com/azure/iot-dps/concepts-security#root-certificate).                                                                                                                                                                                         |

### Provisioniong Service SDK
This repository contains [provisioning service client SDK](./provisioning_service_client/) for the Device Provisioning Service to [programmatically enroll devices](https://docs.microsoft.com/en-us/azure/iot-dps/how-to-manage-enrollments-sdks).

| Feature                                            | Support            | Description                                                                                                                                                                                                                                            |
|----------------------------------------------------|--------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| CRUD Operation with TPM Individual Enrollment      | :heavy_check_mark: | Programmatically manage device enrollment using TPM with the service SDK.  Please visit the [samples folder](./provisioning_service_client/samples/) to learn more about this feature. |
| Bulk CRUD Operation with TPM Individual Enrollment | :heavy_check_mark: | Programmatically bulk manage device enrollment using TPM with the service SDK.  Please visit the [samples folder](./provisioning_service_client/samples/) to learn more about this feature. |
| CRUD Operation with X.509 Individual Enrollment    | :heavy_check_mark: | Programmatically manage device enrollment using X.509 individual enrollment with the service SDK.  Please visit the [samples folder](./provisioning_service_client/samples/) to learn more about this feature. |
| CRUD Operation with X.509 Group Enrollment         | :heavy_check_mark: | Programmatically manage device enrollment using X.509 group enrollment with the service SDK.  Please visit the [samples folder](./provisioning_service_client/samples/) to learn more about this feature. |
| Query enrollments                                  | :heavy_check_mark: | Programmatically query registration states with the service SDK.  Please visit the [samples folder](./provisioning_service_client/samples/) to learn more about this feature.                                                                            |

## OS Platforms and Hardware Compatibility

The IoT Hub device SDK for C can be used with a broad range of OS platforms and devices.

The minimum requirements are for the device platform to support the following:

- **Being capable of establishing an IP connection**: only IP-capable devices can communicate directly with Azure IoT Hub.
- **Support TLS**: required to establish a secure communication channel with Azure IoT Hub.
- **Support SHA-256** (optional): necessary to generate the secure token for authenticating the device with the service. Different authentication methods are available and not all require SHA-256.
- **Have a Real Time Clock or implement code to connect to an NTP server**: necessary for both establishing the TLS connection and generating the secure token for authentication.
- **Having at least 64KB of RAM**: the memory footprint of the SDK depends on the SDK and protocol used as well as the platform targeted. The smallest footprint is achieved targeting microcontrollers.
- **Having at least 4KB of RAM set for incoming SSL max content length buffer**: For some TLS libraries, this may be a configurable option and default may have been set as **4KB** for low memory footprint devices. During TLS handshake, IoT Hub service will send Server Hello which includes IoT Hub server side certificates as part of Server Hello payload. 
During **renewal** of these IoT Hub server side certificates, check will be made on IoT Hub service side to prevent **Server Hello** exceeding 4KB limit so that existing devices which are set for 4KB limit continue to work as before after certificate renewals.

Platform support details can be found in [this document](https://docs.microsoft.com/azure/iot-hub/iot-hub-device-sdk-platform-support).
You can find an exhaustive list of the OS platforms the various SDKs have been tested against in the [Azure Certified for IoT device catalog](https://catalog.azureiotsuite.com/). Note that you might still be able to use the SDKs on OS and hardware platforms that are not listed on this page: all the SDKs are open sourced and designed to be portable. If you have suggestions, feedback or issues to report, refer to the Contribution and Support sections below.

## Porting the Azure IoT Device Client SDK for C to New Devices

The C SDKs and Libraries:
* Are written in ANSI C (C99) and avoids compiler extensions to maximize code portability and broad platform compatibility.
* Expose a platform abstraction layer to isolate OS dependencies (threading and mutual exclusion mechanisms, communications protocol e.g. HTTP). Refer to our [porting guide][c-porting-guide] for more information about our abstraction layer.

In the repository you will find instructions and build tools to compile and run the device client SDK for C on Linux, Windows and microcontroller platforms (refer to the links above for more information on compiling the device client for C).

If you are considering porting the device client SDK for C to a new platform, check out the [porting guide](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md) document.

## Contribution, Feedback and Issues

If you encounter any bugs, have suggestions for new features or if you would like to become an active contributor to this project please follow the instructions provided in the [contribution guidelines](.github/CONTRIBUTING.md).

## Support
* Have a feature request for SDKs? Please post it on [User Voice](https://feedback.azure.com/forums/321918-azure-iot) to help us prioritize.
* Have a technical question? Ask on [Stack Overflow](https://stackoverflow.com/questions/tagged/azure-iot-hub) with tag "azure-iot-hub".
* Need Support? Every customer with an active Azure subscription has access to [support](https://docs.microsoft.com/en-us/azure/azure-supportability/how-to-create-azure-support-request) with guaranteed response time.  Consider submitting a ticket and get assistance from Microsoft support team
* Found a bug? Please help us fix it by thoroughly documenting it and filing an issue on GitHub ([C](https://github.com/Azure/azure-iot-sdk-c), [Java](https://github.com/Azure/azure-iot-sdk-java), [.NET](https://github.com/Azure/azure-iot-sdk-csharp), [Node.js](https://github.com/Azure/azure-iot-sdk-node), [Python](https://github.com/Azure/azure-iot-sdk-python)).

## Read more

* [Azure IoT Hub documentation][iot-hub-documentation]
* [Prepare your development environment to use the Azure IoT device SDK for C][devbox-setup]
* [Setup IoT Hub][setup-iothub]
* [Azure IoT device SDK for C tutorial][c-sdk-intro]
* [How to port the C libraries to other OS platforms](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md)
* [Cross compilation example][c-cross-compile]
* [C SDKs API reference][c-api-reference]

## SDK Folder Structure

### /c-utility, /uamqp, /umqtt, /parson

These are git submodules that contain code, such as adapters and protocol implementations, shared with other projects. Note that some of them contain nested submodules.

### /blob

This folder contains client components that enable access to Azure blob storage.

### /dps_client

This folder contains client library for device provisioning service.

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

# Long Term Support

The project offers a Long Term Support (LTS) version to allow users that do not need the latest features to be shielded from unwanted changes.

A new LTS version will be created every 6 months. The lifetime of an LTS branch is currently planned for one year. LTS branches receive all bug fixes that fall in one of these categories:

- security bugfixes
- critical bugfixes

No new features will be picked up in an LTS branch.

LTS branches are named lts_*mm*_*yyyy*, where *mm* and *yyyy* are the month and year when the branch was created. An example of such a branch is *lts_07_2017*.

## Schedule<sup>1</sup>

Below is a table showing the mapping of the LTS branches to the packages released

| Package | Github Branch | LTS Status | LTS Start Date | Maintenance End Date | Removal Date |
| :-----------: | :-----------: | :--------: | :------------: | :------------------: | :----------: |
| Vcpkg: 2020-07-19<br/> Xenial: 0.2.0.0-27xenial<br/> Trusty: 0.2.0-27trusty<br/> Bionic: 0.2.0.0-20bionic<br/>    | lts_07_2020   | Active     | 2020-07-19     | 2021-07-19           | 2021-07-19   |
| Vcpkg: 2020-02-07.1<br/> Xenial: 0.2.0.0-26xenial<br/> Trusty: 0.2.0-26trusty<br/> Bionic: 0.2.0.0-19bionic<br/>    | lts_02_2020   | Active     | 2020-02-04     | 2021-02-04           | 2021-02-04   |


* <sup>1</sup> All scheduled dates are subject to change by the Azure IoT SDK team.

### Planned Release Schedule
![](./lts_branches.png)

---
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

Microsoft collects performance and usage information which may be used to provide and improve Microsoft products and services and enhance your experience.  To learn more, review the [privacy statement](https://go.microsoft.com/fwlink/?LinkId=521839&clcid=0x409).  

[iot-dev-center]: http://azure.com/iotdev
[iot-hub-documentation]: https://docs.microsoft.com/azure/iot-hub/
[devbox-setup]: doc/devbox_setup.md
[setup-iothub]: https://aka.ms/howtocreateazureiothub
[c-sdk-intro]: https://azure.microsoft.com/documentation/articles/iot-hub-device-sdk-c-intro/
[c-porting-guide]: https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md
[c-cross-compile]: doc/SDK_cross_compile_example.md
[c-api-reference]: https://docs.microsoft.com/azure/iot-hub/iot-c-sdk-ref/
[azure-iot-sdks]:https://github.com/azure/azure-iot-sdks
