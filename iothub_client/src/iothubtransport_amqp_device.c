// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "iothubtransport_amqp_messenger.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "iothubtransport_amqp_cbs_auth.h"
#include "iothubtransport_amqp_device.h"

#define RESULT_OK                                  0
#define INDEFINITE_TIME                            ((time_t)-1)
#define DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS    60
#define DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS    60

static const char* DEVICE_OPTION_SAVED_AUTH_OPTIONS = "saved_device_auth_options";
static const char* DEVICE_OPTION_SAVED_MESSENGER_OPTIONS = "saved_device_messenger_options";

typedef struct DEVICE_INSTANCE_TAG
{
	DEVICE_CONFIG* config;
	DEVICE_STATE state;

	SESSION_HANDLE session_handle;
	CBS_HANDLE cbs_handle;

	AUTHENTICATION_HANDLE authentication_handle;
	AUTHENTICATION_STATE auth_state;
	AUTHENTICATION_ERROR_CODE auth_error_code;
	time_t auth_state_last_changed_time;
	size_t auth_state_change_timeout_secs;

	MESSENGER_HANDLE messenger_handle;
	MESSENGER_STATE msgr_state;
	time_t msgr_state_last_changed_time;
	size_t msgr_state_change_timeout_secs;

	ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback;
	void* on_message_received_context;
} DEVICE_INSTANCE;

typedef struct SEND_EVENT_TASK_TAG
{
	ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_event_send_complete_callback;
	void* on_event_send_complete_context;
} SEND_EVENT_TASK;


// Internal state control

static void update_state(DEVICE_INSTANCE* instance, DEVICE_STATE new_state)
{
	if (new_state != instance->state)
	{
		DEVICE_STATE previous_state = instance->state;
		instance->state = new_state;

		if (instance->config->on_state_changed_callback != NULL)
		{
			instance->config->on_state_changed_callback(instance->config->on_state_changed_context, previous_state, new_state);
		}
	}
}

static int is_timeout_reached(time_t start_time, size_t timeout_in_secs, int *is_timed_out)
{
	int result;

	if (start_time == INDEFINITE_TIME)
	{
		LogError("Failed to verify timeout (start_time is INDEFINITE)");
		result = __FAILURE__;
	}
	else
	{
		time_t current_time;

		if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			LogError("Failed to verify timeout (get_time failed)");
			result = __FAILURE__;
		}
		else
		{
			if (get_difftime(current_time, start_time) >= timeout_in_secs)
			{
				*is_timed_out = 1;
			}
			else
			{
				*is_timed_out = 0;
			}

			result = RESULT_OK;
		}
	}

	return result;
}


// Callback Handlers

static D2C_EVENT_SEND_RESULT get_d2c_event_send_result_from(MESSENGER_EVENT_SEND_COMPLETE_RESULT result)
{
	D2C_EVENT_SEND_RESULT d2c_esr;

	switch (result)
	{
		case MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK:
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_OK;
			break;
		case MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE:
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE;
			break;
		case MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING:
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
			break;
		case MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT:
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
			break;
		case MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED:
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED;
			break;
		default:
			// This is not expected. All states should be mapped.
			d2c_esr = D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_UNKNOWN;
			break;
	};

	return d2c_esr;
}

static void on_event_send_complete_messenger_callback(IOTHUB_MESSAGE_LIST* iothub_message, MESSENGER_EVENT_SEND_COMPLETE_RESULT ev_send_comp_result, void* context)
{
	if (iothub_message == NULL || context == NULL)
	{
		LogError("on_event_send_complete_messenger_callback was invoked, but either iothub_message (%p) or context (%p) are NULL", iothub_message, context);
	}
	else
	{
		SEND_EVENT_TASK* send_task = (SEND_EVENT_TASK*)context;

		// Codes_SRS_DEVICE_09_059: [If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK, D2C_EVENT_SEND_COMPLETE_RESULT_OK shall be reported as `event_send_complete`]
		// Codes_SRS_DEVICE_09_060: [If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE shall be reported as `event_send_complete`]
		// Codes_SRS_DEVICE_09_061: [If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING shall be reported as `event_send_complete`]
		// Codes_SRS_DEVICE_09_062: [If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT, D2C_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT shall be reported as `event_send_complete`]
		// Codes_SRS_DEVICE_09_063: [If `ev_send_comp_result` is MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED, D2C_EVENT_SEND_COMPLETE_RESULT_DEVICE_DESTROYED shall be reported as `event_send_complete`]
		D2C_EVENT_SEND_RESULT device_send_result = get_d2c_event_send_result_from(ev_send_comp_result);

		// Codes_SRS_DEVICE_09_064: [If provided, the user callback and context saved in `send_task` shall be invoked passing the device `event_send_complete`]
		if (send_task->on_event_send_complete_callback != NULL)
		{
			send_task->on_event_send_complete_callback(iothub_message, device_send_result, send_task->on_event_send_complete_context);
		}

		// Codes_SRS_DEVICE_09_065: [The memory allocated for `send_task` shall be released]
		free(send_task);
	}
}

static void on_authentication_error_callback(void* context, AUTHENTICATION_ERROR_CODE error_code)
{
	if (context == NULL)
	{
		LogError("on_authentication_error_callback was invoked with error %d, but context is NULL", error_code);
	}
	else
	{ 
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)context;
		instance->auth_error_code = error_code;
	}
}

static void on_authentication_state_changed_callback(void* context, AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state)
{
	if (context == NULL)
	{
		LogError("on_authentication_state_changed_callback was invoked with new_state %d, but context is NULL", new_state);
	}
	else if (new_state != previous_state)
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)context;
		instance->auth_state = new_state;

		if ((instance->auth_state_last_changed_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			LogError("Device '%s' failed to set time of last authentication state change (get_time failed)", instance->config->device_id);
		}
	}
}

