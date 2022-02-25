// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "internal/iothub_client_retry_control.h"

#include <math.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"

#define RESULT_OK                 0
#define INDEFINITE_TIME           ((time_t)-1)
#define DEFAULT_MAX_DELAY_IN_SECS 30

typedef struct RETRY_CONTROL_INSTANCE_TAG
{
    IOTHUB_CLIENT_RETRY_POLICY policy;
    unsigned int max_retry_time_in_secs;

    unsigned int initial_wait_time_in_secs;
    unsigned int max_jitter_percent;
    unsigned int max_delay_in_secs;

    unsigned int retry_count;
    time_t first_retry_time;
    time_t last_retry_time;
    unsigned int current_wait_time_in_secs;
} RETRY_CONTROL_INSTANCE;

typedef int (*RETRY_ACTION_EVALUATION_FUNCTION)(RETRY_CONTROL_INSTANCE* retry_state, RETRY_ACTION* retry_action);


// ========== Helper Functions ========== //

// ---------- Set/Retrieve Options Helpers ----------//

static void* retry_control_clone_option(const char* name, const void* value)
{
    void* result;

    if ((name == NULL) || (value == NULL))
    {
        LogError("Failed to clone option (either name (%p) or value (%p) are NULL)", name, value);
        result = NULL;
    }
    else if (strcmp(RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, name) == 0 ||
            strcmp(RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, name) == 0)
    {
        unsigned int* cloned_value;

        if ((cloned_value = (unsigned int*)malloc(sizeof(unsigned int))) == NULL)
        {
            LogError("Failed to clone option '%p' (malloc failed)", name);
            result = NULL;
        }
        else
        {
            *cloned_value = *(unsigned int*)value;

            result = (void*)cloned_value;
        }
    }
    else
    {
        LogError("Failed to clone option (option with name '%s' is not suppported)", name);
        result = NULL;
    }

    return result;
}

static void retry_control_destroy_option(const char* name, const void* value)
{
    if ((name == NULL) || (value == NULL))
    {
        LogError("Failed to destroy option (either name (%p) or value (%p) are NULL)", name, value);
    }
    else if (strcmp(RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, name) == 0 ||
        strcmp(RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, name) == 0)
    {
        free((void*)value);
    }
    else
    {
        LogError("Failed to destroy option (option with name '%s' is not suppported)", name);
    }
}

// ========== _should_retry() Auxiliary Functions ========== //

static int evaluate_retry_action(RETRY_CONTROL_INSTANCE* retry_control, RETRY_ACTION* retry_action)
{
    int result;

    if (retry_control->retry_count == 0)
    {
        *retry_action = RETRY_ACTION_RETRY_NOW;
        result = RESULT_OK;
    }
    else if (retry_control->last_retry_time == INDEFINITE_TIME &&
             retry_control->policy != IOTHUB_CLIENT_RETRY_IMMEDIATE)
    {
        LogError("Failed to evaluate retry action (last_retry_time is INDEFINITE_TIME)");
        result = MU_FAILURE;
    }
    else
    {
        time_t current_time;

        if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed to evaluate retry action (get_time() failed)");
            result = MU_FAILURE;
        }
        else if (retry_control->max_retry_time_in_secs > 0 &&
            get_difftime(current_time, retry_control->first_retry_time) >= retry_control->max_retry_time_in_secs)
        {
            *retry_action = RETRY_ACTION_STOP_RETRYING;

            result = RESULT_OK;
        }
        else if (retry_control->policy == IOTHUB_CLIENT_RETRY_IMMEDIATE)
        {
            *retry_action = RETRY_ACTION_RETRY_NOW;

            result = RESULT_OK;
        }
        else if (get_difftime(current_time, retry_control->last_retry_time) < retry_control->current_wait_time_in_secs)
        {
            *retry_action = RETRY_ACTION_RETRY_LATER;

            result = RESULT_OK;
        }
        else
        {
            *retry_action = RETRY_ACTION_RETRY_NOW;

            result = RESULT_OK;
        }
    }

    return result;
}

