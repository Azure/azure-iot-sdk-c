// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file prov_device_client.h
*    @brief Extends the Prov_Device_LL module with additional features.
*
*    @details Prov_Device is a module that extends the Prov_Device_LL
*             module with 2 features:
*                - scheduling the work for the IoTHubCLient from a
*                  thread, so that the user does not need to create their
*                  own thread
*                - thread-safe APIs
*/

#ifndef PROV_DEVICE_CLIENT_H
#define PROV_DEVICE_CLIENT_H

#ifndef PROV_DEVICE_CLIENT_INSTANCE_TYPE
typedef struct PROV_DEVICE_INSTANCE_TAG* PROV_DEVICE_HANDLE;
#define PROV_DEVICE_CLIENT_INSTANCE_TYPE
#endif // PROV_DEVICE_CLIENT_INSTANCE_TYPE

#include <stddef.h>
#include <stdint.h>
#include "prov_device_ll_client.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_prov_client/prov_transport.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, PROV_DEVICE_HANDLE, Prov_Device_Create, const char*, uri, const char*, scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, protocol);
MOCKABLE_FUNCTION(, void, Prov_Device_Destroy, PROV_DEVICE_HANDLE, prov_device_handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_Register_Device, PROV_DEVICE_HANDLE, prov_device_handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, register_status_callback, void*, status_user_context);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_SetOption, PROV_DEVICE_HANDLE, prov_device_handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, const char*, Prov_Device_GetVersionString);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_DEVICE_CLIENT_H