static void on_messenger_state_changed_callback(void* context, MESSENGER_STATE previous_state, MESSENGER_STATE new_state)
{
	if (context == NULL)
	{
		LogError("on_messenger_state_changed_callback was invoked with new_state %d, but context is NULL", new_state);
	}
	else if (new_state != previous_state)
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)context;
		instance->msgr_state = new_state;

		if ((instance->msgr_state_last_changed_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			LogError("Device '%s' failed to set time of last messenger state change (get_time failed)", instance->config->device_id);
		}
	}
}

static MESSENGER_DISPOSITION_RESULT on_messenger_message_received_callback(IOTHUB_MESSAGE_HANDLE iothub_message_handle, void* context)
{
	MESSENGER_DISPOSITION_RESULT msgr_disposition_result;

	// Codes_SRS_DEVICE_09_070: [If `iothub_message_handle` or `context` is NULL, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ABANDONED]
	if (iothub_message_handle == NULL || context == NULL)
	{
		LogError("Failed receiving incoming C2D message (message handle (%p) or context (%p) is NULL)", iothub_message_handle, context);
		msgr_disposition_result = MESSENGER_DISPOSITION_RESULT_ABANDONED;
	}
	else
	{
		DEVICE_INSTANCE* device_instance = (DEVICE_INSTANCE*)context;

		if (device_instance->on_message_received_callback == NULL)
		{
			LogError("Device '%s' failed receiving incoming C2D message (callback is NULL)", device_instance->config->device_id);
			msgr_disposition_result = MESSENGER_DISPOSITION_RESULT_ABANDONED;
		}
		else
		{
			// Codes_SRS_DEVICE_09_071: [The user callback shall be invoked, passing the context it provided]
			switch (device_instance->on_message_received_callback(iothub_message_handle, device_instance->on_message_received_context))
			{
				// Codes_SRS_DEVICE_09_072: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ACCEPTED]
			case DEVICE_MESSAGE_DISPOSITION_RESULT_ACCEPTED:
				msgr_disposition_result = MESSENGER_DISPOSITION_RESULT_ACCEPTED;
				break;
				// Codes_SRS_DEVICE_09_073: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_REJECTED]
			case DEVICE_MESSAGE_DISPOSITION_RESULT_REJECTED:
				msgr_disposition_result = MESSENGER_DISPOSITION_RESULT_REJECTED;
				break;
				// Codes_SRS_DEVICE_09_074: [If the user callback returns DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED, on_messenger_message_received_callback shall return MESSENGER_DISPOSITION_RESULT_ABANDONED]
			case DEVICE_MESSAGE_DISPOSITION_RESULT_ABANDONED:
			default:
				msgr_disposition_result = MESSENGER_DISPOSITION_RESULT_ABANDONED;
				break;
			}
		}
	}

	return msgr_disposition_result;
}


// Configuration Helpers

static void destroy_device_config(DEVICE_CONFIG* config)
{
	if (config != NULL)
	{
		free(config->device_id);
		free(config->iothub_host_fqdn);
		free(config->device_primary_key);
		free(config->device_secondary_key);
		free(config->device_sas_token);
		free(config);
	}
}

static DEVICE_CONFIG* clone_device_config(DEVICE_CONFIG *config)
{
	DEVICE_CONFIG* new_config;

	if ((new_config = (DEVICE_CONFIG*)malloc(sizeof(DEVICE_CONFIG))) == NULL)
	{
		LogError("Failed copying the DEVICE_CONFIG (malloc failed)");
	}
	else
	{
		int result;

		memset(new_config, 0, sizeof(DEVICE_CONFIG));

		if (mallocAndStrcpy_s(&new_config->device_id, config->device_id) != RESULT_OK)
		{
			LogError("Failed copying the DEVICE_CONFIG (failed copying device_id)");
			result = __FAILURE__;
		}
		else if (mallocAndStrcpy_s(&new_config->iothub_host_fqdn, config->iothub_host_fqdn) != RESULT_OK)
		{
			LogError("Failed copying the DEVICE_CONFIG (failed copying iothub_host_fqdn)");
			result = __FAILURE__;
		}
		else if (config->device_sas_token != NULL &&
			mallocAndStrcpy_s(&new_config->device_sas_token, config->device_sas_token) != RESULT_OK)
		{
			LogError("Failed copying the DEVICE_CONFIG (failed copying device_sas_token)");
			result = __FAILURE__;
		}
		else if (config->device_primary_key != NULL &&
			mallocAndStrcpy_s(&new_config->device_primary_key, config->device_primary_key) != RESULT_OK)
		{
			LogError("Failed copying the DEVICE_CONFIG (failed copying device_primary_key)");
			result = __FAILURE__;
		}
		else if (config->device_secondary_key != NULL &&
			mallocAndStrcpy_s(&new_config->device_secondary_key, config->device_secondary_key) != RESULT_OK)
		{
			LogError("Failed copying the DEVICE_CONFIG (failed copying device_secondary_key)");
			result = __FAILURE__;
		}
		else
		{
			new_config->authentication_mode = config->authentication_mode;
			new_config->on_state_changed_callback = config->on_state_changed_callback;
			new_config->on_state_changed_context = config->on_state_changed_context;

			result = RESULT_OK;
		}

		if (result != RESULT_OK)
		{
			destroy_device_config(new_config);
			new_config = NULL;
		}
	}

	return new_config;
}

static void set_authentication_config(DEVICE_INSTANCE* device_instance, AUTHENTICATION_CONFIG* auth_config)
{
	DEVICE_CONFIG *device_config = device_instance->config;

	auth_config->device_id = device_config->device_id;
	auth_config->iothub_host_fqdn = device_config->iothub_host_fqdn;
	auth_config->on_error_callback = on_authentication_error_callback;
	auth_config->on_error_callback_context = device_instance;
	auth_config->on_state_changed_callback = on_authentication_state_changed_callback;
	auth_config->on_state_changed_callback_context = device_instance;
	auth_config->device_primary_key = device_config->device_primary_key;
	auth_config->device_secondary_key = device_config->device_secondary_key;
	auth_config->device_sas_token = device_config->device_sas_token;
}


// Create and Destroy Helpers

