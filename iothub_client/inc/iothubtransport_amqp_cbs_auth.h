// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORT_AMQP_CBS_AUTH_H
#define IOTHUBTRANSPORT_AMQP_CBS_AUTH_H

#include <stdint.h>
#include "iothub_transport_ll.h"
#include "azure_uamqp_c/cbs.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum AUTHENTICATION_STATE_TAG
	{
		AUTHENTICATION_STATE_STOPPED,
		AUTHENTICATION_STATE_STARTING,
		AUTHENTICATION_STATE_STARTED,
		AUTHENTICATION_STATE_ERROR
	} AUTHENTICATION_STATE;

	typedef enum AUTHENTICATION_ERROR_TAG
	{
		AUTHENTICATION_ERROR_AUTH_TIMEOUT,
		AUTHENTICATION_ERROR_AUTH_FAILED,
		AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT,
		AUTHENTICATION_ERROR_SAS_REFRESH_FAILED
	} AUTHENTICATION_ERROR_CODE;

	typedef void(*ON_AUTHENTICATION_STATE_CHANGED_CALLBACK)(void* context, AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state);
	typedef void(*ON_AUTHENTICATION_ERROR_CALLBACK)(void* context, AUTHENTICATION_ERROR_CODE error_code);

	typedef struct AUTHENTICATION_CONFIG_TAG
	{
		char* device_id;
		char* device_primary_key;
		char* device_secondary_key;
		char* device_sas_token;
		char* iothub_host_fqdn;

		ON_AUTHENTICATION_STATE_CHANGED_CALLBACK on_state_changed_callback;
		void* on_state_changed_callback_context;

		ON_AUTHENTICATION_ERROR_CALLBACK on_error_callback;
		void* on_error_callback_context;
	} AUTHENTICATION_CONFIG;

	typedef struct AUTHENTICATION_INSTANCE* AUTHENTICATION_HANDLE;

	MOCKABLE_FUNCTION(, AUTHENTICATION_HANDLE, authentication_create, const AUTHENTICATION_CONFIG*, config);
	MOCKABLE_FUNCTION(, int, authentication_start, AUTHENTICATION_HANDLE, authentication_handle, const CBS_HANDLE, cbs_handle);
	MOCKABLE_FUNCTION(, int, authentication_stop, AUTHENTICATION_HANDLE, authentication_handle);
	MOCKABLE_FUNCTION(, void, authentication_do_work, AUTHENTICATION_HANDLE, authentication_handle);
	MOCKABLE_FUNCTION(, void, authentication_destroy, AUTHENTICATION_HANDLE, authentication_handle);

	MOCKABLE_FUNCTION(, int, authentication_set_cbs_request_timeout_secs, AUTHENTICATION_HANDLE, authentication_handle, uint32_t, value);
	MOCKABLE_FUNCTION(, int, authentication_set_sas_token_refresh_time_secs, AUTHENTICATION_HANDLE, authentication_handle, uint32_t, value);
	MOCKABLE_FUNCTION(, int, authentication_set_sas_token_lifetime_secs, AUTHENTICATION_HANDLE, authentication_handle, uint32_t, value);

#ifdef __cplusplus
}
#endif

#endif /*IOTHUBTRANSPORT_AMQP_CBS_AUTH_H*/
