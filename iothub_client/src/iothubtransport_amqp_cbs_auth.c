// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "internal/iothubtransport_amqp_cbs_auth.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_uamqp_c/async_operation.h"

#define RESULT_OK                                 0
#define INDEFINITE_TIME                           ((time_t)(-1))
#define SAS_TOKEN_TYPE                            "servicebus.windows.net:sastoken"
#define IOTHUB_DEVICES_PATH_FMT                   "%s/devices/%s"
#define IOTHUB_DEVICES_MODULE_PATH_FMT            "%s/devices/%s/modules/%s"
#define DEFAULT_CBS_REQUEST_TIMEOUT_SECS          UINT32_MAX
#define SAS_REFRESH_MULTIPLIER                    .8

typedef struct AUTHENTICATION_INSTANCE_TAG
{
    const char* device_id;
    const char* module_id;
    STRING_HANDLE iothub_host_fqdn;

    ON_AUTHENTICATION_STATE_CHANGED_CALLBACK on_state_changed_callback;
    void* on_state_changed_callback_context;

    ON_AUTHENTICATION_ERROR_CALLBACK on_error_callback;
    void* on_error_callback_context;

    uint64_t cbs_request_timeout_secs;

    AUTHENTICATION_STATE state;
    CBS_HANDLE cbs_handle;

    bool is_cbs_put_token_in_progress;
    bool is_sas_token_refresh_in_progress;

    time_t current_sas_token_put_time;

    // Auth module used to generating handle authorization
    // with either SAS Token, x509 Certs, and Device SAS Token
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
    ASYNC_OPERATION_HANDLE cbs_put_token_async_context;
} AUTHENTICATION_INSTANCE;


// Helper functions:

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
        result = MU_FAILURE;
        LogError("Failed verifying if cbs_put_token has timed out (current_sas_token_put_time is not set)");
    }
    else
    {
        time_t current_time;

        if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            result = MU_FAILURE;
            LogError("Failed verifying if cbs_put_token has timed out (get_time failed)");
        }
        else if ((uint64_t)get_difftime(current_time, instance->current_sas_token_put_time) >= instance->cbs_request_timeout_secs)
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
    uint64_t sas_token_expiry;

    if (instance->current_sas_token_put_time == INDEFINITE_TIME)
    {
        result = MU_FAILURE;
        LogError("Failed verifying if SAS token refresh timed out (current_sas_token_put_time is not set)");
    }
    else if ((sas_token_expiry = IoTHubClient_Auth_Get_SasToken_Expiry(instance->authorization_module)) == 0)
    {
        result = MU_FAILURE;
        LogError("Failed Getting SasToken Expiry");
    }
    else
    {
        time_t current_time;
        if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            result = MU_FAILURE;
            LogError("Failed verifying if SAS token refresh timed out (get_time failed)");
        }
        else if ((uint64_t)get_difftime(current_time, instance->current_sas_token_put_time) >= (sas_token_expiry*SAS_REFRESH_MULTIPLIER))
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

static STRING_HANDLE create_device_and_module_path(STRING_HANDLE iothub_host_fqdn, const char* device_id, const char* module_id)
{
    STRING_HANDLE devices_and_modules_path;

    if (module_id == NULL)
    {
        if ((devices_and_modules_path = STRING_construct_sprintf(IOTHUB_DEVICES_PATH_FMT, STRING_c_str(iothub_host_fqdn), device_id)) == NULL)
        {
            LogError("Failed creating devices_and_modules_path (STRING_new failed)");
        }
    }
    else
    {
        if ((devices_and_modules_path = STRING_construct_sprintf(IOTHUB_DEVICES_MODULE_PATH_FMT, STRING_c_str(iothub_host_fqdn), device_id, module_id)) == NULL)
        {
            LogError("Failed creating devices_and_modules_path (STRING_new failed)");
        }
    }
    return devices_and_modules_path;
}

static void on_cbs_put_token_complete_callback(void* context, CBS_OPERATION_RESULT operation_result, unsigned int status_code, const char* status_description)
{
#ifdef NO_LOGGING
    UNUSED(status_code);
    UNUSED(status_description);
#endif
    AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)context;

    instance->cbs_put_token_async_context = NULL;

    instance->is_cbs_put_token_in_progress = false;

    if (operation_result == CBS_OPERATION_RESULT_OK)
    {
        update_state(instance, AUTHENTICATION_STATE_STARTED);
    }
    else
    {
        LogError("CBS reported status code %u, error: '%s' for put-token operation for device '%s'", status_code, status_description, instance->device_id);

        update_state(instance, AUTHENTICATION_STATE_ERROR);

        if (instance->is_sas_token_refresh_in_progress)
        {
            notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED);
        }
        else
        {
            notify_error(instance, AUTHENTICATION_ERROR_AUTH_FAILED);
        }
    }

    instance->is_sas_token_refresh_in_progress = false;
}

