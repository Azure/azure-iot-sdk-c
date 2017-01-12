// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubtransport_amqp_cbs_auth.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/sastoken.h"

#define RESULT_OK                                 0
#define INDEFINITE_TIME                           ((time_t)(-1))
#define SAS_TOKEN_TYPE                            "servicebus.windows.net:sastoken"
#define IOTHUB_DEVICES_PATH_FMT                   "%s/devices/%s"
#define DEFAULT_CBS_REQUEST_TIMEOUT_SECS          UINT32_MAX
#define DEFAULT_SAS_TOKEN_LIFETIME_SECS           3600
#define DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS       1800

typedef enum CREDENTIAL_TYPE_TAG
{
	CREDENTIAL_TYPE_NONE,
	DEVICE_PRIMARY_KEY,
	DEVICE_SECONDARY_KEY,
	USER_PROVIDED_SAS_TOKEN
} CREDENTIAL_TYPE;

typedef struct AUTHENTICATION_INSTANCE_TAG 
{
	STRING_HANDLE device_id;
	STRING_HANDLE iothub_host_fqdn;
	
	STRING_HANDLE device_sas_token;
	STRING_HANDLE device_primary_key;
	STRING_HANDLE device_secondary_key;

	ON_AUTHENTICATION_STATE_CHANGED_CALLBACK on_state_changed_callback;
	void* on_state_changed_callback_context;

	ON_AUTHENTICATION_ERROR_CALLBACK on_error_callback;
	void* on_error_callback_context;
	
	uint32_t cbs_request_timeout_secs;
	uint32_t sas_token_lifetime_secs;
	uint32_t sas_token_refresh_time_secs;

	AUTHENTICATION_STATE state;
	CBS_HANDLE cbs_handle;

	bool is_cbs_put_token_in_progress;
	bool is_sas_token_refresh_in_progress;

	time_t current_sas_token_put_time;

	CREDENTIAL_TYPE current_credential_in_use;
} AUTHENTICATION_INSTANCE;


// Helper functions:

static int get_seconds_since_epoch(double *seconds)
{
	int result;
	time_t current_time;

	if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
	{
		LogError("Failed getting the current local time (get_time() failed)");
		result = __LINE__;
	}
	else
	{
		*seconds = get_difftime(current_time, (time_t)0);

		result = RESULT_OK;
	}

	return result;
}

static void update_state(AUTHENTICATION_INSTANCE* instance, AUTHENTICATION_STATE new_state)
{
	if (new_state != instance->state)
	{
		AUTHENTICATION_STATE previous_state = instance->state;
		instance->state = new_state;

		if (instance->on_state_changed_callback != NULL)
		{
			instance->on_state_changed_callback(instance->on_state_changed_callback_context, previous_state, new_state);
		}
	}
}

static void notify_error(AUTHENTICATION_INSTANCE* instance, AUTHENTICATION_ERROR_CODE error_code)
{
	if (instance->on_error_callback != NULL)
	{
		instance->on_error_callback(instance->on_error_callback_context, error_code);
	}
}

static int verify_cbs_put_token_timeout(AUTHENTICATION_INSTANCE* instance, bool* is_timed_out)
{
	int result;

	if (instance->current_sas_token_put_time == INDEFINITE_TIME)
	{
		result = __LINE__;
		LogError("Failed verifying if cbs_put_token has timed out (current_sas_token_put_time is not set)");
	}
	else
	{
		time_t current_time;

		if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			result = __LINE__;
			LogError("Failed verifying if cbs_put_token has timed out (get_time failed)");
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_083: [authentication_do_work() shall check for authentication timeout comparing the current time since `instance->current_sas_token_put_time` to `instance->cbs_request_timeout_secs`]
		else if ((uint32_t)get_difftime(current_time, instance->current_sas_token_put_time) >= instance->cbs_request_timeout_secs)
		{
			*is_timed_out = true;
			result = RESULT_OK;
		}
		else
		{
			*is_timed_out = false;
			result = RESULT_OK;
		}
	}

	return result;
}

