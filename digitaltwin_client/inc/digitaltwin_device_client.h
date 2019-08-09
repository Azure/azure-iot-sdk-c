// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**  
  @file digitaltwin_device_client.h

  @brief  Digital Twin DeviceClient handle and functions.

  @remarks The Digital Twin DeviceClient is used to connect Digital Twin interfaces to an IoTHub device corresponding to them.

  @remarks Functions in this header act as the convenience layer for Digital Twin.  This means that calls to DIGITALTWIN_DEVICE_CLIENT_HANDLE are thread safe, albeit at the
           cost of requiring additional resources to manage this as compared to (digitaltwin_device_client_ll.h).

  @see digitaltwin_device_client_ll.h for a more detailed description of the differences between the lower-layer (LL) functions of that file and the convenience layer here.
*/

#ifndef DIGITALTWIN_DEVICE_CLIENT_H 
#define DIGITALTWIN_DEVICE_CLIENT_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_device_client.h"
#include "digitaltwin_client_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
* @brief    Convenience layer handle to bind Digital Twin interface handles to the IoTHub transport.
*/
typedef void* DIGITALTWIN_DEVICE_CLIENT_HANDLE;

/**
  @brief    Creates a new <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c> based on a pre-existing <c>IOTHUB_DEVICE_CLIENT_HANDLE</c>.
 
  @remarks  DigitalTwin_DeviceClient_CreateFromDeviceHandle is used when initially bringing up Digital Twin.  Use <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c> when
            the Digital Twin maps to an an IoTHub device (as opposed to an IoTHub module).  DIGITALTWIN_DEVICE_CLIENT_HANDLE also guarantee thread safety at
            the Digital Twin layer and do NOT require the application to explicitly invoke DoWork() to schedule actions.  <c>DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle</c> 
            is to be used when thread safety is not required (or possible on very small devices) and/or you want explicitly control Digital Twin by DoWork() operations.

  @remarks  Prior to invoking <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c>, applications *MUST* specify all options on the <c>IOTHUB_DEVICE_CLIENT_HANDLE</c> that are
            required.  For instance, trace levels, web proxy configuration, etc.
 
  @warning  Callers MUST NOT directly access <c>deviceHandle</c> after it is successfully passed to <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c>.  The returned 
            <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c> owns this handle, including its destruction.
 
  @param    deviceHandle[in]            An IOTHUB_DEVICE_CLIENT_HANDLE that has been already created and bound to a specific connection string (or transport, or DPS handle, or whatever
                                        mechanism is preferred).  See remarks about its lifetime management.
  @param    dtDeviceClientHandle[out]   A <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c> to be used by the application.
 
  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_DeviceClient_CreateFromDeviceHandle, IOTHUB_DEVICE_CLIENT_HANDLE, deviceHandle, DIGITALTWIN_DEVICE_CLIENT_HANDLE*, dtDeviceClientHandle);


/**
  @brief    Registers the specified DIGITALTWIN_INTERFACE_CLIENT_HANDLE's with the DigitalTwin Service.
 
  @remarks  <c>DigitalTwin_DeviceClient_RegisterInterfacesAsync</c> registers specified dtInterfaces with the Digital Twin Service.  This registration occurrs
            asychronously.  While registration is in progress, the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>'s that are being registered are NOT valid for sending telemetry on
            nor will they be able to receive commands.

  @remarks  <c>DigitalTwin_DeviceClient_RegisterInterfacesAsync</c> may not be called multiple times for the same <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c>.  If a given Digital Twin device
            needs to have its handles re-registered, it needs to <c>DigitalTwin_DeviceClient_Destroy</c> the existing <c>DIGITALTWIN_DEVICE_CLIENT</c> and create a new one.

  @param    dtDeviceClientHandle[in]            A <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c> created by <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c>.
  @param    dtInterfaces[in]                    An array of length numDTInterfaces of <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>'s to register with the service.
  @param    numDTInterfaces[in]                 The number of items in the dtInterfaces array.
  @param    dtInterfaceRegisteredCallback[in]   User specified callback that will be invoked on registration completion or failure.  Callers should not begin sending Digital Twin telemetry until this callback is invoked.
  @param    userContextCallback[in]             User context that is provided to the callback.
 
  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_DeviceClient_RegisterInterfacesAsync, DIGITALTWIN_DEVICE_CLIENT_HANDLE, dtDeviceClientHandle, const char*, deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE*, dtInterfaces, unsigned int, numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, dtInterfaceRegisteredCallback, void*, userContextCallback);

/**
  @brief    Destroys resources associated with a <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c>.
 
  @remarks  <c>DigitalTwin_DeviceClient_Destroy</c> will destroy resources created from the <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c> call, including the <c>IOTHUB_DEVICE_CLIENT_HANDLE</c> whose
            ownership was transferred during the call to <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c>.

  @remarks  If the specified <c>DIGITALTWIN_DEVICE_CLIENT_HANDLE</c> was in the middle of processing a service request - e.g. alerting one of its interfaces that
            a property had been updated or that a command arrived - the <c>DigitalTwin_DeviceClient_Destroy</c> call will block until the interface's callback has completed
            execution.

  @remarks  After <c>DigitalTwin_DeviceClient_Destroy</c> returns, there will be no further callbacks on any threads associated with any Digital Twin interfaces.

  @remarks  Using <c>dtDeviceClientHandle</c> after the this call may result in an application crash.

  @returns  N/A.
*
*/
MOCKABLE_FUNCTION(, void, DigitalTwin_DeviceClient_Destroy, DIGITALTWIN_DEVICE_CLIENT_HANDLE, dtDeviceClientHandle);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_DEVICE_CLIENT_H
