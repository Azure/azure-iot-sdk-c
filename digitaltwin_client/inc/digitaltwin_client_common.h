// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**  
  @file digitaltwin_client_common.h

  @brief  Common handle, callbacks, and error codes used throughout Digital Twin SDK.
*/


#ifndef DIGITALTWIN_CLIENT_COMMON 
#define DIGITALTWIN_CLIENT_COMMON

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
  @brief    Handle representing a Digital Twin interface.
 
  @remarks  The <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> is used to represent a Digital Twin interface.  Interfaces may be used 
            for receiving commands, reporting properties, receiving property updates from the server, and sending telemetry.
            The <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> is not available for sending or receiving data until after the interface has been registered 
            with the appropriate Digital Twin device/module client.
*/
typedef void* DIGITALTWIN_INTERFACE_CLIENT_HANDLE;

#define DIGITALTWIN_CLIENT_RESULT_VALUES                        \
        DIGITALTWIN_CLIENT_OK,                                  \
        DIGITALTWIN_CLIENT_ERROR_INVALID_ARG,                   \
        DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY,                 \
        DIGITALTWIN_CLIENT_ERROR_REGISTRATION_PENDING,          \
        DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED,  \
        DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED,      \
        DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING,       \
        DIGITALTWIN_CLIENT_ERROR_COMMAND_NOT_PRESENT,           \
        DIGITALTWIN_CLIENT_ERROR_SHUTTING_DOWN,                 \
        DIGITALTWIN_CLIENT_ERROR_TIMEOUT,                       \
        DIGITALTWIN_CLIENT_ERROR_HANDLE_DESTROYED,              \
        DIGITALTWIN_CLIENT_ERROR_ACCESS_DENIED,                 \
        DIGITALTWIN_CLIENT_ERROR_CALLBACK_ALREADY_REGISTERED,   \
        DIGITALTWIN_CLIENT_ERROR_DUPLICATE_COMPONENTS, \
        DIGITALTWIN_CLIENT_ERROR                                \

MU_DEFINE_ENUM(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
/// @endcond


/**
  @brief    Function signature for the callback that is invoked by the Digital Twin SDK when interfaces are registered, fail, or are destroyed.
  
  @remarks  On calls to {@link DigitalTwin_InterfaceClient_Create} and the various RegisterInterfacesAsync API's, the device application may optionally
            pass a function pointer of <c>DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK</c> as a callback.
 
  @remarks  After the RegisterInterfaces call completes - either successfully or on failure - the Digital Twin Device SDK invokes any callback functions
            associated with it.  If multiple interfaces have each registered for a callback, their callback functions will be invoked one at a time.
 
  @remarks  If a device/module handle that a <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> has been associated with is destroyed *before* the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>
            itself, then the interface's callback function will be invoked with <c>dtInterfaceStatus</c> set to <c>DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING</c>.
 
  @remarks  Device applications may not perform any operations until after they are successfully registered, specified by a <c>DIGITALTWIN_CLIENT_OK</c> status.  Interface handles may not perform operations
            after they receive a <c>DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING</c> status.
 
  @remarks  Property update and commands initiated from the server may begin arriving on the interface as soon as the application returns from this callback.
 
  @param[in] dtInterfaceStatus Status of registering the interface.
  @param[in] userContextCallback Context pointer that was specified in the parameter <c>userContextCallback</c>, either during <c>DigitalTwin_InterfaceClient_Create</c> or <c>RegisterInterfacesAsync</c> call.

  @returns  N/A.
*/
typedef void(*DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK)(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userContextCallback);

#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_CLIENT_COMMON

