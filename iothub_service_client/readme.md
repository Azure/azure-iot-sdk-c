# Microsoft Azure IoT service SDK for C

## End of active development for service SDK

The Azure IoT Service SDK for C is no longer under active development.

We will continue to fix critical bugs such as crashes, data corruption, and security vulnerabilities.  We will **NOT** add any new feature or fix bugs that are not critical, however.

Azure IoT Service SDK support is available in higher-level languages ([C#](https://github.com/Azure/azure-iot-sdk-csharp), [Java](https://github.com/Azure/azure-iot-sdk-java), [Node](https://github.com/Azure/azure-iot-sdk-node), [Python](https://github.com/Azure/azure-iot-sdk-python)).

The rest of this repo - for instance the [device sdk](../iothub_client) for deploying code to devices - continues to be under active development.  This ending of new features only applies to code under this subfolder.

## Folder Layout

This folder contains the following 

* The Azure IoT service SDK for C to easily and securely manage an instance of the Microsoft Azure IoT Hub service as well as send Cloud to Device messages through IOT Hub.
* Samples showing how to use the SDK

## Features

* Implements CRUD operations on Azure IoT Hub device registry
* Interact with a Device Twins from a back-end application
* Invoke a Cloud to Device direct Method 
* Implements sending a Cloud to Device message

## Using the service SDK for C

For using the service SDK for C, you will need to compile the library and the samples.
Instructions on how to setup your development environemnt can be found [here][devbox_setup].

## Samples

The repository contains a set of simple samples that will help you get started.
You can find a list of these samples with instructions on how to run them [here][samples]. 

[devbox_setup]: ../doc/devbox_setup.md
[samples]: ./samples/
