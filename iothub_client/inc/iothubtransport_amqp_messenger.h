// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORT_AMQP_MESSENGER
#define IOTHUBTRANSPORT_AMQP_MESSENGER

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_uamqp_c/session.h"
#include "iothub_client_private.h"

#ifdef __cplusplus
extern "C"
{
#endif


static const char* MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS = "event_send_timeout_secs";
static const char* MESSENGER_OPTION_SAVED_OPTIONS = "saved_messenger_options";

typedef struct MESSENGER_INSTANCE* MESSENGER_HANDLE;

typedef enum MESSENGER_SEND_STATUS_TAG
{
	MESSENGER_SEND_STATUS_IDLE,
	MESSENGER_SEND_STATUS_BUSY
} MESSENGER_SEND_STATUS;

typedef enum MESSENGER_EVENT_SEND_COMPLETE_RESULT_TAG
{
	MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK,
	MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE,
	MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING,
	MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT,
	MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED
} MESSENGER_EVENT_SEND_COMPLETE_RESULT;

typedef enum MESSENGER_DISPOSITION_RESULT_TAG
{
	MESSENGER_DISPOSITION_RESULT_ACCEPTED,
	MESSENGER_DISPOSITION_RESULT_REJECTED,
	MESSENGER_DISPOSITION_RESULT_ABANDONED
} MESSENGER_DISPOSITION_RESULT;

typedef enum MESSENGER_STATE_TAG
{
	MESSENGER_STATE_STARTING,
	MESSENGER_STATE_STARTED,
	MESSENGER_STATE_STOPPING,
	MESSENGER_STATE_STOPPED,
	MESSENGER_STATE_ERROR
} MESSENGER_STATE;

typedef void(*ON_MESSENGER_EVENT_SEND_COMPLETE)(IOTHUB_MESSAGE_LIST* iothub_message_list, MESSENGER_EVENT_SEND_COMPLETE_RESULT messenger_event_send_complete_result, void* context);
typedef void(*ON_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, MESSENGER_STATE previous_state, MESSENGER_STATE new_state);
typedef MESSENGER_DISPOSITION_RESULT(*ON_MESSENGER_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, void* context);

typedef struct MESSENGER_CONFIG_TAG
{
	char* device_id;
	char* iothub_host_fqdn;
	ON_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
	void* on_state_changed_context;
} MESSENGER_CONFIG;

MOCKABLE_FUNCTION(, MESSENGER_HANDLE, messenger_create, const MESSENGER_CONFIG*, messenger_config);
MOCKABLE_FUNCTION(, int, messenger_send_async, MESSENGER_HANDLE, messenger_handle, IOTHUB_MESSAGE_LIST*, message, ON_MESSENGER_EVENT_SEND_COMPLETE, on_messenger_event_send_complete_callback, void*, context);
MOCKABLE_FUNCTION(, int, messenger_subscribe_for_messages, MESSENGER_HANDLE, messenger_handle, ON_MESSENGER_MESSAGE_RECEIVED, on_message_received_callback, void*, context);
MOCKABLE_FUNCTION(, int, messenger_unsubscribe_for_messages, MESSENGER_HANDLE, messenger_handle);
MOCKABLE_FUNCTION(, int, messenger_get_send_status, MESSENGER_HANDLE, messenger_handle, MESSENGER_SEND_STATUS*, send_status);
MOCKABLE_FUNCTION(, int, messenger_start, MESSENGER_HANDLE, messenger_handle, SESSION_HANDLE, session_handle);
MOCKABLE_FUNCTION(, int, messenger_stop, MESSENGER_HANDLE, messenger_handle);
MOCKABLE_FUNCTION(, void, messenger_do_work, MESSENGER_HANDLE, messenger_handle);
MOCKABLE_FUNCTION(, void, messenger_destroy, MESSENGER_HANDLE, messenger_handle);
MOCKABLE_FUNCTION(, int, messenger_set_option, MESSENGER_HANDLE, messenger_handle, const char*, name, void*, value);
MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, messenger_retrieve_options, MESSENGER_HANDLE, messenger_handle);


#ifdef __cplusplus
}
#endif

#endif /*IOTHUBTRANSPORT_AMQP_MESSENGER*/
