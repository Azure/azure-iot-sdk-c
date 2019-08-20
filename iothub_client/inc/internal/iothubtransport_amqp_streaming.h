// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORT_AMQP_STREAMING
#define IOTHUBTRANSPORT_AMQP_STREAMING

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/optionhandler.h"
#include "azure_uamqp_c/session.h"
#include "iothub_client_private.h"
#include "iothub_client_streaming.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct AMQP_STREAMING_CLIENT_TAG* AMQP_STREAMING_CLIENT_HANDLE;

	#define AMQP_STREAMING_CLIENT_STATE_VALUES \
		AMQP_STREAMING_CLIENT_STATE_STARTING, \
		AMQP_STREAMING_CLIENT_STATE_STARTED, \
		AMQP_STREAMING_CLIENT_STATE_STOPPING, \
		AMQP_STREAMING_CLIENT_STATE_STOPPED, \
		AMQP_STREAMING_CLIENT_STATE_ERROR
	
	MU_DEFINE_ENUM(AMQP_STREAMING_CLIENT_STATE, AMQP_STREAMING_CLIENT_STATE_VALUES);

	typedef void(*AMQP_STREAMING_CLIENT_STATE_CHANGED_CALLBACK)(const void* context, AMQP_STREAMING_CLIENT_STATE previous_state, AMQP_STREAMING_CLIENT_STATE new_state);

    typedef struct AMQP_STREAMING_CLIENT_CONFIG_TAG
	{
		pfTransport_GetOption_Product_Info_Callback prod_info_cb;
		void* prod_info_ctx;
		const char* device_id;
		const char* module_id;
		const char* iothub_host_fqdn;
		AMQP_STREAMING_CLIENT_STATE_CHANGED_CALLBACK on_state_changed_callback;
		const void* on_state_changed_context;
	} AMQP_STREAMING_CLIENT_CONFIG;

	MOCKABLE_FUNCTION(, AMQP_STREAMING_CLIENT_HANDLE, amqp_streaming_client_create, const AMQP_STREAMING_CLIENT_CONFIG*, client_config);
	MOCKABLE_FUNCTION(, int, amqp_streaming_client_start, AMQP_STREAMING_CLIENT_HANDLE, client_handle, SESSION_HANDLE, session_handle); 
	MOCKABLE_FUNCTION(, int, amqp_streaming_client_stop, AMQP_STREAMING_CLIENT_HANDLE, client_handle);
	MOCKABLE_FUNCTION(, void, amqp_streaming_client_do_work, AMQP_STREAMING_CLIENT_HANDLE, client_handle);
	MOCKABLE_FUNCTION(, void, amqp_streaming_client_destroy, AMQP_STREAMING_CLIENT_HANDLE, client_handle);
	MOCKABLE_FUNCTION(, int, amqp_streaming_client_set_option, AMQP_STREAMING_CLIENT_HANDLE, client_handle, const char*, name, void*, value);
	MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, amqp_streaming_client_retrieve_options, AMQP_STREAMING_CLIENT_HANDLE, client_handle);
    MOCKABLE_FUNCTION(, int, amqp_streaming_client_set_stream_request_callback, AMQP_STREAMING_CLIENT_HANDLE, client_handle, DEVICE_STREAM_C2D_REQUEST_CALLBACK, streamRequestCallback, void*, context);
    MOCKABLE_FUNCTION(, int, amqp_streaming_client_send_stream_response, AMQP_STREAMING_CLIENT_HANDLE, client, DEVICE_STREAM_C2D_RESPONSE*, streamResponse);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBTRANSPORT_AMQP_STREAMING*/
