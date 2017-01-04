// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORT_AMQP_MESSENGER
#define IOTHUBTRANSPORT_AMQP_MESSENGER

#include "iothub_message.h"
#include "azure_uamqp_c/session.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum MESSENGER_STATE_TAG
	{
		MESSENGER_STATE_STARTING,
		MESSENGER_STATE_STARTED,
		MESSENGER_STATE_STOPPED,
		MESSENGER_STATE_ERROR
	} MESSENGER_STATE;

	typedef void(*ON_MESSENGER_STATE_CHANGED_CALLBACK)(void* context, MESSENGER_STATE previous_state, MESSENGER_STATE new_state);

	typedef enum MESSENGER_DISPOSITION_RESULT_TAG
	{
		MESSENGER_DISPOSITION_RESULT_ACCEPTED,
		MESSENGER_DISPOSITION_RESULT_REJECTED,
		MESSENGER_DISPOSITION_RESULT_ABANDONED
	} MESSENGER_DISPOSITION_RESULT;

	typedef MESSENGER_DISPOSITION_RESULT(*ON_NEW_MESSAGE_RECEIVED)(IOTHUB_MESSAGE_HANDLE message, void* context);

	typedef struct MESSENGER_CONFIG_TAG
	{
		char* device_id;
		char* iothub_host_fqdn;
		PDLIST_ENTRY wait_to_send_list;
		ON_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_context;
	} MESSENGER_CONFIG;

	typedef struct MESSENGER_INSTANCE* MESSENGER_HANDLE;

	MOCKABLE_FUNCTION(, MESSENGER_HANDLE, messenger_create, const MESSENGER_CONFIG*, messenger_config);
	MOCKABLE_FUNCTION(, int, messenger_subscribe_for_messages, MESSENGER_HANDLE, messenger_handle, ON_NEW_MESSAGE_RECEIVED, on_message_received_callback, void*, context);
	MOCKABLE_FUNCTION(, int, messenger_unsubscribe_for_messages, MESSENGER_HANDLE, messenger_handle);
	MOCKABLE_FUNCTION(, int, messenger_start, MESSENGER_HANDLE, messenger_handle, SESSION_HANDLE, session_handle);
	MOCKABLE_FUNCTION(, int, messenger_stop, MESSENGER_HANDLE, messenger_handle);
	MOCKABLE_FUNCTION(, void, messenger_do_work, MESSENGER_HANDLE, messenger_handle);
	MOCKABLE_FUNCTION(, void, messenger_destroy, MESSENGER_HANDLE, messenger_handle);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBTRANSPORT_AMQP_MESSENGER*/