static void internal_destroy_device(DEVICE_INSTANCE* instance)
{
	if (instance != NULL)
	{
		if (instance->messenger_handle != NULL)
		{
			messenger_destroy(instance->messenger_handle);
		}

		if (instance->authentication_handle != NULL)
		{
			authentication_destroy(instance->authentication_handle);
		}

		destroy_device_config(instance->config);
		free(instance);
	}
}

static int create_authentication_instance(DEVICE_INSTANCE *instance)
{
	int result;
	AUTHENTICATION_CONFIG auth_config;

	set_authentication_config(instance, &auth_config);

	if ((instance->authentication_handle = authentication_create(&auth_config)) == NULL)
	{
		LogError("Failed creating the AUTHENTICATION_HANDLE (authentication_create failed)");
		result = __FAILURE__;
	}
	else
	{
		result = RESULT_OK;
	}

	return result;
}

static int create_messenger_instance(DEVICE_INSTANCE* instance)
{
	int result;

	MESSENGER_CONFIG messenger_config;
	messenger_config.device_id = instance->config->device_id;
	messenger_config.iothub_host_fqdn = instance->config->iothub_host_fqdn;
	messenger_config.on_state_changed_callback = on_messenger_state_changed_callback;
	messenger_config.on_state_changed_context = instance;

	if ((instance->messenger_handle = messenger_create(&messenger_config)) == NULL)
	{
		LogError("Failed creating the MESSENGER_HANDLE (messenger_create failed)");
		result = __FAILURE__;
	}
	else
	{
		result = RESULT_OK;
	}

	return result;
}

// ---------- Set/Retrieve Options Helpers ----------//

static void* device_clone_option(const char* name, const void* value)
{
	void* result;

	if (name == NULL || value == NULL)
	{
		LogError("Failed to clone device option (either name (%p) or value (%p) is NULL)", name, value);
		result = NULL;
	}
	else
	{
		if (strcmp(DEVICE_OPTION_SAVED_AUTH_OPTIONS, name) == 0 ||
			strcmp(DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, name) == 0)
		{
			if ((result = (void*)OptionHandler_Clone((OPTIONHANDLER_HANDLE)value)) == NULL)
			{
				LogError("Failed to clone device option (OptionHandler_Clone failed for option %s)", name);
			}
		}
		else
		{
			LogError("Failed to clone device option (option with name '%s' is not suppported)", name);
			result = NULL;
		}
	}

	return result;
}

static void device_destroy_option(const char* name, const void* value)
{
	if (name == NULL || value == NULL)
	{
		LogError("Failed to destroy device option (either name (%p) or value (%p) is NULL)", name, value);
	}
	else
	{
		if (strcmp(name, DEVICE_OPTION_SAVED_AUTH_OPTIONS) == 0 ||
			strcmp(name, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS) == 0)
		{
			OptionHandler_Destroy((OPTIONHANDLER_HANDLE)value);
		}
		else
		{
			LogError("Failed to clone device option (option with name '%s' is not suppported)", name);
		}
	}
}


// Public APIs:

DEVICE_HANDLE device_create(DEVICE_CONFIG *config)
{
	DEVICE_INSTANCE *instance;

	// Codes_SRS_DEVICE_09_001: [If `config` or device_id or iothub_host_fqdn or on_state_changed_callback are NULL then device_create shall fail and return NULL]
	if (config == NULL)
	{
		LogError("Failed creating the device instance (config is NULL)");
		instance = NULL;
	}
	else if (config->device_id == NULL)
	{
		LogError("Failed creating the device instance (config->device_id is NULL)");
		instance = NULL;
	}
	else if (config->iothub_host_fqdn == NULL)
	{
		LogError("Failed creating the device instance (config->iothub_host_fqdn is NULL)");
		instance = NULL;
	}
	else if (config->on_state_changed_callback == NULL)
	{
		LogError("Failed creating the device instance (config->on_state_changed_callback is NULL)");
		instance = NULL;
	}
	// Codes_SRS_DEVICE_09_002: [device_create shall allocate memory for the device instance structure]
	else if ((instance = (DEVICE_INSTANCE*)malloc(sizeof(DEVICE_INSTANCE))) == NULL)
	{
		// Codes_SRS_DEVICE_09_003: [If malloc fails, device_create shall fail and return NULL]
		LogError("Failed creating the device instance (malloc failed)");
	}
	else
	{
		int result;

		memset(instance, 0, sizeof(DEVICE_INSTANCE));

		// Codes_SRS_DEVICE_09_004: [All `config` parameters shall be saved into `instance`]
		if ((instance->config = clone_device_config(config)) == NULL)
		{
			// Codes_SRS_DEVICE_09_005: [If any `config` parameters fail to be saved into `instance`, device_create shall fail and return NULL]
			LogError("Failed creating the device instance for device '%s' (failed copying the configuration)", config->device_id);
			result = __FAILURE__;
		}
		// Codes_SRS_DEVICE_09_006: [If `instance->authentication_mode` is DEVICE_AUTH_MODE_CBS, `instance->authentication_handle` shall be set using authentication_create()]
		else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
			     create_authentication_instance(instance) != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_007: [If the AUTHENTICATION_HANDLE fails to be created, device_create shall fail and return NULL]
			LogError("Failed creating the device instance for device '%s' (failed creating the authentication instance)", instance->config->device_id);
			result = __FAILURE__;
		}
		// Codes_SRS_DEVICE_09_008: [`instance->messenger_handle` shall be set using messenger_create()]
		else if (create_messenger_instance(instance) != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_009: [If the MESSENGER_HANDLE fails to be created, device_create shall fail and return NULL]
			LogError("Failed creating the device instance for device '%s' (failed creating the messenger instance)", instance->config->device_id);
			result = __FAILURE__;
		}
		else
		{
			instance->auth_state = AUTHENTICATION_STATE_STOPPED;
			instance->msgr_state = MESSENGER_STATE_STOPPED;
			instance->state = DEVICE_STATE_STOPPED;
			instance->auth_state_last_changed_time = INDEFINITE_TIME;
			instance->auth_state_change_timeout_secs = DEFAULT_AUTH_STATE_CHANGED_TIMEOUT_SECS;
			instance->msgr_state_last_changed_time = INDEFINITE_TIME;
			instance->msgr_state_change_timeout_secs = DEFAULT_MSGR_STATE_CHANGED_TIMEOUT_SECS;

			result = RESULT_OK;
		}

		if (result != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_010: [If device_create fails it shall release all memory it has allocated]
			internal_destroy_device(instance);
			instance = NULL;
		}
	}

	// Codes_SRS_DEVICE_09_011: [If device_create succeeds it shall return a handle to its `instance` structure]
	return (DEVICE_HANDLE)instance;
}