static int verify_sas_token_refresh_timeout(AUTHENTICATION_INSTANCE* instance, bool* is_timed_out)
{
	int result;

	if (instance->current_sas_token_put_time == INDEFINITE_TIME)
	{
		result = __LINE__;
		LogError("Failed verifying if SAS token refresh timed out (current_sas_token_put_time is not set)");
	}
	else
	{
		time_t current_time;

		if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			result = __LINE__;
			LogError("Failed verifying if SAS token refresh timed out (get_time failed)");
		}
		else if ((uint32_t)get_difftime(current_time, instance->current_sas_token_put_time) >= instance->sas_token_refresh_time_secs)
		{
			*is_timed_out = true;
			result = RESULT_OK;
		}
		else
		{
			*is_timed_out = false;
			result = RESULT_OK;
		}
	}

	return result;
}

static STRING_HANDLE create_devices_path(STRING_HANDLE iothub_host_fqdn, STRING_HANDLE device_id)
{
	STRING_HANDLE devices_path;

	if ((devices_path = STRING_new()) == NULL)
	{
		LogError("Failed creating devices_path (STRING_new failed)");
	}
	else if (STRING_sprintf(devices_path, IOTHUB_DEVICES_PATH_FMT, STRING_c_str(iothub_host_fqdn), STRING_c_str(device_id)) != RESULT_OK)
	{
		STRING_delete(devices_path);
		devices_path = NULL;
		LogError("Failed creating devices_path (STRING_sprintf failed)");
	}

	return devices_path;
}


static bool are_device_keys_used_for_authentication(AUTHENTICATION_INSTANCE* instance)
{
	return (instance->current_credential_in_use == DEVICE_PRIMARY_KEY || instance->current_credential_in_use == DEVICE_SECONDARY_KEY);
}

// @returns  0 there is one more device key to be attempted, !=0 otherwise.
static int mark_current_device_key_as_invalid(AUTHENTICATION_INSTANCE* instance)
{
	int result;

	if (instance->current_credential_in_use == DEVICE_PRIMARY_KEY)
	{
		if (instance->device_secondary_key != NULL)
		{
			instance->current_credential_in_use = DEVICE_SECONDARY_KEY;
			LogError("Primary key of device '%s' was marked as invalid. Using secondary key now", STRING_c_str(instance->device_id));
			result = 0;
		}
		else
		{
			instance->current_credential_in_use = CREDENTIAL_TYPE_NONE;
			LogError("Primary key of device '%s' was marked as invalid. No other device keys available", STRING_c_str(instance->device_id));
			result = __LINE__;
		}
	}
	else if (instance->current_credential_in_use == DEVICE_SECONDARY_KEY)
	{
		instance->current_credential_in_use = CREDENTIAL_TYPE_NONE;
		LogError("Secondary key of device '%s' was marked as invalid. No other device keys available", STRING_c_str(instance->device_id));
		result = __LINE__;
	}
	else
	{
		result = __LINE__;
	}

	return result;
}

static STRING_HANDLE get_current_valid_device_key(AUTHENTICATION_INSTANCE* instance)
{
	STRING_HANDLE device_key;

	switch (instance->current_credential_in_use)
	{
	case DEVICE_PRIMARY_KEY:
		device_key = instance->device_primary_key;
		break;
	case DEVICE_SECONDARY_KEY:
		device_key = instance->device_secondary_key;
		break;
	default:
		device_key = NULL;
		break;
	}

	return device_key;
}

