// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_DEVICE_LL_CLIENT_H
#define PROV_DEVICE_LL_CLIENT_H

#include "umock_c/umock_c_prod.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_prov_client/prov_transport.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct PROV_INSTANCE_INFO_TAG* PROV_DEVICE_LL_HANDLE;

typedef enum PROV_DEVICE_RESULT_VALUE_TAG
{
    PROV_DEVICE_RESULT_OK = 0,
    PROV_DEVICE_RESULT_INVALID_ARG = 1,
    PROV_DEVICE_RESULT_SUCCESS = 2,
    PROV_DEVICE_RESULT_MEMORY = 3,
    PROV_DEVICE_RESULT_PARSING = 4,
    PROV_DEVICE_RESULT_TRANSPORT = 5,
    PROV_DEVICE_RESULT_INVALID_STATE = 6,
    PROV_DEVICE_RESULT_DEV_AUTH_ERROR = 7,
    PROV_DEVICE_RESULT_TIMEOUT = 8,
    PROV_DEVICE_RESULT_KEY_ERROR = 9,
    PROV_DEVICE_RESULT_ERROR = 10,
    PROV_DEVICE_RESULT_HUB_NOT_SPECIFIED = 11,
    PROV_DEVICE_RESULT_UNAUTHORIZED = 12,
    PROV_DEVICE_RESULT_DISABLED = 13
} PROV_DEVICE_RESULT_VALUE;

typedef enum PROV_DEVICE_REG_STATUS_TAG
{
    PROV_DEVICE_REG_STATUS_CONNECTED = 0,
    PROV_DEVICE_REG_STATUS_REGISTERING = 1,
    PROV_DEVICE_REG_STATUS_ASSIGNING = 2,
    PROV_DEVICE_REG_STATUS_ASSIGNED = 3,
    PROV_DEVICE_REG_STATUS_ERROR = 4,
    PROV_DEVICE_REG_HUB_NOT_SPECIFIED = 5
} PROV_DEVICE_REG_STATUS;

static const char* const PROV_REGISTRATION_ID = "registration_id";
static const char* const PROV_OPTION_LOG_TRACE = "logtrace";
static const char* const PROV_OPTION_TIMEOUT = "provisioning_timeout";

typedef void(*PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK)(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context);
typedef void(*PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK)(PROV_DEVICE_REG_STATUS reg_status, void* user_context);

typedef const PROV_DEVICE_TRANSPORT_PROVIDER*(*PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION)(void);

/**
* @brief    Creates a Provisioning Client for communications with the Device Provisioning Client Service
*
* @param    uri         The URI of the Device Provisioning Service
* @param    scope_id    The customer specific Id Scope
* @param    protocol    Function pointer for protocol implementation
*
* @return   A non-NULL PROV_DEVICE_LL_HANDLE value that is used when invoking other functions
*           and NULL on Failure
*/
MOCKABLE_FUNCTION(, PROV_DEVICE_LL_HANDLE, Prov_Device_LL_Create, const char*, uri, const char*, scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, protocol);

/**
* @brief    Disposes of resources allocated by the provisioning Client.
*
* @param    handle  The handle created by a call to the create function
*
*/
MOCKABLE_FUNCTION(, void, Prov_Device_LL_Destroy, PROV_DEVICE_LL_HANDLE, handle);

/**
* @brief    Asynchronous call initiates the registration of a device.
*
* @param    handle              The handle created by a call to the create function.
* @param    register_callback   The callback that gets called on registration or if an error is encountered
* @param    user_context        User specified context that will be provided to the callback
* @param    reg_status_cb       An optional registration status callback used to inform the caller of registration status
* @param    status_user_ctext   User specified context that will be provided to the registration status callback
*
* @return PROV_DEVICE_RESULT_OK upon success or an error code upon failure
*/
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_Register_Device, PROV_DEVICE_LL_HANDLE, handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, reg_status_cb, void*, status_user_ctext);

/**
* @brief    Api to be called by user when work (registering device) can be done
*
* @param    handle  The handle created by a call to the create function.
*
*/
MOCKABLE_FUNCTION(, void, Prov_Device_LL_DoWork, PROV_DEVICE_LL_HANDLE, handle);

/**
* @brief    API sets a runtime option identified by parameter optionName to a value pointed to by value
*
* @param    handle          The handle created by a call to the create function.
* @param    optionName      The name of the option to be set
* @param    value           A pointer to the value of the option to be set
*
* @return PROV_DEVICE_RESULT_OK upon success or an error code upon failure
*/
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_SetOption, PROV_DEVICE_LL_HANDLE, handle, const char*, optionName, const void*, value);

/**
* @brief    API to get the version of the provisioning client
*
* @return The version number of the provisioning client
*/
MOCKABLE_FUNCTION(, const char*, Prov_Device_LL_GetVersionString);


/**
* @brief    Sets the Provisioning Data that is sent to the Provisioning service
*
* @param    handle          The handle created by a call to the create function.
* @param    json            The data field that is sent to the service.  Setting json to NULL will unset the value previously set
*
* @return PROV_DEVICE_RESULT_OK upon success or an error code upon failure
*/
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_Set_Provisioning_Payload, PROV_DEVICE_LL_HANDLE, handle, const char*, json);

/**
* @brief    Retrieves the Provisioning Data that is sent from the Provisioning service
*
* @param    handle          The handle created by a call to the create function.
*
* @return The data that was specified by the service
*/
MOCKABLE_FUNCTION(, const char*, Prov_Device_LL_Get_Provisioning_Payload, PROV_DEVICE_LL_HANDLE, handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_DEVICE_LL_CLIENT_H
