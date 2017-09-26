// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_CLIENT_H
#define DPS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_hub_modules/iothub_device_auth.h"
#include "azure_hub_modules/dps_transport.h"

typedef struct DPS_INSTANCE_INFO_TAG* DPS_LL_HANDLE;

#define DPS_RESULT_VALUES    \
    DPS_CLIENT_OK,                  \
    DPS_CLIENT_INVALID_ARG,         \
    DPS_CLIENT_ERROR

DEFINE_ENUM(DPS_RESULT, DPS_RESULT_VALUES);

#define DPS_ERROR_VALUES            \
    DPS_ERROR_SUCCESS,              \
    DPS_ERROR_INVALID_ARG,          \
    DPS_ERROR_MEMORY,               \
    DPS_ERROR_PARSING,              \
    DPS_ERROR_TRANSPORT,            \
    DPS_ERROR_INVALID_STATE,        \
    DPS_ERROR_DEV_AUTH_ERROR,       \
    DPS_ERROR_TIMEOUT,              \
    DPS_ERROR_KEY_ERROR

DEFINE_ENUM(DPS_ERROR, DPS_ERROR_VALUES);

#define DPS_REGISTRATION_STATUS_VALUES      \
    DPS_REGISTRATION_STATUS_CONNECTED,      \
    DPS_REGISTRATION_STATUS_REGISTERING,    \
    DPS_REGISTRATION_STATUS_ASSIGNING,      \
    DPS_REGISTRATION_STATUS_ASSIGNED

DEFINE_ENUM(DPS_REGISTRATION_STATUS, DPS_REGISTRATION_STATUS_VALUES);

typedef void(*DPS_CLIENT_REGISTER_DEVICE_CALLBACK)(DPS_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context);
typedef void(*DPS_CLIENT_ON_ERROR_CALLBACK)(DPS_ERROR error_type, void* user_context);
typedef void(*DPS_CLIENT_REGISTER_STATUS_CALLBACK)(DPS_REGISTRATION_STATUS reg_status, void* user_context);

typedef const DPS_TRANSPORT_PROVIDER*(*DPS_TRANSPORT_PROVIDER_FUNCTION)(void);

MOCKABLE_FUNCTION(, DPS_LL_HANDLE, DPS_LL_Create, const char*, dps_uri, const char*, scope_id, DPS_TRANSPORT_PROVIDER_FUNCTION, dps_protocol, DPS_CLIENT_ON_ERROR_CALLBACK, on_error_callback, void*, user_context);
MOCKABLE_FUNCTION(, void, DPS_LL_Destroy, DPS_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, DPS_RESULT, DPS_LL_Register_Device, DPS_LL_HANDLE, handle, DPS_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, DPS_CLIENT_REGISTER_STATUS_CALLBACK, reg_status_cb, void*, status_user_ctext);
MOCKABLE_FUNCTION(, void, DPS_LL_DoWork, DPS_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, DPS_RESULT, DPS_LL_SetOption, DPS_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, const char*, DPS_LL_Get_Version_String);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_CLIENT_H
