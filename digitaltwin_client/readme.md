# Microsoft Azure IoT Digital Twin Client SDK for C

This folder contains the headers, source, samples, and internal testing for the Digital Twin Client for the C SDK.

These instructions and samples assume basic familiarity with Digital Twin and Azure IoT Plug and Play concepts.  To learn more, please go [here](https://aka.ms/iotpnpdocs).

## Getting started
### Building
Start by building the Digital Twin Client SDK -- instructions are available [here](./doc/building_sdk.md).  Please use these instructions as they include CMake flags specific to the samples.  On Windows, be sure to clone the repo near your root to maintain a valid path length.

**Please use the [public-preview](https://github.com/Azure/azure-iot-sdk-c/tree/public-preview) branch unless you have been directed to be on `public-preview-pnp`.  The `public-preview-pnp` branch is our advance working branch which might contain breaking changes.**

### Exploring samples
There are two separate, but related, sample directories.

* For connecting to Azure IoT Hub with a non-resource-constrained device, you will want to use the [digitaltwin\_sample\_device directory](./samples/digitaltwin_sample_device). 
* If you are connecting to Azure IoT Central or using a device with limited resources, instead use the [digitaltwin\_sample\_ll_device directory](./samples/digitaltwin_sample_ll_device).

To get started with either of these sample directories, follow the remaining instructions [here](./samples/readme.md).

## Documentation

Conceptual documentation for the Digital Twin C SDK is available [here](./doc/readme.md).  

Reference guidance for the API's is available in the [public headers](./inc) themselves.

**There are important decisions you need to make that are covered in depth in these documents.  This includes threading models and optimizing for resource constrained devices. Even after succesfully running through the sample content, you should still understand this material prior to productizing a Digital Twin device.**


## If something is broken

If you hit an issue with the C SDK Digital Twin, please open a GitHub issue in https://github.com/Azure/azure-iot-sdk-c.
