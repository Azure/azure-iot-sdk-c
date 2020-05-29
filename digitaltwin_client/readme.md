# Microsoft Azure IoT Digital Twin Client SDK for C

This folder contains the headers, source, samples, and internal testing for the Digital Twin Client for the C SDK.

These instructions and samples assume basic familiarity with Digital Twin and Azure IoT Plug and Play concepts.  To learn more, please go [here](https://aka.ms/iotpnpdocs).

## Getting started
### Building
Start by building the Digital Twin Client SDK; instructions are available [here](./doc/building_sdk.md).  Be sure to clone the repo near your system root to maintain a valid path length.

**Please use [public-preview](https://github.com/Azure/azure-iot-sdk-c/tree/public-preview) branch unless you have been directed to be on public-preview-pnp.  public-preview-pnp branch is our advance working branch which might contain breaking changes.**

### Exploring samples
There are two separate, but related, sample directories.

* For connecting to Iot Hub, you will want to use the [digitaltwin\_sample\_device directory](./samples/digitaltwin_sample_device). 
* If you are connecting to IoT Central or using a device with limited resources, instead use the [digitaltwin\_sample\_ll_device directory](./samples/digitaltwin_sample_ll_device).

To get stared with either of these sample directories, follow the remaining instructions [here](./samples/readme.md).

## Documentation

Conceptual documentation for the Digital Twin C SDK is available [here](./doc/readme.md).  

**There are important decisions you need to make, especially related to your threading models and optimizing for resource constrained devices, that are covered in depth in these documents.  Even after succesfully running through the sample content, you should still understand this prior to productizing a Digital Twin device.**

Reference guidance for the API's is available in the [public headers](./inc) themselves.

## Visual Studio Code Integration

Visual Studio Code has a plugin to make authoring Digital Twin clients using the C SDK easier.  It includes the ability to automatically generate C device code based on interface definitions.  See [here](https://docs.microsoft.com/en-us/azure/iot-pnp/howto-develop-with-vs-vscode) for more information.

## If something is broken

If you hit an issue with the C SDK Digital Twin, please open a GitHub issue in https://github.com/Azure/azure-iot-sdk-c.