static void on_cbs_put_token_complete_callback(void* context, CBS_OPERATION_RESULT operation_result, unsigned int status_code, const char* status_description)
{
#ifdef NO_LOGGING
	UNUSED(status_code);
	UNUSED(status_description);
#endif
	AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)context;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_095: [`instance->is_sas_token_refresh_in_progress` and `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
	instance->is_cbs_put_token_in_progress = false;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_091: [If `result` is CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_STARTED and `instance->on_state_changed_callback` invoked]
	if (operation_result == CBS_OPERATION_RESULT_OK)
	{
		update_state(instance, AUTHENTICATION_STATE_STARTED);
	}
	else
	{
		LogError("CBS reported status code %u, error: '%s' for put-token operation for device '%s'", status_code, status_description, STRING_c_str(instance->device_id));

		if (mark_current_device_key_as_invalid(instance) != 0)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_092: [If `result` is not CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
			update_state(instance, AUTHENTICATION_STATE_ERROR);

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_094: [If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED]
			if (instance->is_sas_token_refresh_in_progress)
			{
				notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED);
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_093: [If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
			else
			{
				notify_error(instance, AUTHENTICATION_ERROR_AUTH_FAILED);
			}
		}
	}

	instance->is_sas_token_refresh_in_progress = false;
}

static int put_SAS_token_to_cbs(AUTHENTICATION_INSTANCE* instance, STRING_HANDLE cbs_audience, STRING_HANDLE sas_token)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_043: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_057: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_075: [authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to TRUE]
	instance->is_cbs_put_token_in_progress = true;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_046: [The SAS token provided shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_058: [The SAS token shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_076: [The SAS token shall be sent to CBS using cbs_put_token(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback]
	if (cbs_put_token(instance->cbs_handle, SAS_TOKEN_TYPE, STRING_c_str(cbs_audience), STRING_c_str(sas_token), on_cbs_put_token_complete_callback, instance) != RESULT_OK)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_048: [If cbs_put_token() failed, authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to FALSE, destroy `devices_path` and return]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_060: [If cbs_put_token() fails, `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_078: [If cbs_put_token() fails, `instance->is_cbs_put_token_in_progress` shall be set to FALSE]
		instance->is_cbs_put_token_in_progress = false;
		result = __LINE__;
		LogError("Failed putting SAS token to CBS for device '%s' (cbs_put_token failed)", STRING_c_str(instance->device_id));
	}
	else
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_047: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_059: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_077: [If cbs_put_token() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with the current time]
		time_t current_time;

		if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
		{
			LogError("Failed setting current_sas_token_put_time for device '%s' (get_time() failed)", STRING_c_str(instance->device_id));
		}

		instance->current_sas_token_put_time = current_time; // If it failed, fear not. `current_sas_token_put_time` shall be checked for INDEFINITE_TIME wherever it is used.

		result = RESULT_OK;
	}

	return result;
}