static unsigned int calculate_next_wait_time(RETRY_CONTROL_INSTANCE* retry_control)
{
    unsigned int result;

    if (retry_control->policy == IOTHUB_CLIENT_RETRY_INTERVAL)
    {
        result = retry_control->initial_wait_time_in_secs;
    }
    else if (retry_control->policy == IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF)
    {
        unsigned int base_delay = retry_control->initial_wait_time_in_secs * (retry_control->retry_count);

        if (base_delay > retry_control->max_delay_in_secs) 
        {
            base_delay = retry_control->max_delay_in_secs;
        }        
        
        result = base_delay;
    }
    else if (retry_control->policy == IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF)
    {
        double base_delay = pow(2, retry_control->retry_count - 1) * retry_control->initial_wait_time_in_secs;

        if (base_delay > retry_control->max_delay_in_secs) 
        {
            base_delay = retry_control->max_delay_in_secs;
        }

        result = (unsigned int)base_delay;
    }
    else if (retry_control->policy == IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER)
    {
        double jitter_percent = (retry_control->max_jitter_percent / 100.0) * (rand() / ((double)RAND_MAX));

        double base_delay = pow(2, retry_control->retry_count - 1) * retry_control->initial_wait_time_in_secs;

        if (base_delay > retry_control->max_delay_in_secs) 
        {
            base_delay = retry_control->max_delay_in_secs;
        }

        result =  (unsigned int)(base_delay * (1 + jitter_percent));
    }
    else if (retry_control->policy == IOTHUB_CLIENT_RETRY_RANDOM)
    {
        double random_percent = ((double)rand() / (double)RAND_MAX);
        result = (unsigned int)(retry_control->initial_wait_time_in_secs * random_percent);
    }
    else
    {
        LogError("Failed to calculate the next wait time (policy %d is not expected)", retry_control->policy);

        result = 0;
    }

    return result;
}


// ========== Public API ========== //

int is_timeout_reached(time_t start_time, unsigned int timeout_in_secs, bool* is_timed_out)
{
    int result;

    if (start_time == INDEFINITE_TIME)
    {
        LogError("Failed to verify timeout (start_time is INDEFINITE)");
        result = MU_FAILURE;
    }
    else if (is_timed_out == NULL)
    {
        LogError("Failed to verify timeout (is_timed_out is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        time_t current_time;

        if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed to verify timeout (get_time failed)");
            result = MU_FAILURE;
        }
        else
        {
            if (get_difftime(current_time, start_time) >= timeout_in_secs)
            {
                *is_timed_out = true;
            }
            else
            {
                *is_timed_out = false;
            }

            result = RESULT_OK;
        }
    }

    return result;
}

void retry_control_reset(RETRY_CONTROL_HANDLE retry_control_handle)
{
    if (retry_control_handle == NULL)
    {
        LogError("Failed to reset the retry control (retry_state_handle is NULL)");
    }
    else
    {
        RETRY_CONTROL_INSTANCE* retry_control = (RETRY_CONTROL_INSTANCE*)retry_control_handle;

        retry_control->retry_count = 0;
        retry_control->current_wait_time_in_secs = 0;
        retry_control->first_retry_time = INDEFINITE_TIME;
        retry_control->last_retry_time = INDEFINITE_TIME;
    }
}

RETRY_CONTROL_HANDLE retry_control_create(IOTHUB_CLIENT_RETRY_POLICY policy, unsigned int max_retry_time_in_secs)
{
    RETRY_CONTROL_INSTANCE* retry_control;

    if ((retry_control = (RETRY_CONTROL_INSTANCE*)malloc(sizeof(RETRY_CONTROL_INSTANCE))) == NULL)
    {
        LogError("Failed creating the retry control (malloc failed)");
    }
    else
    {
        memset(retry_control, 0, sizeof(RETRY_CONTROL_INSTANCE));
        retry_control->policy = policy;
        retry_control->max_retry_time_in_secs = max_retry_time_in_secs;

        if (retry_control->policy == IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF ||
            retry_control->policy == IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER)
        {
            retry_control->initial_wait_time_in_secs = 1;
        }
        else
        {
            retry_control->initial_wait_time_in_secs = 5;
        }

        retry_control->max_jitter_percent = 5;
        retry_control->max_delay_in_secs = DEFAULT_MAX_DELAY_IN_SECS;

        retry_control_reset(retry_control);
    }

    return (RETRY_CONTROL_HANDLE)retry_control;
}

void retry_control_destroy(RETRY_CONTROL_HANDLE retry_control_handle)
{
    if (retry_control_handle == NULL)
    {
        LogError("Failed to destroy the retry control (retry_control_handle is NULL)");
    }
    else
    {
        free(retry_control_handle);
    }
}

