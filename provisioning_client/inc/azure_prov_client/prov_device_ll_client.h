// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_DEVICE_LL_CLIENT_H
#define PROV_DEVICE_LL_CLIENT_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_prov_client/prov_transport.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct PROV_INSTANCE_INFO_TAG* PROV_DEVICE_LL_HANDLE;

#define PROV_DEVICE_RESULT_VALUE       \
    PROV_DEVICE_RESULT_OK,              \
    PROV_DEVICE_RESULT_INVALID_ARG,     \
    PROV_DEVICE_RESULT_SUCCESS,         \
    PROV_DEVICE_RESULT_MEMORY,          \
    PROV_DEVICE_RESULT_PARSING,         \
    PROV_DEVICE_RESULT_TRANSPORT,       \
    PROV_DEVICE_RESULT_INVALID_STATE,   \
    PROV_DEVICE_RESULT_DEV_AUTH_ERROR,  \
    PROV_DEVICE_RESULT_TIMEOUT,         \
    PROV_DEVICE_RESULT_KEY_ERROR,       \
    PROV_DEVICE_RESULT_ERROR

DEFINE_ENUM(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

#define PROV_DEVICE_REG_STATUS_VALUES      \
    PROV_DEVICE_REG_STATUS_CONNECTED,      \
    PROV_DEVICE_REG_STATUS_REGISTERING,    \
    PROV_DEVICE_REG_STATUS_ASSIGNING,      \
    PROV_DEVICE_REG_STATUS_ASSIGNED,       \
    PROV_DEVICE_REG_STATUS_ERROR

DEFINE_ENUM(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

typedef void(*PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK)(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context);
typedef void(*PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK)(PROV_DEVICE_REG_STATUS reg_status, void* user_context);

typedef const PROV_DEVICE_TRANSPORT_PROVIDER*(*PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION)(void);

MOCKABLE_FUNCTION(, PROV_DEVICE_LL_HANDLE, Prov_Device_LL_Create, const char*, uri, const char*, scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, protocol);
MOCKABLE_FUNCTION(, void, Prov_Device_LL_Destroy, PROV_DEVICE_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_Register_Device, PROV_DEVICE_LL_HANDLE, handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, reg_status_cb, void*, status_user_ctext);
MOCKABLE_FUNCTION(, void, Prov_Device_LL_DoWork, PROV_DEVICE_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_SetOption, PROV_DEVICE_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, const char*, Prov_Device_LL_GetVersionString);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_DEVICE_LL_CLIENT_H
