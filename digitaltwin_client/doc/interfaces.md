# Digital Twin Interface Implementation

## Overview

A Digital Twin device is defined by one or more interfaces.  Like other resources across the C IoT SDK, device applications interact with an interface by a handle - in this case [`DIGITALTWIN_INTERFACE_CLIENT_HANDLE`](../inc/digitaltwin_interface_client.h).

This document will cover high-level concepts that are C SDK specific.  However it will not document [Digital Twin interfaces in general](https://aka.ms/iotpnpdocs) or provide an [API reference](../inc) as these are documented elsewhere.

## Initial interface client creation and registration

* A `DIGITALTWIN_INTERFACE_CLIENT_HANDLE` handle is created by the Digital Twin SDK function `DigitalTwin_InterfaceClient_Create`.  
  * The application specifies the `interfaceId` and `interfaceInstanceName` in this step.  These are used during registration and to route method and property requests coming from the server.
  * The application may optionally specify a callback function to be invoked when the interface has registered, registration has failed, or its owning device handle has closed it.
* The application optionally invokes `DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback` and/or `DigitalTwin_InterfaceClient_SetCommandsCallback`, depending on whether the model that the interface implements supports commands and/or properties.
  * These callback commands **MUST** be invoked prior to the interface handle being registered.  Attempts to set callbacks post-registration will fail.
* The device application registers this interface by passing the `DIGITALTWIN_INTERFACE_CLIENT_HANDLE` into one of the appropriate [Digital Twin device RegisterInterfacesAsync](./connection_setup.md#Register_Interfaces) commands.
* Interface registration is a network operation which will take time and may fail.  The device application must not attempt any operations - e.g. sending telemetry or reporting properties - until the interface has been registered.  Attempts to do so will fail.

## Operations after registration

* Once the interface registration has been ACK'd by the server, the Digital Twin Device SDK will automatically query for desired properties and invoke the interface's property update callback (if specified in `DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback`).
* The Digital Twin SDK will  listen for property changes after the initial property query.  It will route any future property updates to the `DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback` specified callback.
* The Digital Twin SDK will listed for incoming commands and invoke the callback specified in `DigitalTwin_InterfaceClient_SetCommandsCallback`, if one was set. 
* The device application may periodically invoke `DigitalTwin_InterfaceClient_SendTelemetryAsync` and `DigitalTwin_InterfaceClient_ReportPropertyAsync` to report information to the server.  It may also invoke `DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync` to report the status of an asynchronous command.
* When the interface is ready to be shut down, `DigitalTwin_InterfaceClient_Destroy` frees its resources.  

## No \_LL\_ interface handles

When setting up the [threading model of the Digital Twin SDK](./threading_notes.md), the device side application must decide whether it whether it is using the \_LL\_ or convenience layer.

There is **NO** equivalent for interface handles.  Interface handles - in theory - are abstracted from whether they are being used on an \_LL\_ handle or not.  See for instance the [samples](../samples) directory.  The [device information](../samples/digitaltwin_sample_device_info) and [environmental sensor](../samples/digitaltwin_sample_environmental_sensor) work with both the [convenience](../samples/digitaltwin_sample_device) and [LL](../samples/digitaltwin_sample_ll_device) samples.

Note that a given interface *might* need to know whether it is \_LL\_ or not.  If the interface needs to spin worker threads to perform operations, this may be problematic for the threadless \_LL\_.  

The Digital Twin Device SDK does not provide a mechanism for letting interfaces know which type of handle they are running on.  If this really matters, the device application needs to take care of it out-of-band.
