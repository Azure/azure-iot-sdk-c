// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORTAMQP_AMQP_DEVICE_H
#define IOTHUBTRANSPORTAMQP_AMQP_DEVICE_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_uamqp_c/session.h"
#include "azure_uamqp_c/cbs.h"
#include "iothub_message.h"
#include "iothub_client_private.h"
#include "iothubtransport_amqp_device.h"

#ifdef __cplusplus
extern "C"
{
#endif

// @brief    name of option to apply the instance obtained using device_retrieve_options
static const char* DEVICE_OPTION_SAVED_OPTIONS = "saved_device_options";
static const char* DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS = "event_send_timeout_secs";
static const char* DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS = "cbs_request_timeout_secs";
static const char* DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS = "sas_token_refresh_time_secs";
static const char* DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS = "sas_token_lifetime_secs";

typedef enum DEVICE_STATE_TAG
{
	DEVICE_STATE_STOPPED,
	DEVICE_STATE_STOPPING,
	DEVICE_STATE_STARTING,
	DEVICE_STATE_STARTED,
	DEVICE_STATE_ERROR_AUTH,
	DEVICE_STATE_ERROR_AUTH_TIMEOUT,
	DEVICE_STATE_ERROR_MSG
} DEVICE_STATE;

typedef enum DEVICE_AUTH_MODE_TAG
{
	DEVICE_AUTH_MODE_CBS,
	DEVICE_AUTH_MODE_X509
} DEVICE_AUTH_MODE;

typedef enum DEVICE_SEND_STATUS_TAG
{
	DEVICE_SEND_STATUS_IDLE,
	DEVICE_SEND_STATUS_BUSY
} DEVICE_SEND_STATUS;

typedef enum D2C_EVENT_SEND_RESULT_TAG
{
	D2C_EVENT_SEND_COMPLETE_RESULT_OK,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT,
	D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED,
	D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN
} D2C_EVENT_SEND_RESULT;

typedef enum DEVICE_MESSAGE_DISPOSITION_RESULT_TAG
{
	DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED,
	DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED
} DEVICE_MESSAGE_DISPOSITION_RESULT;

typedef void(*ON_DEVICE_STATE_CHANGED)(void* context, DEVICE_STATE previous_state, DEVICE_STATE new_state);
typedef DEVICE_MESSAGE_DISPOSITION_RESULT(*ON_DEVICE_C2D_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, void* context);
typedef void(*ON_DEVICE_D2C_EVENT_SEND_COMPLETE)(IOTHUB_MESSAGE_LIST* message, D2C_EVENT_SEND_RESULT result, void* context);

typedef struct DEVICE_CONFIG_TAG
{
	char* device_id;
	char* iothub_host_fqdn;
	DEVICE_AUTH_MODE authentication_mode;
	ON_DEVICE_STATE_CHANGED on_state_changed_callback;
	void* on_state_changed_context;

	char* device_primary_key;
	char* device_secondary_key;
	char* device_sas_token;
} DEVICE_CONFIG;

typedef struct DEVICE_INSTANCE* DEVICE_HANDLE;

MOCKABLE_FUNCTION(, DEVICE_HANDLE, device_create, DEVICE_CONFIG*, config);
MOCKABLE_FUNCTION(, void, device_destroy, DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, device_start_async, DEVICE_HANDLE, handle, SESSION_HANDLE, session_handle, CBS_HANDLE, cbs_handle);
MOCKABLE_FUNCTION(, int, device_stop, DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, device_do_work, DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, device_send_event_async, DEVICE_HANDLE, handle, IOTHUB_MESSAGE_LIST*, message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE, on_device_d2c_event_send_complete_callback, void*, context);
MOCKABLE_FUNCTION(, int, device_get_send_status, DEVICE_HANDLE, handle, DEVICE_SEND_STATUS, *send_status);
MOCKABLE_FUNCTION(, int, device_subscribe_message, DEVICE_HANDLE, handle, ON_DEVICE_C2D_MESSAGE_RECEIVED, on_message_received_callback, void*, context);
MOCKABLE_FUNCTION(, int, device_unsubscribe_message, DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, device_set_retry_policy, DEVICE_HANDLE, handle, IOTHUB_CLIENT_RETRY_POLICY, policy, size_t, retry_timeout_limit_in_seconds);
MOCKABLE_FUNCTION(, int, device_set_option, DEVICE_HANDLE, handle, const char*, name, void*, value);
MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, device_retrieve_options, DEVICE_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif // IOTHUBTRANSPORTAMQP_AMQP_DEVICE_H