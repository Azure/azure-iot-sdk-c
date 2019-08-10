// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


/**  
  @file digitaltwin_device_client_ll.h

  @brief  Digital Twin DeviceClient_LL handle and functions.

  @remarks The Digital Twin DeviceClient LL is used to connect Digital Twin interfaces to an IoTHub device corresponding to them.

  @warning API's in this header are *NOT* thread safe.  See <c>DIGITALTWIN_DEVICE_CLIENT_LL</c> for more information.  It is critically
           important you understand the differences between the this lower-layer (LL) and convenience API's (<c>digitaltwin_device_client.h</c>) as this
           your decision will have large implications on the threading model and resource consumption of your application.
*/


#ifndef DIGITALTWIN_DEVICE_CLIENT_LL_H
#define DIGITALTWIN_DEVICE_CLIENT_LL_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_device_client_ll.h"
#include "digitaltwin_client_common.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
  @brief    Lower-layer (LL) handle to bind Digital Twin interface handles to IoTHub transport.

  @remarks  This handle is *NOT* thread safe.  The underlying Digital Twin SDK, in order to save resources on constrained devices, does 
            not implement locking or a background thread to automatically process data.

  @remarks  The "LL" stands for lower-layer, to differentiate it from the convenience layer (DigitalTwin_Device* APi's without the "_LL_") which is built on top of the LL layer.

  @warning  ALL calls to this handle either need to be on the same thread or else the application must add locking logic at its layer to
            manually perform locking.  Using the same <c>DIGITALTWIN_DEVICE_CLIENT_LL</c> on different threads and without application
            specific locking will result in race conditions, as the <c>DIGITALTWIN_DEVICE_CLIENT_LL</c> itself does not use locking.
*/
typedef struct DIGITALTWIN_DEVICE_CLIENT_LL* DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE;

/**
  @brief    Creates a new <c>DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE</c> based on a pre-existing <c>IOTHUB_DEVICE_CLIENT_LL_HANDLE</c>.
 
  @remarks  <c>DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle</c> behaves nearly identically to <c>DigitalTwin_DeviceClient_CreateFromDeviceHandle</c>, with the exception
            that a different handle type is used.

  @see      DigitalTwin_DeviceClient_CreateFromDeviceHandle
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE, deviceHandle, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE*, dtDeviceLLClientHandle);


/**
  @brief    Registers the specified <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>'s with the Digital Twin Service.

  @remarks  <c>DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync</c> behaves nearly identically to <c>DigitalTwin_DeviceClient_RegisterInterfacesAsync</c>, with the exception
            that a different handle type is used.

  @see DigitalTwin_DeviceClient_RegisterInterfacesAsync
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE, dtDeviceClientHandle, const char*, deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE*, dtInterfaces, unsigned int, numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, dtInterfaceRegisteredCallback, void*, userContextCallback);

/**
  @brief    Invokes worker actions on <c>DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE</c>

  @remarks  When using a DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE, the Digital Twin SDK does *NOT* perform any operations in the background.  Applications instead
            must schedule peroidic work via the <c>DigitalTwin_DeviceClient_LL_DoWork</c> API.

  @remarks  Examples of operations <c>DigitalTwin_DeviceClient_LL_DoWork</c> performs - all on the call to <c>DigitalTwin_DeviceClient_LL_DoWork</c> - include:
            - Examining pending send operations and invoking the appropriate network I/O
            - Retrieving network I/O
            - Invoking user callbacks when send events are complete or else incoming commands / properties arrive from the server.

  @remarks  Device applications should be aware that <c>DigitalTwin_DeviceClient_LL_DoWork</c> may take considerable time to perform these operations, especially if there are
            any interface callbacks that take time to execute.  These applications should be designed accordiningly.

  @remarks  <c>DigitalTwin_DeviceClient_LL_DoWork</c> does not have an analog in the DigitalTwin_DeviceClient_* header, by design.  For the DigitalTwin_DeviceClient_* API's (aka
            the convenience layer), this DoWork is automatically invoked by the SDK.  This maybe more convenient to program to not need to call this function and also to provide locking. 
            using the convenience layer, instead of this _LL_ layer, comes at the cost of an extra thread being spun and additional RAM and ROM required to manage additional 
            dispatching complexity.

  @attention <c>DigitalTwin_DeviceClient_LL_DoWork</c> needs to be invoked perodically for the length of a connection, even if the device application has no data to send.
             Otherwise the underlying network stack and/or service may time out the connection due to inactivity.

  @param[in] dtDeviceClientHandle Handle to perform work on.

  @returns  N/A.

*/
MOCKABLE_FUNCTION(, void, DigitalTwin_DeviceClient_LL_DoWork, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE, dtDeviceClientHandle);

/**
  @brief    Destroys resources associated with a <c>DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE</c>.

  @remarks  <c>DigitalTwin_DeviceClient_LL_Destroy</c> behaves nearly identically to <c>DigitalTwin_DeviceClient_Destroy</c>, with the exception 
            that a different handle type is used.

  @see      DigitalTwin_DeviceClient_Destroy
*/
MOCKABLE_FUNCTION(, void, DigitalTwin_DeviceClient_LL_Destroy, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE, dtDeviceClientHandle);


#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_DEVICE_CLIENT_LL_H
