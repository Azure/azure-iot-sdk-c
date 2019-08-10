# Digital Twin C SDK High-Level Documentation

## Overview

This folder contains high level concepts of Digital Twins and Azure IoT PnP devices related to the C SDK.

* The core [Digital Twin and Azure IoT PnP documentation](https://aka.ms/iotpnpdocs).  If you are unfamiliar with Digital Twins / Azure IoT PnP, you should read through this documentation first.
  * The documents in this folder, reference material, and samples in this repo assume basic knowledge of Digital Twins / Azure IoT PnP.  
* The [C SDK Digital Twin sample](../samples), which provide concrete examples of Digital Twin implementation.  **Reading the code here maybe the best place to start.**
* The [public header files](../inc) have API reference material on the API's themselves.
* Documentation for the [Azure IoT C Device SDK](../../doc) and building notes [here](../../iothub_client/readme.md).  
  * The Digital Twin SDK tries to be self-contained as possible.  Other that the items explicitly called out below, **you should not have to learn too much about the IoTHub Device SDK to get started with the Digital Twin Device SDK.**
  * The Digital Twin device client uses the IoT C Device SDK for its transport layer.
  * The infrastructure for building the Digital Twin SDK (or getting it from a package) leverages IoTHub Device SDK work, as well.

## Support

If you need help on the Digital Twin SDK, please use the same support options described [here](../../readme.md#support).  

## Folder layout

In this folder, and in rough order of how you should read the documentation:

* [Build instructions](./building_sdk.md) shows how to build the Digital Twin SDK and begin making applications with it.
* [Connection setup](./connection_setup.md) describes how to setup an initial IoTHub connection from your Digital Twin device.
* [Threading notes](./threading_notes.md) describes the threading models available to Digital Twin device developers and their pros, cons, and caveats.
* [Interface handles](./interfaces.md) describes how to create and interact with Digital Twin interface handles.
* [Properly formatting your data](./data_format.md) While Digital Twin is based on JSON, there are nuances and common mistakes application developers can make while using it.  **Some of these are very hard to diagnose and will impact service developers**.
* [Where is the C service SDK?](./where_is_c_service_sdk.md) covers the Digital Twin Service SDK (or lack thereof).