int retry_control_should_retry(RETRY_CONTROL_HANDLE retry_control_handle, RETRY_ACTION* retry_action)
{
    int result;

    if ((retry_control_handle == NULL) || (retry_action == NULL))
    {
        LogError("Failed to evaluate if retry should be attempted (either retry_control_handle (%p) or retry_action (%p) are NULL)", retry_control_handle, retry_action);
        result = MU_FAILURE;
    }
    else
    {
        RETRY_CONTROL_INSTANCE* retry_control = (RETRY_CONTROL_INSTANCE*)retry_control_handle;

        if (retry_control->policy == IOTHUB_CLIENT_RETRY_NONE)
        {
            *retry_action = RETRY_ACTION_STOP_RETRYING;
            result = RESULT_OK;
        }
        else if (retry_control->first_retry_time == INDEFINITE_TIME && (retry_control->first_retry_time = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed to evaluate if retry should be attempted (get_time() failed)");
            result = MU_FAILURE;
        }
        else if (evaluate_retry_action(retry_control, retry_action) != RESULT_OK)
        {
            LogError("Failed to evaluate if retry should be attempted (evaluate_retry_action() failed)");
            result = MU_FAILURE;
        }
        else
        {
            if (*retry_action == RETRY_ACTION_RETRY_NOW)
            {
                retry_control->retry_count++;

                if (retry_control->policy != IOTHUB_CLIENT_RETRY_IMMEDIATE)
                {
                    retry_control->last_retry_time = get_time(NULL);

                    retry_control->current_wait_time_in_secs = calculate_next_wait_time(retry_control);
                }
            }

            result = RESULT_OK;
        }
    }

    return result;
}

int retry_control_set_option(RETRY_CONTROL_HANDLE retry_control_handle, const char* name, const void* value)
{
    int result;

    if (retry_control_handle == NULL || name == NULL || value == NULL)
    {
        LogError("Failed to set option (either retry_state_handle (%p), name (%p) or value (%p) are NULL)", retry_control_handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        RETRY_CONTROL_INSTANCE* retry_control = (RETRY_CONTROL_INSTANCE*)retry_control_handle;

        if (strcmp(RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, name) == 0)
        {
            unsigned int cast_value = *((unsigned int*)value);

            if (cast_value < 1)
            {
                LogError("Failed to set option '%s' (value must be equal or greater to 1)", name);
                result = MU_FAILURE;
            }
            else
            {
                retry_control->initial_wait_time_in_secs = cast_value;

                result = RESULT_OK;
            }
        }
        else if (strcmp(RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, name) == 0)
        {
            unsigned int cast_value = *((unsigned int*)value);

            if (cast_value > 100) // it's unsigned int, it doesn't need to be checked for less than zero.
            {
                LogError("Failed to set option '%s' (value must be in the range 0 to 100)", name);
                result = MU_FAILURE;
            }
            else
            {
                retry_control->max_jitter_percent = cast_value;

                result = RESULT_OK;
            }
        }
        else if (strcmp(RETRY_CONTROL_OPTION_MAX_DELAY_IN_SECS, name) == 0)
        {
            retry_control->max_delay_in_secs = *((unsigned int*)value);

            result = RESULT_OK;
        }
        else if (strcmp(RETRY_CONTROL_OPTION_SAVED_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, retry_control_handle) != OPTIONHANDLER_OK)
            {
                LogError("messenger_set_option failed (OptionHandler_FeedOptions failed)");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            LogError("messenger_set_option failed (option with name '%s' is not suppported)", name);
            result = MU_FAILURE;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE retry_control_retrieve_options(RETRY_CONTROL_HANDLE retry_control_handle)
{
    OPTIONHANDLER_HANDLE result;

    if (retry_control_handle == NULL)
    {
        LogError("Failed to retrieve options (retry_state_handle is NULL)");
        result = NULL;
    }
    else
    {
        OPTIONHANDLER_HANDLE options = OptionHandler_Create(retry_control_clone_option, retry_control_destroy_option, (pfSetOption)retry_control_set_option);

        if (options == NULL)
        {
            LogError("Failed to retrieve options (OptionHandler_Create failed)");
            result = NULL;
        }
        else
        {
            RETRY_CONTROL_INSTANCE* retry_control = (RETRY_CONTROL_INSTANCE*)retry_control_handle;

            if (OptionHandler_AddOption(options, RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS, (void*)&retry_control->initial_wait_time_in_secs) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options (OptionHandler_Create failed for option '%s')", RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS);
                result = NULL;
            }
            else if (OptionHandler_AddOption(options, RETRY_CONTROL_OPTION_MAX_JITTER_PERCENT, (void*)&retry_control->max_jitter_percent) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options (OptionHandler_Create failed for option '%s')", RETRY_CONTROL_OPTION_INITIAL_WAIT_TIME_IN_SECS);
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
