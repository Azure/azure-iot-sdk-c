# Microsoft Azure IoT serializer library for C

## Before using the serializer, **please make sure its right for your project.**

While the serializer can help interacting with JSON because of its declarative model, there are limitations you should be aware of:
* The serializer makes use of non-C99 pre-processor directives.  '\_\_COUNTER\_\_' in particular **is not supported on many embedded compilers**.  Making the serializer work on such a compiler requires the burdensome step of using a compiler that understands '\_\_COUNTER\_\_' (e.g. Visual Studio, gcc, or clang) and having it pre-process these headers as a 1st step.
* The serializer assumes your schema will never change.  If the schema changes and it sees elements it doesn't recognize, it stops parsing.
* The serializer can take only up to 61 fields as children of any given parent. (You can go as deep as you want, so it is not a 61 field max total, but only no 61 siblings).  This is because a preprocessor limitations of certain compilers and is not fixable.
* For embedded devices with extremely limited resources (where RAM and ROM and measured in KB), the serializer adds substantial overhead versus a [stripped down](../doc/run_c_sdk_on_constrained_device.md) C SDK.
* We will continue to fix critical serializer bugs, but we do not plan on adding additional features to the serializer.

An example of programmatically parsing JSON for the IoT C SDK *without the serializer* for both device twins and methods is available [here](../iothub_client/samples/iothub_client_device_twin_and_methods_sample).

## Serializer Instructions
This folder contains the following 
* A helper library for serializing and deserializing data

### Features
* Format the data you want to send to the Cloud and deserialize data received from the Cloud
* Simply declare a "model" for the device using Macros
* Easily implement Azure IoT Device Twins and Methods features

### Using the seralizer library for C

The seralizer library for C is used with the Azure IoT device SDK for which you will find detailed instructions on how to use on Linux, mbed, Windows and other platforms [here][device-sdk].

[devbox_setup]: ../doc/devbox_setup.md
[device-sdk]: ../iothub_client/