# Digital Twin Connection Setup to Service and Interface Registration

## Overview

This document covers initial creation of a Digital Twin device using the C SDK.  It covers:

* How to setup the initial connection between a Digital Twin and Azure, intermediated by the IoTHub Device client.
* Protocol guidelines.
* Interface registration details.

## IoTHub Device Client and Digital Twin interactions

### Background on IoTHub Device Client

The [IoTHub Device Client](../../iothub_client) is a pre-existing library used to connect devices to IoTHub and send and receive data from service applications.  The Digital Twin SDK in [this folder](../../iothub_client) uses the **IoTHub Device Client** as its transport layer, while providing a better abstracted Digital Twin layer to applications.  A key benefit of using Digital Twin SDK instead of the IoTHub Device Client directly is the Digital Twin SDK encourages modeling of your data, which will make it easier for service applications to use.

### Handle types

Like all API's in the the C IoT SDK repo, the Digital Twins API's use handles.  There are currently two types of Digital Twin device handles, each of which correspond to an IoTHub Device Client handle.  Which type of handle you use depends on your threading model and/or resource limitations of your device.

| Digital Twin Handle        | Corresponding IoTHub Handle           | Notes  |
| :------------- |:-------------| :-----|
| [`DIGITALTWIN_DEVICE_CLIENT_HANDLE`](../inc/digitaltwin_device_client.h)      | IOTHUB_DEVICE_CLIENT_HANDLE | Used to represent an IoTHub device, typically on a device with higher resources.  Create with `DigitalTwin_DeviceClient_CreateFromDeviceHandle`. |
| [`DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE`](../inc/digitaltwin_device_client_ll.h)      | IOTHUB_DEVICE_CLIENT_LL_HANDLE  | Used to represent an IoTHub device, typically on a more constrained device.  A comparison of the LL and non-LL handles is [here](./threading_notes.md).  Create with `DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle`.|

### IoTHub handles creation and hand-off to Digital Twin

The first step in using the Digital Twin SDK is to create the appropriate `IOTHUB_*_HANDLE`, based on the table above.  There are many ways to create these handles.  As just a few examples

* `IoTHubDeviceClient_CreateFromConnectionString` - create an IOTHUB_DEVICE_CLIENT_HANDLE from its connection string.
* `IoTHubDeviceClient_LL_CreateFromDeviceAuth` - create an IOTHUB_DEVICE_CLIENT_LL_HANDLE based on DPS.

**NOTE: The only protocols supported for Digital Twin for the C SDK are MQTT and MQTT over Web Sockets.**  Attempts to use AMQP, AMQP over Web Sockets, or HTTP for C will fail.

Once the handle is created, you may need to [set options on it](../../doc/Iothub_sdk_options.md).  Some of the options you may need to set include enabling web proxies, enabling verbose logging at the transport layer, configuring client certificates, and more.  All options that need to be set should be *before* the next step.

Once the `IOTHUB_*_HANDLE` has been successfully created, it needs to be passed to create a `DIGITALTWIN_*_HANDLE`.  Use the appropriate `DigitalTwin*_CreateFrom*Handle` referenced in the table above.

### The `DIGITALTWIN_*_HANDLE` is the owner of `IOTHUB_*_HANDLE`

Once the `DIGITALTWIN_*_HANDLE` has been created, **NEVER** access the `IOTHUB_*_HANDLE`.  The `DIGITALTWIN_*_HANDLE` owns the `IOTHUB_*_HANDLE` and is responsible for its destruction.  Using the `IOTHUB_*_HANDLE` after this point can impact your application's stability, including causing crashes, and/or put non-Digital Twin schematized data into your Hub.

### Device Provisioning Service (DPS) interactions

Digital Twin supports connections via the [Device Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/about-iot-dps) (DPS).  DPS is still used to create an `IOTHUB_DEVICE_*_HANDLE` and this handle is provided to the appropriate Digital Twin handle creation API.

An example of DPS and Digital Twin interacting is available [here](../samples/digitaltwin_sample_ll_device).

<a name="Register_Interfaces"></a>

## Registering Interfaces

Once the `DIGITALTWIN_*_HANDLE` is created, the next step is to register interfaces.  Interface registration is the process by which interfaces that the device application has created are broadcast to the Digital Twin service.  This service in turn makes them available to service operators and applications.  For instance, your device application may register interfaces for "urn:fabrikam:com:lightbulb:3" and "urn:fabrikam:com:firmwareupdater:1" to alert the service it supports a light bulb and firmware update interfaces.

The device application registers interfaces via [`DigitalTwin_DeviceClient_RegisterInterfacesAsync`](../inc/digitaltwin_device_client.h) (or the \_LL\_ equivalent).  The device application can only begin Digital Twin operations - namely sending telemetry, reporting property updates, or receiving property updates and commands - after the interface registration process completes.

For a given `DIGITALTWIN_*_HANDLE`, its corresponding `RegisterInterfacesAsync` function may not be called more than one time.  Attempts to call `RegisterInterfacesAsync` multiple times for a given handle will fail.

Once a `DIGITALTWIN_INTERFACE_CLIENT_HANDLE` is passed to `RegisterInterfacesAsync`, it may **NOT** be used in calls to `RegisterInterfacesAsync` even of a different `DIGITALTWIN_*_HANDLE`.

## Shutting down handles

The resources associated with a `DIGITALTWIN_*_HANDLE` are freed via [`DigitalTwin_DeviceClient_Destroy`](../inc/digitaltwin_device_client.h) (or the \_LL\_ equivalent).  

**DO NOT DESTROY THE INITIAL `IOTHUB_*_HANDLE` USED TO SETUP `DIGITALTWIN_*_HANDLE`.**  The Digital Twin handle takes ownership of the IoTHub handle associated with it and is responsible for destroying the underlying IoTHub handle.  Attempting to close the IoTHub handle either before or after Digital Twin's destroy is invoked will result in a crash.

After the Digital Twin handle has been closed, any interface handles that have been registered with it but have not themselves been closed will fail if the application attempts to use them.