static int create_and_put_SAS_token_to_cbs(AUTHENTICATION_INSTANCE* instance, STRING_HANDLE device_key)
{
	int result;
	double seconds_since_epoch;

	if (get_seconds_since_epoch(&seconds_since_epoch) != RESULT_OK)
	{
		result = __LINE__;
		LogError("Failed creating a SAS token (get_seconds_since_epoch() failed)");
	}
	else
	{
		STRING_HANDLE devices_path = NULL;
		STRING_HANDLE sasTokenKeyName = NULL;
		STRING_HANDLE sas_token = NULL;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_052: [The SAS token expiration time shall be calculated adding `instance->sas_token_lifetime_secs` to the current number of seconds since epoch time UTC]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_070: [The SAS token expiration time shall be calculated adding `instance->sas_token_lifetime_secs` to the current number of seconds since epoch time UTC]
		uint32_t sas_token_expiration_time_secs = (uint32_t)seconds_since_epoch + instance->sas_token_lifetime_secs;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_053: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_071: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
		if ((devices_path = create_devices_path(instance->iothub_host_fqdn, instance->device_id)) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_054: [If `devices_path` failed to be created, authentication_do_work() shall fail and return]
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_072: [If `devices_path` failed to be created, authentication_do_work() shall fail and return]		
			result = __LINE__;
			LogError("Failed creating a SAS token (create_devices_path() failed)");
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_115: [An empty STRING_HANDLE, referred to as `sasTokenKeyName`, shall be created using STRING_new()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_117: [An empty STRING_HANDLE, referred to as `sasTokenKeyName`, shall be created using STRING_new()]
		else if ((sasTokenKeyName = STRING_new()) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_116: [If `sasTokenKeyName` failed to be created, authentication_do_work() shall fail and return]
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_118: [If `sasTokenKeyName` failed to be created, authentication_do_work() shall fail and return]
			result = __LINE__;
			LogError("Failed creating a SAS token (STRING_new() failed)");
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_055: [The SAS token shall be created using SASToken_Create(), passing the selected device key, `device_path`, `sasTokenKeyName` and expiration time as arguments]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_073: [The SAS token shall be created using SASToken_Create(), passing the selected device key, device_path, sasTokenKeyName and expiration time as arguments]
		else if ((sas_token = SASToken_Create(device_key, devices_path, sasTokenKeyName, (size_t)sas_token_expiration_time_secs)) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_056: [If SASToken_Create() fails, authentication_do_work() shall fail and return]
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_074: [If SASToken_Create() fails, authentication_do_work() shall fail and return]
			result = __LINE__;
			LogError("Failed creating a SAS token (SASToken_Create() failed)");
		}
		else if (put_SAS_token_to_cbs(instance, devices_path, sas_token) != RESULT_OK)
		{
			result = __LINE__;
			LogError("Failed putting SAS token to CBS");
		}
		else
		{
			result = RESULT_OK;
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_081: [authentication_do_work() shall free the memory it allocated for `devices_path`, `sasTokenKeyName` and SAS token]
		if (devices_path != NULL)
			STRING_delete(devices_path);
		if (sasTokenKeyName != NULL)
			STRING_delete(sasTokenKeyName);
		if (sas_token != NULL)
			STRING_delete(sas_token);
	}

	return result;
}



// Public APIs:

int authentication_start(AUTHENTICATION_HANDLE authentication_handle, const CBS_HANDLE cbs_handle)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_025: [If authentication_handle is NULL, authentication_start() shall fail and return __LINE__ as error code]
	if (authentication_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_start failed (authentication_handle is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_026: [If `cbs_handle` is NULL, authentication_start() shall fail and return __LINE__ as error code]
	else if (cbs_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_start failed (cbs_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_027: [If authenticate state has been started already, authentication_start() shall fail and return __LINE__ as error code]
		if (instance->state != AUTHENTICATION_STATE_STOPPED)
		{
			result = __LINE__;
			LogError("authentication_start failed (messenger has already been started; current state: %d)", instance->state);
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_028: [authentication_start() shall save `cbs_handle` on `instance->cbs_handle`]
			instance->cbs_handle = cbs_handle;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_029: [If no failures occur, `instance->state` shall be set to AUTHENTICATION_STATE_STARTING and `instance->on_state_changed_callback` invoked]
			update_state(instance, AUTHENTICATION_STATE_STARTING);

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_030: [If no failures occur, authentication_start() shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

int authentication_stop(AUTHENTICATION_HANDLE authentication_handle)
{
	int result;
	
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_031: [If `authentication_handle` is NULL, authentication_stop() shall fail and return __LINE__]
	if (authentication_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_stop failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_032: [If `instance->state` is AUTHENTICATION_STATE_STOPPED, authentication_stop() shall fail and return __LINE__]
		if (instance->state == AUTHENTICATION_STATE_STOPPED)
		{
			result = __LINE__;
			LogError("authentication_stop failed (messenger is already stopped)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_033: [`instance->cbs_handle` shall be set to NULL]
			instance->cbs_handle = NULL;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_034: [`instance->state` shall be set to AUTHENTICATION_STATE_STOPPED and `instance->on_state_changed_callback` invoked]
			update_state(instance, AUTHENTICATION_STATE_STOPPED);

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_035: [authentication_stop() shall return success code 0]
			result = RESULT_OK;
		}
	}

	return result;
}

void authentication_destroy(AUTHENTICATION_HANDLE authentication_handle)
{
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_106: [If authentication_handle is NULL, authentication_destroy() shall return]
	if (authentication_handle == NULL)
	{
		LogError("authentication_destroy failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_107: [If `instance->state` is AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_stop() shall be invoked and its result ignored]
		if (instance->state != AUTHENTICATION_STATE_STOPPED)
		{
			(void)authentication_stop(authentication_handle);
		}
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_108: [authentication_destroy() shall destroy `instance->device_id` using STRING_delete()]
		if (instance->device_id != NULL)
			STRING_delete(instance->device_id);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_109: [authentication_destroy() shall destroy `instance->device_sas_token` using STRING_delete()]
		if (instance->device_sas_token != NULL)
			STRING_delete(instance->device_sas_token);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_110: [authentication_destroy() shall destroy `instance->device_primary_key` using STRING_delete()]
		if (instance->device_primary_key != NULL)
			STRING_delete(instance->device_primary_key);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_111: [authentication_destroy() shall destroy `instance->device_secondary_key` using STRING_delete()]
		if (instance->device_secondary_key != NULL)
			STRING_delete(instance->device_secondary_key);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_112: [authentication_destroy() shall destroy `instance->iothub_host_fqdn` using STRING_delete()]
		if (instance->iothub_host_fqdn != NULL)
			STRING_delete(instance->iothub_host_fqdn);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_113: [authentication_destroy() shall destroy `instance` using free()]
		free(instance);
	}
}

AUTHENTICATION_HANDLE authentication_create(const AUTHENTICATION_CONFIG* config)
{
	AUTHENTICATION_HANDLE result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_001: [If parameter `config` or `config->device_id` are NULL, authentication_create() shall fail and return NULL.]
	if (config == NULL)
	{
		result = NULL;
		LogError("authentication_create failed (config is NULL)");
	}
	else if (config->device_id == NULL)
	{
		result = NULL;
		LogError("authentication_create failed (config->device_id is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_002: [If device keys and SAS token are NULL, authentication_create() shall fail and return NULL.]
	else if (config->device_sas_token == NULL && config->device_primary_key == NULL)
	{
		result = NULL;
		LogError("authentication_create failed (either config->device_sas_token or config->device_primary_key must be provided; config->device_secondary_key is optional)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_003: [If device keys and SAS token are both provided, authentication_create() shall fail and return NULL.]
	else if (config->device_sas_token != NULL && (config->device_primary_key != NULL || config->device_secondary_key != NULL))
	{
		result = NULL;
		LogError("authentication_create failed (both config->device_sas_token and device device keys were provided; must provide only one)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_004: [If `config->iothub_host_fqdn` is NULL, authentication_create() shall fail and return NULL.]
	else if (config->iothub_host_fqdn == NULL)
	{
		result = NULL;
		LogError("authentication_create failed (config->iothub_host_fqdn is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_005: [If `config->on_state_changed_callback` is NULL, authentication_create() shall fail and return NULL]
	else if (config->on_state_changed_callback == NULL)
	{
		result = NULL;
		LogError("authentication_create failed (config->on_state_changed_callback is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_006: [authentication_create() shall allocate memory for a new authenticate state structure AUTHENTICATION_INSTANCE.]
		if ((instance = (AUTHENTICATION_INSTANCE*)malloc(sizeof(AUTHENTICATION_INSTANCE))) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_007: [If malloc() fails, authentication_create() shall fail and return NULL.]
			result = NULL;
			LogError("authentication_create failed (malloc failed)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_123: [authentication_create() shall initialize all fields of `instance` with 0 using memset().]
			memset(instance, 0, sizeof(AUTHENTICATION_INSTANCE));

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_008: [authentication_create() shall save a copy of `config->device_id` into the `instance->device_id`]
			if ((instance->device_id = STRING_construct(config->device_id)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_009: [If STRING_construct() fails, authentication_create() shall fail and return NULL]
				result = NULL;
				LogError("authentication_create failed (config->device_id could not be copied; STRING_construct failed)");
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_010: [If `device_config->device_sas_token` is not NULL, authentication_create() shall save a copy into the `instance->device_sas_token`]
			else if (config->device_sas_token != NULL && (instance->device_sas_token = STRING_construct(config->device_sas_token)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_011: [If STRING_construct() fails, authentication_create() shall fail and return NULL]
				result = NULL;
				LogError("authentication_create failed (config->device_sas_token could not be copied; STRING_construct failed)");
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_012: [If provided, authentication_create() shall save a copy of `config->device_primary_key` into the `instance->device_primary_key`]
			else if (config->device_primary_key != NULL && (instance->device_primary_key = STRING_construct(config->device_primary_key)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_013: [If STRING_construct() fails to copy `config->device_primary_key`, authentication_create() shall fail and return NULL]
				result = NULL;
				LogError("authentication_create failed (config->device_primary_key could not be copied; STRING_construct failed)");
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_014: [If provided, authentication_create() shall save a copy of `config->device_secondary_key` into `instance->device_secondary_key`]
			else if (config->device_secondary_key != NULL && (instance->device_secondary_key = STRING_construct(config->device_secondary_key)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_015: [If STRING_construct() fails to copy `config->device_secondary_key`, authentication_create() shall fail and return NULL]
				result = NULL;
				LogError("authentication_create failed (config->device_secondary_key could not be copied; STRING_construct failed)");
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_016: [If provided, authentication_create() shall save a copy of `config->iothub_host_fqdn` into `instance->iothub_host_fqdn`]
			else if ((instance->iothub_host_fqdn = STRING_construct(config->iothub_host_fqdn)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_017: [If STRING_clone() fails to copy `config->iothub_host_fqdn`, authentication_create() shall fail and return NULL]
				result = NULL;
				LogError("authentication_create failed (config->iothub_host_fqdn could not be copied; STRING_construct failed)");
			}
			else
			{
				instance->state = AUTHENTICATION_STATE_STOPPED;

				if (instance->device_sas_token != NULL)
				{
					instance->current_credential_in_use = USER_PROVIDED_SAS_TOKEN;
				}
				else
				{
					instance->current_credential_in_use = DEVICE_PRIMARY_KEY;
				}

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_018: [authentication_create() shall save `config->on_state_changed_callback` and `config->on_state_changed_callback_context` into `instance->on_state_changed_callback` and `instance->on_state_changed_callback_context`.]
				instance->on_state_changed_callback = config->on_state_changed_callback;
				instance->on_state_changed_callback_context = config->on_state_changed_callback_context;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_019: [authentication_create() shall save `config->on_error_callback` and `config->on_error_callback_context` into `instance->on_error_callback` and `instance->on_error_callback_context`.]
				instance->on_error_callback = config->on_error_callback;
				instance->on_error_callback_context = config->on_error_callback_context;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_021: [authentication_create() shall set `instance->cbs_request_timeout_secs` with the default value of UINT32_MAX]
				instance->cbs_request_timeout_secs = DEFAULT_CBS_REQUEST_TIMEOUT_SECS;
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_022: [authentication_create() shall set `instance->sas_token_lifetime_secs` with the default value of one hour]
				instance->sas_token_lifetime_secs = DEFAULT_SAS_TOKEN_LIFETIME_SECS;
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_023: [authentication_create() shall set `instance->sas_token_refresh_time_secs` with the default value of 30 minutes]
				instance->sas_token_refresh_time_secs = DEFAULT_SAS_TOKEN_REFRESH_TIME_SECS;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_024: [If no failure occurs, authentication_create() shall return a reference to the AUTHENTICATION_INSTANCE handle]
				result = (AUTHENTICATION_HANDLE)instance;
			}

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_020: [If any failure occurs, authentication_create() shall free any memory it allocated previously]
			if (result == NULL)
			{
				authentication_destroy((AUTHENTICATION_HANDLE)instance);
			}
		}
	}
	
    return result;
}

void authentication_do_work(AUTHENTICATION_HANDLE authentication_handle)
{
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_036: [If authentication_handle is NULL, authentication_do_work() shall fail and return]
	if (authentication_handle == NULL)
	{
		LogError("authentication_do_work failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_038: [If `instance->is_cbs_put_token_in_progress` is TRUE, authentication_do_work() shall only verify the authentication timeout]
		if (instance->is_cbs_put_token_in_progress)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_084: [If no timeout has occurred, authentication_do_work() shall return]

			bool is_timed_out;
			if (verify_cbs_put_token_timeout(instance, &is_timed_out) == RESULT_OK && is_timed_out)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_085: [`instance->is_cbs_put_token_in_progress` shall be set to FALSE]
				instance->is_cbs_put_token_in_progress = false;
			
				if (mark_current_device_key_as_invalid(instance))
				{
					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_086: [`instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
					update_state(instance, AUTHENTICATION_STATE_ERROR);

					if (instance->is_sas_token_refresh_in_progress)
					{
						// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_087: [If `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT]
						notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT);
					}
					else
					{
						// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_088: [If `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_TIMEOUT]
						notify_error(instance, AUTHENTICATION_ERROR_AUTH_TIMEOUT);
					}
				}

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_089: [`instance->is_sas_token_refresh_in_progress` shall be set to FALSE]
				instance->is_sas_token_refresh_in_progress = false;
			}
		}
		else if (instance->state == AUTHENTICATION_STATE_STARTED)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_040: [If `instance->state` is AUTHENTICATION_STATE_STARTED and user-provided SAS token was used, authentication_do_work() shall return]
			if (are_device_keys_used_for_authentication(instance))
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_039: [If `instance->state` is AUTHENTICATION_STATE_STARTED and device keys were used, authentication_do_work() shall only verify the SAS token refresh time]
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_065: [The SAS token shall be refreshed if the current time minus `instance->current_sas_token_put_time` equals or exceeds `instance->sas_token_refresh_time_secs`]
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_066: [If SAS token does not need to be refreshed, authentication_do_work() shall return]
				bool is_timed_out;
				if (verify_sas_token_refresh_timeout(instance, &is_timed_out) == RESULT_OK && is_timed_out)
				{
					STRING_HANDLE device_key;

					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_119: [authentication_do_work() shall set `instance->is_sas_token_refresh_in_progress` to TRUE]
					instance->is_sas_token_refresh_in_progress = true;

					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_067: [authentication_do_work() shall create a SAS token using `instance->device_primary_key`, unless it has failed previously]
					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_069: [If using `instance->device_primary_key` has failed previously, a SAS token shall be created using `instance->device_secondary_key`]
					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_068: [If using `instance->device_primary_key` has failed previously and `instance->device_secondary_key` is not provided,  authentication_do_work() shall fail and return]
					while ((device_key = get_current_valid_device_key(instance)) != NULL)
					{
						if (create_and_put_SAS_token_to_cbs(instance, device_key) == RESULT_OK)
						{
							break;
						}
						else
						{
							LogError("Failed refreshing SAS token (%d)", instance->current_credential_in_use);
							(void)mark_current_device_key_as_invalid(instance);
						}
					}

					if (!instance->is_cbs_put_token_in_progress)
					{
						// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_120: [If cbs_put_token() fails, `instance->is_sas_token_refresh_in_progress` shall be set to FALSE]
						instance->is_sas_token_refresh_in_progress = false;

						// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_079: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
						update_state(instance, AUTHENTICATION_STATE_ERROR);

						// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_080: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED]
						notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED);
					}
				}
			}
		}
		else if (instance->state == AUTHENTICATION_STATE_STARTING)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_041: [If `instance->device_sas_token` is provided, authentication_do_work() shall put it to CBS]
			if (instance->device_sas_token != NULL)
			{
				STRING_HANDLE devices_path;
				
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_044: [A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id]
				if ((devices_path = create_devices_path(instance->iothub_host_fqdn, instance->device_id)) == NULL)
				{
					// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_045: [If `devices_path` failed to be created, authentication_do_work() shall set `instance->is_cbs_put_token_in_progress` to FALSE and return]
					LogError("Failed authenticating using SAS token (create_devices_path() failed)");
				}
				else if (put_SAS_token_to_cbs(instance, devices_path, instance->device_sas_token) != RESULT_OK)
				{
					LogError("Failed authenticating using SAS token (put_SAS_token_to_cbs() failed)");
				}

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_063: [authentication_do_work() shall free the memory it allocated for `devices_path`, `sasTokenKeyName` and SAS token]
				STRING_delete(devices_path);
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_042: [Otherwise, authentication_do_work() shall use device keys for CBS authentication]
			else
			{
				STRING_HANDLE device_key;

				while ((device_key = get_current_valid_device_key(instance)) != NULL)
				{
					if (create_and_put_SAS_token_to_cbs(instance, device_key) == RESULT_OK)
					{
						break;
					}
					else
					{
						LogError("Failed authenticating device '%s' using device keys (%d)", STRING_c_str(instance->device_id), instance->current_credential_in_use);
						(void)mark_current_device_key_as_invalid(instance);
					}
				}
			}

			if (!instance->is_cbs_put_token_in_progress)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_061: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_121: [If cbs_put_token() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked]
				update_state(instance, AUTHENTICATION_STATE_ERROR);

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_062: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_122: [If cbs_put_token() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED]
				notify_error(instance, AUTHENTICATION_ERROR_AUTH_FAILED);
			}
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_037: [If `instance->state` is not AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_do_work() shall fail and return]
			// Nothing to be done.
		}
	}
}

int authentication_set_cbs_request_timeout_secs(AUTHENTICATION_HANDLE authentication_handle, uint32_t value)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_097: [If `authentication_handle` is NULL, authentication_set_cbs_request_timeout_secs() shall fail and return __LINE__]
	if (authentication_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_set_cbs_request_timeout_secs failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_098: [`value` shall be saved on `instance->cbs_request_timeout_secs`]
		instance->cbs_request_timeout_secs = value;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_099: [authentication_set_cbs_request_timeout_secs() shall return 0]
		result = RESULT_OK;
	}

    return result;
}

int authentication_set_sas_token_refresh_time_secs(AUTHENTICATION_HANDLE authentication_handle, uint32_t value)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_100: [If `authentication_handle` is NULL, authentication_set_sas_token_refresh_time_secs() shall fail and return __LINE__]
	if (authentication_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_set_sas_token_refresh_time_secs failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_101: [`value` shall be saved on `instance->sas_token_refresh_time_secs`]
		instance->sas_token_refresh_time_secs = value;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_102: [authentication_set_sas_token_refresh_time_secs() shall return 0]
		result = RESULT_OK;
	}

	return result;
}

int authentication_set_sas_token_lifetime_secs(AUTHENTICATION_HANDLE authentication_handle, uint32_t value)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_103: [If `authentication_handle` is NULL, authentication_set_sas_token_lifetime_secs() shall fail and return __LINE__]
	if (authentication_handle == NULL)
	{
		result = __LINE__;
		LogError("authentication_set_sas_token_lifetime_secs failed (authentication_handle is NULL)");
	}
	else
	{
		AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_104: [`value` shall be saved on `instance->sas_token_lifetime_secs`]
		instance->sas_token_lifetime_secs = value;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_105: [authentication_set_sas_token_lifetime_secs() shall return 0]
		result = RESULT_OK;
	}

	return result;
}