int device_start_async(DEVICE_HANDLE handle, SESSION_HANDLE session_handle, CBS_HANDLE cbs_handle)
{
	int result;

	// Codes_SRS_DEVICE_09_017: [If `handle` is NULL, device_start_async shall return a non-zero result]
	if (handle == NULL)
	{
		LogError("Failed starting device (handle is NULL)");
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		// Codes_SRS_DEVICE_09_018: [If the device state is not DEVICE_STATE_STOPPED, device_start_async shall return a non-zero result]
		if (instance->state != DEVICE_STATE_STOPPED)
		{
			LogError("Failed starting device (device is not stopped)");
			result = __FAILURE__;
		}
		// Codes_SRS_DEVICE_09_019: [If `session_handle` is NULL, device_start_async shall return a non-zero result]
		else if (session_handle == NULL)
		{
			LogError("Failed starting device (session_handle is NULL)");
			result = __FAILURE__;
		}
		// Codes_SRS_DEVICE_09_020: [If using CBS authentication and `cbs_handle` is NULL, device_start_async shall return a non-zero result]
		else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS && cbs_handle == NULL)
		{
			LogError("Failed starting device (device using CBS authentication, but cbs_handle is NULL)");
			result = __FAILURE__;
		}
		else
		{
			// Codes_SRS_DEVICE_09_021: [`session_handle` and `cbs_handle` shall be saved into the `instance`]
			instance->session_handle = session_handle;
			instance->cbs_handle = cbs_handle;

			// Codes_SRS_DEVICE_09_022: [The device state shall be updated to DEVICE_STATE_STARTING, and state changed callback invoked]
			update_state(instance, DEVICE_STATE_STARTING);

			// Codes_SRS_DEVICE_09_023: [If no failures occur, device_start_async shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

// @brief
//     stops a device instance (stops messenger and authentication) synchronously.
// @returns
//     0 if the function succeeds, non-zero otherwise.
int device_stop(DEVICE_HANDLE handle)
{
	int result;

	// Codes_SRS_DEVICE_09_024: [If `handle` is NULL, device_stop shall return a non-zero result]
	if (handle == NULL)
	{
		LogError("Failed stopping device (handle is NULL)");
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		// Codes_SRS_DEVICE_09_025: [If the device state is already DEVICE_STATE_STOPPED or DEVICE_STATE_STOPPING, device_stop shall return a non-zero result]
		if (instance->state == DEVICE_STATE_STOPPED || instance->state == DEVICE_STATE_STOPPING)
		{
			LogError("Failed stopping device '%s' (device is already stopped or stopping)", instance->config->device_id);
			result = __FAILURE__;
		}
		else
		{
			// Codes_SRS_DEVICE_09_026: [The device state shall be updated to DEVICE_STATE_STOPPING, and state changed callback invoked]
			update_state(instance, DEVICE_STATE_STOPPING);

			// Codes_SRS_DEVICE_09_027: [If `instance->messenger_handle` state is not MESSENGER_STATE_STOPPED, messenger_stop shall be invoked]
			if (instance->msgr_state != MESSENGER_STATE_STOPPED && 
				instance->msgr_state != MESSENGER_STATE_STOPPING &&
				messenger_stop(instance->messenger_handle) != RESULT_OK)
			{
				// Codes_SRS_DEVICE_09_028: [If messenger_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_MSG and the function shall return non-zero result]
				LogError("Failed stopping device '%s' (messenger_stop failed)", instance->config->device_id);
				result = __FAILURE__;
				update_state(instance, DEVICE_STATE_ERROR_MSG);
			}
			// Codes_SRS_DEVICE_09_029: [If CBS authentication is used, if `instance->authentication_handle` state is not AUTHENTICATION_STATE_STOPPED, authentication_stop shall be invoked]
			else if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
				instance->auth_state != AUTHENTICATION_STATE_STOPPED &&
				authentication_stop(instance->authentication_handle) != RESULT_OK)
			{
				// Codes_SRS_DEVICE_09_030: [If authentication_stop fails, the `instance` state shall be updated to DEVICE_STATE_ERROR_AUTH and the function shall return non-zero result]
				LogError("Failed stopping device '%s' (authentication_stop failed)", instance->config->device_id);
				result = __FAILURE__;
				update_state(instance, DEVICE_STATE_ERROR_AUTH);
			}
			else
			{
				// Codes_SRS_DEVICE_09_031: [The device state shall be updated to DEVICE_STATE_STOPPED, and state changed callback invoked]
				update_state(instance, DEVICE_STATE_STOPPED);

				// Codes_SRS_DEVICE_09_032: [If no failures occur, device_stop shall return 0]
				result = RESULT_OK;
			}
		}
	}

	return result;
}

void device_do_work(DEVICE_HANDLE handle)
{
	// Codes_SRS_DEVICE_09_033: [If `handle` is NULL, device_do_work shall return]
	if (handle == NULL)
	{
		LogError("Failed to perform device_do_work (handle is NULL)");
	}
	else
	{
		// Cranking the state monster:
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		if (instance->state == DEVICE_STATE_STARTING)
		{
			// Codes_SRS_DEVICE_09_034: [If CBS authentication is used and authentication state is AUTHENTICATION_STATE_STOPPED, authentication_start shall be invoked]
			if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS)
			{
				if (instance->auth_state == AUTHENTICATION_STATE_STOPPED)
				{
					if (authentication_start(instance->authentication_handle, instance->cbs_handle) != RESULT_OK)
					{
						// Codes_SRS_DEVICE_09_035: [If authentication_start fails, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
						LogError("Device '%s' failed to be authenticated (authentication_start failed)", instance->config->device_id);

						update_state(instance, DEVICE_STATE_ERROR_AUTH);
					}
				}
				// Codes_SRS_DEVICE_09_036: [If authentication state is AUTHENTICATION_STATE_STARTING, the device shall track the time since last event change and timeout if needed]
				else if (instance->auth_state == AUTHENTICATION_STATE_STARTING)
				{
					int is_timed_out;
					if (is_timeout_reached(instance->auth_state_last_changed_time, instance->auth_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
					{
						LogError("Device '%s' failed verifying the timeout for authentication start (is_timeout_reached failed)", instance->config->device_id);
						update_state(instance, DEVICE_STATE_ERROR_AUTH);
					}
					// Codes_SRS_DEVICE_09_037: [If authentication_start times out, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT]
					else if (is_timed_out == 1)
					{
						LogError("Device '%s' authentication did not complete starting within expected timeout (%d)", instance->config->device_id, instance->auth_state_change_timeout_secs);

						update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
					}
				}
				else if (instance->auth_state == AUTHENTICATION_STATE_ERROR)
				{
					// Codes_SRS_DEVICE_09_038: [If authentication state is AUTHENTICATION_STATE_ERROR and error code is AUTH_FAILED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
					if (instance->auth_error_code == AUTHENTICATION_ERROR_AUTH_FAILED)
					{
						update_state(instance, DEVICE_STATE_ERROR_AUTH);
					}
					// Codes_SRS_DEVICE_09_039: [If authentication state is AUTHENTICATION_STATE_ERROR and error code is TIMEOUT, the device state shall be updated to DEVICE_STATE_ERROR_AUTH_TIMEOUT]
					else // DEVICE_STATE_ERROR_TIMEOUT
					{
						update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
					}
				}
				// There is no AUTHENTICATION_STATE_STOPPING
			}

			// Codes_SRS_DEVICE_09_040: [Messenger shall not be started if using CBS authentication and authentication start has not completed yet]
			if (instance->config->authentication_mode == DEVICE_AUTH_MODE_X509 || instance->auth_state == AUTHENTICATION_STATE_STARTED)
			{
				// Codes_SRS_DEVICE_09_041: [If messenger state is MESSENGER_STATE_STOPPED, messenger_start shall be invoked]
				if (instance->msgr_state == MESSENGER_STATE_STOPPED)
				{
					// Codes_SRS_DEVICE_09_042: [If messenger_start fails, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
					if (messenger_start(instance->messenger_handle, instance->session_handle) != RESULT_OK)
					{
						LogError("Device '%s' messenger failed to be started (messenger_start failed)", instance->config->device_id);

						update_state(instance, DEVICE_STATE_ERROR_MSG);
					}
				}
				// Codes_SRS_DEVICE_09_043: [If messenger state is MESSENGER_STATE_STARTING, the device shall track the time since last event change and timeout if needed]
				else if (instance->msgr_state == MESSENGER_STATE_STARTING)
				{
					int is_timed_out;
					if (is_timeout_reached(instance->msgr_state_last_changed_time, instance->msgr_state_change_timeout_secs, &is_timed_out) != RESULT_OK)
					{
						LogError("Device '%s' failed verifying the timeout for messenger start (is_timeout_reached failed)", instance->config->device_id);

						update_state(instance, DEVICE_STATE_ERROR_MSG);
					}
					// Codes_SRS_DEVICE_09_044: [If messenger_start times out, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
					else if (is_timed_out == 1)
					{
						LogError("Device '%s' messenger did not complete starting within expected timeout (%d)", instance->config->device_id, instance->msgr_state_change_timeout_secs);

						update_state(instance, DEVICE_STATE_ERROR_MSG);
					}
				}
				// Codes_SRS_DEVICE_09_045: [If messenger state is MESSENGER_STATE_ERROR, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
				else if (instance->msgr_state == MESSENGER_STATE_ERROR)
				{
					LogError("Device '%s' messenger failed to be started (messenger got into error state)", instance->config->device_id);
					
					update_state(instance, DEVICE_STATE_ERROR_MSG);
				}
				// Codes_SRS_DEVICE_09_046: [If messenger state is MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_STARTED]
				else if (instance->msgr_state == MESSENGER_STATE_STARTED)
				{
					update_state(instance, DEVICE_STATE_STARTED);
				}
			}
		}
		else if (instance->state == DEVICE_STATE_STARTED)
		{
			// Codes_SRS_DEVICE_09_047: [If CBS authentication is used and authentication state is not AUTHENTICATION_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_AUTH]
			if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS &&
				instance->auth_state != AUTHENTICATION_STATE_STARTED)
			{
				LogError("Device '%s' is started but authentication reported unexpected state %d", instance->config->device_id, instance->auth_state);

				if (instance->auth_state != AUTHENTICATION_STATE_ERROR)
				{
					if (instance->auth_error_code == AUTHENTICATION_ERROR_AUTH_FAILED)
					{
						update_state(instance, DEVICE_STATE_ERROR_AUTH);
					}
					else // AUTHENTICATION_ERROR_AUTH_TIMEOUT
					{
						update_state(instance, DEVICE_STATE_ERROR_AUTH_TIMEOUT);
					}
				}
				else
				{
					update_state(instance, DEVICE_STATE_ERROR_AUTH);
				}
			}
			else
			{
				// Codes_SRS_DEVICE_09_048: [If messenger state is not MESSENGER_STATE_STARTED, the device state shall be updated to DEVICE_STATE_ERROR_MSG]
				if (instance->msgr_state != MESSENGER_STATE_STARTED)
				{
					LogError("Device '%s' is started but messenger reported unexpected state %d", instance->config->device_id, instance->msgr_state);
					update_state(instance, DEVICE_STATE_ERROR_MSG);
				}
			}
		}

		// Invoking the do_works():
		if (instance->config->authentication_mode == DEVICE_AUTH_MODE_CBS)
		{
			if (instance->auth_state != AUTHENTICATION_STATE_STOPPED && instance->auth_state != AUTHENTICATION_STATE_ERROR)
			{
				// Codes_SRS_DEVICE_09_049: [If CBS is used for authentication and `instance->authentication_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked]
				authentication_do_work(instance->authentication_handle);
			}
		}

		if (instance->msgr_state != MESSENGER_STATE_STOPPED && instance->msgr_state != MESSENGER_STATE_ERROR)
		{
			// Codes_SRS_DEVICE_09_050: [If `instance->messenger_handle` state is not STOPPED or ERROR, authentication_do_work shall be invoked]
			messenger_do_work(instance->messenger_handle);
		}
	}
}

void device_destroy(DEVICE_HANDLE handle)
{
	// Codes_SRS_DEVICE_09_012: [If `handle` is NULL, device_destroy shall return]
	if (handle == NULL)
	{
		LogError("Failed destroying device handle (handle is NULL)");
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;
		// Codes_SRS_DEVICE_09_013: [If the device is in state DEVICE_STATE_STARTED or DEVICE_STATE_STARTING, device_stop() shall be invoked]
		if (instance->state == DEVICE_STATE_STARTED || instance->state == DEVICE_STATE_STARTING)
		{
			(void)device_stop((DEVICE_HANDLE)instance);
		}

		// Codes_SRS_DEVICE_09_014: [`instance->messenger_handle shall be destroyed using messenger_destroy()`]
		// Codes_SRS_DEVICE_09_015: [If created, `instance->authentication_handle` shall be destroyed using authentication_destroy()`]
		// Codes_SRS_DEVICE_09_016: [The contents of `instance->config` shall be detroyed and then it shall be freed]
		internal_destroy_device((DEVICE_INSTANCE*)handle);
	}
}

int device_send_event_async(DEVICE_HANDLE handle, IOTHUB_MESSAGE_LIST* message, ON_DEVICE_D2C_EVENT_SEND_COMPLETE on_device_d2c_event_send_complete_callback, void* context)
{
	int result;

	// Codes_SRS_DEVICE_09_051: [If `handle` are `message` are NULL, device_send_event_async shall return a non-zero result]
	if (handle == NULL || message == NULL)
	{
		LogError("Failed sending event (either handle (%p) or message (%p) are NULL)", handle, message);
		result = __FAILURE__;
	}
	else
	{
		SEND_EVENT_TASK* send_task;

		// Codes_SRS_DEVICE_09_052: [A structure (`send_task`) shall be created to track the send state of the message]
		if ((send_task = (SEND_EVENT_TASK*)malloc(sizeof(SEND_EVENT_TASK))) == NULL)
		{
			// Codes_SRS_DEVICE_09_053: [If `send_task` fails to be created, device_send_event_async shall return a non-zero value]
			LogError("Failed sending event (failed creating task to send event)");
			result = __FAILURE__;
		}
		else
		{
			DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

			// Codes_SRS_DEVICE_09_054: [`send_task` shall contain the user callback and the context provided]
			memset(send_task, 0, sizeof(SEND_EVENT_TASK));
			send_task->on_event_send_complete_callback = on_device_d2c_event_send_complete_callback;
			send_task->on_event_send_complete_context = context;
			
			// Codes_SRS_DEVICE_09_055: [The message shall be sent using messenger_send_async, passing `on_event_send_complete_messenger_callback` and `send_task`]
			if (messenger_send_async(instance->messenger_handle, message, on_event_send_complete_messenger_callback, (void*)send_task) != RESULT_OK)
			{
				// Codes_SRS_DEVICE_09_056: [If messenger_send_async fails, device_send_event_async shall return a non-zero value]
				LogError("Failed sending event (messenger_send_async failed)");
				// Codes_SRS_DEVICE_09_057: [If any failures occur, device_send_event_async shall release all memory it has allocated]
				free(send_task);
				result = __FAILURE__;
			}
			else
			{	
				// Codes_SRS_DEVICE_09_058: [If no failures occur, device_send_event_async shall return 0]
				result = RESULT_OK;
			}
		}
	}

	return result;
}

int device_get_send_status(DEVICE_HANDLE handle, DEVICE_SEND_STATUS *send_status)
{
	int result;


	// Codes_SRS_DEVICE_09_105: [If `handle` or `send_status` is NULL, device_get_send_status shall return a non-zero result]
	if (handle == NULL || send_status == NULL)
	{
		LogError("Failed getting the device messenger send status (NULL parameter received; handle=%p, send_status=%p)", handle, send_status);
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;
		MESSENGER_SEND_STATUS messenger_send_status;
		
		// Codes_SRS_DEVICE_09_106: [The status of `instance->messenger_handle` shall be obtained using messenger_get_send_status]
		if (messenger_get_send_status(instance->messenger_handle, &messenger_send_status) != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_107: [If messenger_get_send_status fails, device_get_send_status shall return a non-zero result]
			LogError("Failed getting the device messenger send status (messenger_get_send_status failed)");
			result = __FAILURE__;
		}
		else
		{
			// Codes_SRS_DEVICE_09_108: [If messenger_get_send_status returns MESSENGER_SEND_STATUS_IDLE, device_get_send_status return status DEVICE_SEND_STATUS_IDLE]
			if (messenger_send_status == MESSENGER_SEND_STATUS_IDLE)
			{
				*send_status = DEVICE_SEND_STATUS_IDLE;
			}
			// Codes_SRS_DEVICE_09_109: [If messenger_get_send_status returns MESSENGER_SEND_STATUS_BUSY, device_get_send_status return status DEVICE_SEND_STATUS_BUSY]
			else // i.e., messenger_send_status == MESSENGER_SEND_STATUS_BUSY
			{
				*send_status = DEVICE_SEND_STATUS_BUSY;
			}

			// Codes_SRS_DEVICE_09_110: [If device_get_send_status succeeds, it shall return zero as result]
			result = RESULT_OK;
		}
	}

	return result;
}

int device_subscribe_message(DEVICE_HANDLE handle, ON_DEVICE_C2D_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
	int result;

	// Codes_SRS_DEVICE_09_066: [If `handle` or `on_message_received_callback` or `context` is NULL, device_subscribe_message shall return a non-zero result]
	if (handle == NULL || on_message_received_callback == NULL || context == NULL)
	{
		LogError("Failed subscribing to C2D messages (either handle (%p), on_message_received_callback (%p) or context (%p) is NULL)",
			handle, on_message_received_callback, context);
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		// Codes_SRS_DEVICE_09_067: [messenger_subscribe_for_messages shall be invoked passing `on_messenger_message_received_callback` and the user callback and context]
		if (messenger_subscribe_for_messages(instance->messenger_handle, on_messenger_message_received_callback, handle) != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_068: [If messenger_subscribe_for_messages fails, device_subscribe_message shall return a non-zero result]
			LogError("Failed subscribing to C2D messages (messenger_subscribe_for_messages failed)");
			result = __FAILURE__;
		}
		else
		{
			instance->on_message_received_callback = on_message_received_callback;
			instance->on_message_received_context = context;

			// Codes_SRS_DEVICE_09_069: [If no failures occur, device_subscribe_message shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

int device_unsubscribe_message(DEVICE_HANDLE handle)
{
	int result;

	// Codes_SRS_DEVICE_09_076: [If `handle` is NULL, device_unsubscribe_message shall return a non-zero result]
	if (handle == NULL)
	{
		LogError("Failed unsubscribing to C2D messages (handle is NULL)");
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		// Codes_SRS_DEVICE_09_077: [messenger_unsubscribe_for_messages shall be invoked passing `instance->messenger_handle`]
		if (messenger_unsubscribe_for_messages(instance->messenger_handle) != RESULT_OK)
		{
			// Codes_SRS_DEVICE_09_078: [If messenger_unsubscribe_for_messages fails, device_unsubscribe_message shall return a non-zero result]
			LogError("Failed unsubscribing to C2D messages (messenger_unsubscribe_for_messages failed)");
			result = __FAILURE__;
		}
		else
		{
			// Codes_SRS_DEVICE_09_079: [If no failures occur, device_unsubscribe_message shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

int device_set_retry_policy(DEVICE_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY policy, size_t retry_timeout_limit_in_seconds)
{
	(void)retry_timeout_limit_in_seconds;
	(void)policy;
	int result;

	// Codes_SRS_DEVICE_09_080: [If `handle` is NULL, device_set_retry_policy shall return a non-zero result]
	if (handle == NULL)
	{
		LogError("Failed setting retry policy (handle is NULL)");
		result = __FAILURE__;
	}
	else
	{
		// Codes_SRS_DEVICE_09_081: [device_set_retry_policy shall return a non-zero result]
		LogError("Failed setting retry policy (functionality not supported)");
		result = __FAILURE__;
	}

	return result;
}

int device_set_option(DEVICE_HANDLE handle, const char* name, void* value)
{
	int result;

	// Codes_SRS_DEVICE_09_082: [If `handle` or `name` or `value` are NULL, device_set_option shall return a non-zero result]
	if (handle == NULL || name == NULL || value == NULL)
	{
		LogError("failed setting device option (one of the followin are NULL: _handle=%p, name=%p, value=%p)",
			handle, name, value);
		result = __FAILURE__;
	}
	else
	{
		DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

		if (strcmp(DEVICE_OPTION_CBS_REQUEST_TIMEOUT_SECS, name) == 0 ||
			strcmp(DEVICE_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, name) == 0 ||
			strcmp(DEVICE_OPTION_SAS_TOKEN_LIFETIME_SECS, name) == 0)
		{
			// Codes_SRS_DEVICE_09_083: [If `name` refers to authentication but CBS authentication is not used, device_set_option shall return a non-zero result]
			if (instance->authentication_handle == NULL)
			{
				LogError("failed setting option for device '%s' (cannot set authentication option '%s'; not using CBS authentication)", instance->config->device_id, name);
				result = __FAILURE__;
			}
			// Codes_SRS_DEVICE_09_084: [If `name` refers to authentication, it shall be passed along with `value` to authentication_set_option]
			else if(authentication_set_option(instance->authentication_handle, name, value) != RESULT_OK)
			{
				// Codes_SRS_DEVICE_09_085: [If authentication_set_option fails, device_set_option shall return a non-zero result]
				LogError("failed setting option for device '%s' (failed setting authentication option '%s')", instance->config->device_id, name);
				result = __FAILURE__;
			}
			else
			{
				result = RESULT_OK;
			}
		}
		else if (strcmp(DEVICE_OPTION_EVENT_SEND_TIMEOUT_SECS, name) == 0)
		{
			// Codes_SRS_DEVICE_09_086: [If `name` refers to messenger module, it shall be passed along with `value` to messenger_set_option]
			if (messenger_set_option(instance->messenger_handle, name, value) != RESULT_OK)
			{
				// Codes_SRS_DEVICE_09_087: [If messenger_set_option fails, device_set_option shall return a non-zero result]
				LogError("failed setting option for device '%s' (failed setting messenger option '%s')", instance->config->device_id, name);
				result = __FAILURE__;
			}
			else
			{
				result = RESULT_OK;
			}
		}
		else if (strcmp(DEVICE_OPTION_SAVED_AUTH_OPTIONS, name) == 0)
		{
			// Codes_SRS_DEVICE_09_088: [If `name` is DEVICE_OPTION_SAVED_AUTH_OPTIONS but CBS authentication is not being used, device_set_option shall return a non-zero result]
			if (instance->authentication_handle == NULL)
			{
				LogError("failed setting option for device '%s' (cannot set authentication option '%s'; not using CBS authentication)", instance->config->device_id, name);
				result = __FAILURE__;
			}
			else if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, instance->authentication_handle) != OPTIONHANDLER_OK)
			{
				// Codes_SRS_DEVICE_09_091: [If any call to OptionHandler_FeedOptions fails, device_set_option shall return a non-zero result]
				LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed for authentication instance)", instance->config->device_id);
				result = __FAILURE__;
			}
			else
			{
				result = RESULT_OK;
			}
		}
		else if (strcmp(DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, name) == 0)
		{
			// Codes_SRS_DEVICE_09_089: [If `name` is DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, `value` shall be fed to `instance->messenger_handle` using OptionHandler_FeedOptions]
			if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, instance->messenger_handle) != OPTIONHANDLER_OK)
			{
				// Codes_SRS_DEVICE_09_091: [If any call to OptionHandler_FeedOptions fails, device_set_option shall return a non-zero result]
				LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed for messenger instance)", instance->config->device_id);
				result = __FAILURE__;
			}
			else
			{
				result = RESULT_OK;
			}
		}
		else if (strcmp(DEVICE_OPTION_SAVED_OPTIONS, name) == 0)
		{
			// Codes_SRS_DEVICE_09_090: [If `name` is DEVICE_OPTION_SAVED_OPTIONS, `value` shall be fed to `instance` using OptionHandler_FeedOptions]
			if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, handle) != OPTIONHANDLER_OK)
			{
				// Codes_SRS_DEVICE_09_091: [If any call to OptionHandler_FeedOptions fails, device_set_option shall return a non-zero result]
				LogError("failed setting option for device '%s' (OptionHandler_FeedOptions failed)", instance->config->device_id);
				result = __FAILURE__;
			}
			else
			{
				result = RESULT_OK;
			}
		}
		else
		{
			// Codes_SRS_DEVICE_09_092: [If no failures occur, device_set_option shall return 0]
			LogError("failed setting option for device '%s' (option with name '%s' is not suppported)", instance->config->device_id, name);
			result = __FAILURE__;
		}
	}

	return result;
}

