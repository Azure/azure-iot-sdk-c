# Microsoft Azure IoT Digital Twin Client SDK for C

This folder contains the headers, source, samples, and internal testing for the Digital Twin Client for the C SDK.

These instructions and samples assume basic familiarity with Digital Twin concepts.  To learn more background information, see [here](https://aka.ms/iotpnpdocs).

## Getting started
### Building
Instructions on how to build the Digital Twin Client SDK or else retrieve packages for it are available [here](./doc/building_sdk.md).  If you are familiar with building the IoTHub Device SDK, the instructions are almost identical.

### Exploring samples

There are two separate but related samples directories.  **It may be easiest to just jump in here for a guided tour of Digital Twin.**

* If you are connecting to IoT Central and/or a device with limited resources, checkout [this directory](./samples/digitaltwin_sample_ll_device).  It uses the Azure IoT C SDK in a more resource friendly way (albeit slightly more difficult to program against).  It also contains provisioning logic needed to connect to IoT Central.
* The [convenience layer samples directory](./samples/digitaltwin_sample_device) contains Digital Twin samples demonstrating creation of interfaces and basic operation when going against IotHub.

Both samples are further documented [here](./samples/readme.md).

## Documentation

Conceptual documentation for the Digital Twin C SDK is available [here](./doc/readme.md).  

**There are important decisions you need to make, especially related to your threading models and optimizing for resource constrained devices, that are covered in depth in these documents.  Even after succesfully running through the sample content, you should still understand this prior to productizing a Digital Twin device.**

Reference guidance for the API's is available in the [public headers](./inc) themselves.

## If something is broken

If you hit an issue with the C SDK Digital Twin, please open a GitHub issue in https://github.com/Azure/azure-iot-sdk-c-pnp.