static int put_SAS_token_to_cbs(AUTHENTICATION_INSTANCE* instance, STRING_HANDLE cbs_audience, const char* sas_token)
{
    int result;

    if (instance == NULL)
    {
        result = MU_FAILURE;
        LogError("Invalid AUTHENTICATION_INSTANCE");
    }
    else
    {
        instance->is_cbs_put_token_in_progress = true;

        const char* cbs_audience_c_str = STRING_c_str(cbs_audience);

        instance->cbs_put_token_async_context = cbs_put_token_async(instance->cbs_handle, SAS_TOKEN_TYPE, cbs_audience_c_str, sas_token, on_cbs_put_token_complete_callback, instance);
        if (instance->cbs_put_token_async_context == NULL)
        {
            instance->is_cbs_put_token_in_progress = false;
            result = MU_FAILURE;
            LogError("Failed putting SAS token to CBS for device '%s' (cbs_put_token failed)", instance->device_id);
        }
        else
        {
            time_t current_time;

            if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
            {
                LogError("Failed setting current_sas_token_put_time for device '%s' (get_time() failed)", instance->device_id);
            }

            instance->current_sas_token_put_time = current_time; // If it failed, fear not. `current_sas_token_put_time` shall be checked for INDEFINITE_TIME wherever it is used.

            result = RESULT_OK;
        }
    }

    return result;
}