OPTIONHANDLER_HANDLE device_retrieve_options(DEVICE_HANDLE handle)
{
	OPTIONHANDLER_HANDLE result;

	// Codes_SRS_DEVICE_09_093: [If `handle` is NULL, device_retrieve_options shall return NULL]
	if (handle == NULL)
	{
		LogError("Failed to retrieve options from device instance (handle is NULL)");
		result = NULL;
	}
	else
	{
		// Codes_SRS_DEVICE_09_094: [A OPTIONHANDLER_HANDLE instance, aka `options` shall be created using OptionHandler_Create]
		OPTIONHANDLER_HANDLE options = OptionHandler_Create(device_clone_option, device_destroy_option, (pfSetOption)device_set_option);

		if (options == NULL)
		{
			// Codes_SRS_DEVICE_09_095: [If OptionHandler_Create fails, device_retrieve_options shall return NULL]
			LogError("Failed to retrieve options from device instance (OptionHandler_Create failed)");
			result = NULL;
		}
		else
		{
			DEVICE_INSTANCE* instance = (DEVICE_INSTANCE*)handle;

			OPTIONHANDLER_HANDLE dependency_options = NULL;

			// Codes_SRS_DEVICE_09_096: [If CBS authentication is used, `instance->authentication_handle` options shall be retrieved using authentication_retrieve_options]
			if (instance->authentication_handle != NULL &&
				(dependency_options = authentication_retrieve_options(instance->authentication_handle)) == NULL)
			{
				// Codes_SRS_DEVICE_09_097: [If authentication_retrieve_options fails, device_retrieve_options shall return NULL]
				LogError("Failed to retrieve options from device '%s' (failed to retrieve options from authentication instance)", instance->config->device_id);
				result = NULL;
			}
			// Codes_SRS_DEVICE_09_098: [The authentication options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_AUTH_OPTIONS]
			else if (instance->authentication_handle != NULL &&
				OptionHandler_AddOption(options, DEVICE_OPTION_SAVED_AUTH_OPTIONS, (const void*)dependency_options) != OPTIONHANDLER_OK)
			{
				// Codes_SRS_DEVICE_09_102: [If any call to OptionHandler_AddOption fails, device_retrieve_options shall return NULL]
				LogError("Failed to retrieve options from device '%s' (OptionHandler_AddOption failed for option '%s')", instance->config->device_id, DEVICE_OPTION_SAVED_AUTH_OPTIONS);
				result = NULL;
			}
			// Codes_SRS_DEVICE_09_099: [`instance->messenger_handle` options shall be retrieved using messenger_retrieve_options]
			else if ((dependency_options = messenger_retrieve_options(instance->messenger_handle)) == NULL)
			{
				// Codes_SRS_DEVICE_09_100: [If messenger_retrieve_options fails, device_retrieve_options shall return NULL]
				LogError("Failed to retrieve options from device '%s' (failed to retrieve options from messenger instance)", instance->config->device_id);
				result = NULL;
			}
			// Codes_SRS_DEVICE_09_101: [The messenger options shall be added to `options` using OptionHandler_AddOption as DEVICE_OPTION_SAVED_MESSENGER_OPTIONS]
			else if (OptionHandler_AddOption(options, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS, (const void*)dependency_options) != OPTIONHANDLER_OK)
			{
				// Codes_SRS_DEVICE_09_102: [If any call to OptionHandler_AddOption fails, device_retrieve_options shall return NULL]
				LogError("Failed to retrieve options from device '%s' (OptionHandler_AddOption failed for option '%s')", instance->config->device_id, DEVICE_OPTION_SAVED_MESSENGER_OPTIONS);
				result = NULL;
			}
			else
			{
				// Codes_SRS_DEVICE_09_104: [If no failures occur, a handle to `options` shall be return]
				result = options;
			}

			if (result == NULL)
			{
				// Codes_SRS_DEVICE_09_103: [If any failure occurs, any memory allocated by device_retrieve_options shall be destroyed]
				OptionHandler_Destroy(options);
			}
		}
	}

	return result;
}