static int create_and_put_SAS_token_to_cbs(AUTHENTICATION_INSTANCE* instance)
{
    int result;
    char* sas_token;
    STRING_HANDLE device_and_module_path;

    if ((device_and_module_path = create_device_and_module_path(instance->iothub_host_fqdn, instance->device_id, instance->module_id)) == NULL)
    {
        result = MU_FAILURE;
        sas_token = NULL;
        LogError("Failed creating a SAS token (create_device_and_module_path() failed)");
    }
    else
    {
        IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(instance->authorization_module);
        if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY || cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
        {
            sas_token = IoTHubClient_Auth_Get_SasToken(instance->authorization_module, STRING_c_str(device_and_module_path), 0, NULL);
            if (sas_token == NULL)
            {
                LogError("failure getting sas token.");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
        {
            SAS_TOKEN_STATUS token_status = IoTHubClient_Auth_Is_SasToken_Valid(instance->authorization_module);
            if (token_status == SAS_TOKEN_STATUS_INVALID)
            {
                LogError("sas token is invalid.");
                sas_token = NULL;
                result = MU_FAILURE;
            }
            else if (token_status == SAS_TOKEN_STATUS_FAILED)
            {
                LogError("testing Sas Token failed.");
                sas_token = NULL;
                result = MU_FAILURE;
            }
            else
            {
                sas_token = IoTHubClient_Auth_Get_SasToken(instance->authorization_module, NULL, 0, NULL);
                if (sas_token == NULL)
                {
                    LogError("failure getting sas Token.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = RESULT_OK;
                }
            }
        }
        else if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
        {
            sas_token = NULL;
            result = RESULT_OK;
        }
        else
        {
            LogError("failure unknown credential type found.");
            sas_token = NULL;
            result = MU_FAILURE;
        }


        if (sas_token != NULL)
        {
            if (put_SAS_token_to_cbs(instance, device_and_module_path, sas_token) != RESULT_OK)
            {
                result = MU_FAILURE;
                LogError("Failed putting SAS token to CBS");
            }
            else
            {
                result = RESULT_OK;
            }
            free(sas_token);
        }

        STRING_delete(device_and_module_path);
    }
    return result;
}

// ---------- Set/Retrieve Options Helpers ----------//
static void* authentication_clone_option(const char* name, const void* value)
{
    void* result;

    if (name == NULL)
    {
        LogError("Failed to clone authentication option (name is NULL)");
        result = NULL;
    }
    else if (value == NULL)
    {
        LogError("Failed to clone authentication option (value is NULL)");
        result = NULL;
    }
    else
    {
        if (strcmp(AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, name) == 0 ||
            strcmp(AUTHENTICATION_OPTION_SAVED_OPTIONS, name) == 0)
        {
            result = (void*)value;
        }
        else
        {
            LogError("Failed to clone authentication option (option with name '%s' is not suppported)", name);
            result = NULL;
        }
    }

    return result;
}

static void authentication_destroy_option(const char* name, const void* value)
{
    if (name == NULL)
    {
        LogError("Failed to destroy authentication option (name is NULL)");
    }
    else if (value == NULL)
    {
        LogError("Failed to destroy authentication option (value is NULL)");
    }
    else
    {
        if (strcmp(name, AUTHENTICATION_OPTION_SAVED_OPTIONS) == 0)
        {
            OptionHandler_Destroy((OPTIONHANDLER_HANDLE)value);
        }
    }
}

// Public APIs:
int authentication_start(AUTHENTICATION_HANDLE authentication_handle, const CBS_HANDLE cbs_handle)
{
    int result;

    if (authentication_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("authentication_start failed (authentication_handle is NULL)");
    }
    else if (cbs_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("authentication_start failed (cbs_handle is NULL)");
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

        if (instance->state != AUTHENTICATION_STATE_STOPPED)
        {
            result = MU_FAILURE;
            LogError("authentication_start failed (messenger has already been started; current state: %d)", instance->state);
        }
        else
        {
            instance->cbs_handle = cbs_handle;

            update_state(instance, AUTHENTICATION_STATE_STARTING);

            result = RESULT_OK;
        }
    }

    return result;
}

int authentication_stop(AUTHENTICATION_HANDLE authentication_handle)
{
    int result;

    if (authentication_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("authentication_stop failed (authentication_handle is NULL)");
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

        if (instance->state == AUTHENTICATION_STATE_STOPPED)
        {
            result = MU_FAILURE;
            LogError("authentication_stop failed (messenger is already stopped)");
        }
        else
        {
            instance->cbs_handle = NULL;

            update_state(instance, AUTHENTICATION_STATE_STOPPED);

            result = RESULT_OK;
        }
    }

    return result;
}

void authentication_destroy(AUTHENTICATION_HANDLE authentication_handle)
{
    if (authentication_handle == NULL)
    {
        LogError("authentication_destroy failed (authentication_handle is NULL)");
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

        if (instance->state != AUTHENTICATION_STATE_STOPPED)
        {
            (void)authentication_stop(authentication_handle);
        }
        if (instance->iothub_host_fqdn != NULL)
            STRING_delete(instance->iothub_host_fqdn);

        if (instance->cbs_put_token_async_context != NULL)
        {
            async_operation_cancel(instance->cbs_put_token_async_context);
        }

        free(instance);
    }
}

AUTHENTICATION_HANDLE authentication_create(const AUTHENTICATION_CONFIG* config)
{
    AUTHENTICATION_HANDLE result;

    if (config == NULL)
    {
        result = NULL;
        LogError("authentication_create failed (config is NULL)");
    }
    else if (config->authorization_module == NULL)
    {
        result = NULL;
        LogError("authentication_create failed (config->authorization_module is NULL)");
    }
    else if (config->iothub_host_fqdn == NULL)
    {
        result = NULL;
        LogError("authentication_create failed (config->iothub_host_fqdn is NULL)");
    }
    else if (config->on_state_changed_callback == NULL)
    {
        result = NULL;
        LogError("authentication_create failed (config->on_state_changed_callback is NULL)");
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance;

        if ((instance = (AUTHENTICATION_INSTANCE*)malloc(sizeof(AUTHENTICATION_INSTANCE))) == NULL)
        {
            result = NULL;
            LogError("authentication_create failed (malloc failed)");
        }
        else
        {
            memset(instance, 0, sizeof(AUTHENTICATION_INSTANCE));

            if ((instance->device_id = IoTHubClient_Auth_Get_DeviceId(config->authorization_module) ) == NULL)
            {
                result = NULL;
                LogError("authentication_create failed (config->device_id could not be copied; STRING_construct failed)");
            }
            else if ((instance->iothub_host_fqdn = STRING_construct(config->iothub_host_fqdn)) == NULL)
            {
                result = NULL;
                LogError("authentication_create failed (config->iothub_host_fqdn could not be copied; STRING_construct failed)");
            }
            else
            {
                instance->state = AUTHENTICATION_STATE_STOPPED;

                instance->module_id = IoTHubClient_Auth_Get_ModuleId(config->authorization_module);

                instance->on_state_changed_callback = config->on_state_changed_callback;
                instance->on_state_changed_callback_context = config->on_state_changed_callback_context;

                instance->on_error_callback = config->on_error_callback;
                instance->on_error_callback_context = config->on_error_callback_context;

                instance->cbs_request_timeout_secs = DEFAULT_CBS_REQUEST_TIMEOUT_SECS;

                instance->authorization_module = config->authorization_module;

                result = (AUTHENTICATION_HANDLE)instance;
            }

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
    if (authentication_handle == NULL)
    {
        LogError("authentication_do_work failed (authentication_handle is NULL)");
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

        if (instance->is_cbs_put_token_in_progress)
        {

            bool is_timed_out;
            if (verify_cbs_put_token_timeout(instance, &is_timed_out) == RESULT_OK && is_timed_out)
            {
                instance->is_cbs_put_token_in_progress = false;

                update_state(instance, AUTHENTICATION_STATE_ERROR);

                if (instance->is_sas_token_refresh_in_progress)
                {
                    notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT);
                }
                else
                {
                    notify_error(instance, AUTHENTICATION_ERROR_AUTH_TIMEOUT);
                }

                instance->is_sas_token_refresh_in_progress = false;
            }
        }
        else if (instance->state == AUTHENTICATION_STATE_STARTED)
        {
            IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(instance->authorization_module);
            if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY || cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
            {
                bool is_timed_out;
                if (verify_sas_token_refresh_timeout(instance, &is_timed_out) == RESULT_OK && is_timed_out)
                {
                    instance->is_sas_token_refresh_in_progress = true;

                    if (create_and_put_SAS_token_to_cbs(instance) != RESULT_OK)
                    {
                        LogError("Failed refreshing SAS token '%s'", instance->device_id);
                    }

                    if (!instance->is_cbs_put_token_in_progress)
                    {
                        instance->is_sas_token_refresh_in_progress = false;

                        update_state(instance, AUTHENTICATION_STATE_ERROR);

                        notify_error(instance, AUTHENTICATION_ERROR_SAS_REFRESH_FAILED);
                    }
                }
            }
        }
        else if (instance->state == AUTHENTICATION_STATE_STARTING)
        {
            if (create_and_put_SAS_token_to_cbs(instance) != RESULT_OK)
            {
                LogError("Failed authenticating device '%s' using device keys", instance->device_id);
            }

            if (!instance->is_cbs_put_token_in_progress)
            {
                update_state(instance, AUTHENTICATION_STATE_ERROR);

                notify_error(instance, AUTHENTICATION_ERROR_AUTH_FAILED);
            }
        }
        else
        {
            // Nothing to be done.
        }
    }
}

int authentication_set_option(AUTHENTICATION_HANDLE authentication_handle, const char* name, void* value)
{
    int result;

    if (authentication_handle == NULL || name == NULL || value == NULL)
    {
        LogError("authentication_set_option failed (one of the followin are NULL: authentication_handle=%p, name=%p, value=%p)",
            authentication_handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

        if (strcmp(AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, name) == 0)
        {
            instance->cbs_request_timeout_secs = *((size_t*)value);
            result = RESULT_OK;
        }
        else if (strcmp(AUTHENTICATION_OPTION_SAVED_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, authentication_handle) != OPTIONHANDLER_OK)
            {
                LogError("authentication_set_option failed (OptionHandler_FeedOptions failed)");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            LogError("authentication_set_option failed (option with name '%s' is not suppported)", name);
            result = MU_FAILURE;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE authentication_retrieve_options(AUTHENTICATION_HANDLE authentication_handle)
{
    OPTIONHANDLER_HANDLE result;

    if (authentication_handle == NULL)
    {
        LogError("Failed to retrieve options from authentication instance (authentication_handle is NULL)");
        result = NULL;
    }
    else
    {
        OPTIONHANDLER_HANDLE options = OptionHandler_Create(authentication_clone_option, authentication_destroy_option, (pfSetOption)authentication_set_option);

        if (options == NULL)
        {
            LogError("Failed to retrieve options from authentication instance (OptionHandler_Create failed)");
            result = NULL;
        }
        else
        {
            AUTHENTICATION_INSTANCE* instance = (AUTHENTICATION_INSTANCE*)authentication_handle;

            if (OptionHandler_AddOption(options, AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, (void*)&instance->cbs_request_timeout_secs) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options from authentication instance (OptionHandler_Create failed for option '%s')", AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS);
                result = NULL;
            }
            else
            {
                result = options;
            }

            if (result == NULL)
            {
                OptionHandler_Destroy(options);
            }
        }
    }
    return result;
}
