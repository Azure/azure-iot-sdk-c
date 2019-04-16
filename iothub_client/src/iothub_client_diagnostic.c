// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/buffer_.h"

#include "parson.h"
#include "internal/iothub_client_diagnostic.h"

#define DTRACING_NAMESPACE "azureiot*com^dtracing^1"
#define DTRACING_JSON_DOT_SEPARATOR "."
#define DTRACING_SAMPLING_MODE "sampling_mode"
#define DTRACING_SAMPLING_RATE "sampling_rate"

static const char* DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY = DTRACING_NAMESPACE DTRACING_JSON_DOT_SEPARATOR DTRACING_SAMPLING_MODE;
static const char* DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY = DTRACING_NAMESPACE DTRACING_JSON_DOT_SEPARATOR DTRACING_SAMPLING_RATE;

static const char* MESSAGE_DISTRIBUTED_TRACING_TIMESTAMP_KEY = "timestamp=";
static const char* DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION = "401";
static const char* DEVICE_TWIN_REPORTED_STATUS_SUCCESS = "204";

static const char* DISTRIBUTED_TRACING_REPORTED_TWIN_TEMPLATE = "{ \"__iot:interfaces\": \
{ \"" DTRACING_NAMESPACE "\": { \"@id\": \"http://azureiot.com/dtracing/1.0.0\" } }, \
  \"" DTRACING_NAMESPACE "\": { \
    \"" DTRACING_SAMPLING_MODE "\": { \
        \"value\": %d, \
        \"status\" : { \
        \"code\": %s, \
        \"description\" : \"%s\" \
    } \
}, \
    \"" DTRACING_SAMPLING_RATE "\": { \
        \"value\": %d, \
        \"status\" : { \
        \"code\": %s, \
        \"description\" : \"%s\" \
        } \
    } \
} \
}";

#define TIME_STRING_BUFFER_LEN 30

static const int BASE_36 = 36;

#define INDEFINITE_TIME ((time_t)-1)

static char* get_epoch_time(char* timeBuffer)
{
    char* result;
    time_t epochTime;
    int timeLen = sizeof(time_t);

    if ((epochTime = get_time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failed getting current time");
        result = NULL;
    }
    else if (timeLen == sizeof(int64_t))
    {
        if (sprintf(timeBuffer, "%"PRIu64, (int64_t)epochTime) < 0)
        {
            LogError("Failed sprintf to timeBuffer with 8 bytes of time_t");
            result = NULL;
        }
        else
        {
            result = timeBuffer;
        }
    }
    else if (timeLen == sizeof(int32_t))
    {
        if (sprintf(timeBuffer, "%"PRIu32, (int32_t)epochTime) < 0)
        {
            LogError("Failed sprintf to timeBuffer with 4 bytes of time_t");
            result = NULL;
        }
        else
        {
            result = timeBuffer;
        }
    }
    else
    {
        LogError("Unknown size of time_t");
        result = NULL;
    }

    return result;
}

static char get_base36_char(unsigned char value)
{
    return value <= 9 ? '0' + value : 'a' + value - 10;
}

static char* generate_eight_random_characters(char *randomString)
{
    int i;
    char* randomStringPos = randomString;
    for (i = 0; i < 4; ++i)
    {
        int rawRandom = rand();
        int first = rawRandom % BASE_36;
        int second = rawRandom / BASE_36 % BASE_36;
        *randomStringPos++ = get_base36_char((unsigned char)first);
        *randomStringPos++ = get_base36_char((unsigned char)second);
    }
    *randomStringPos = 0;

    return randomString;
}

static bool should_add_diagnostic_info(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting)
{
    bool result = false;
    if (diagSetting->diagSamplingPercentage > 0)
    {
        double number;
        double percentage;

        if (diagSetting->currentMessageNumber == UINT32_MAX)
        {
            diagSetting->currentMessageNumber %= diagSetting->diagSamplingPercentage * 100;
        }
        ++diagSetting->currentMessageNumber;

        number = diagSetting->currentMessageNumber;
        percentage = diagSetting->diagSamplingPercentage;
        result = (floor((number - 2) * percentage / 100.0) < floor((number - 1) * percentage / 100.0));
    }
    return result;
}

static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* prepare_message_diagnostic_data()
{
    IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* result = (IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA*)malloc(sizeof(IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA));
    if (result == NULL)
    {
        LogError("malloc for DiagnosticData failed");
    }
    else
    {
        char* diagId = (char*)malloc(9);
        if (diagId == NULL)
        {
            LogError("malloc for diagId failed");
            free(result);
            result = NULL;
        }
        else
        {
            char* timeBuffer;

            (void)generate_eight_random_characters(diagId);
            result->diagnosticId = diagId;

            timeBuffer = (char*)malloc(TIME_STRING_BUFFER_LEN);
            if (timeBuffer == NULL)
            {
                LogError("malloc for timeBuffer failed");
                free(result->diagnosticId);
                free(result);
                result = NULL;
            }
            else if (get_epoch_time(timeBuffer) == NULL)
            {
                LogError("Failed getting current time");
                free(result->diagnosticId);
                free(result);
                free(timeBuffer);
                result = NULL;
            }
            else
            {
                result->diagnosticCreationTimeUtc = timeBuffer;
            }
        }
    }
    return result;
}

// **Deprecated - should use IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary instead 
int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle)
{
    int result;
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
    if (diagSetting == NULL || messageHandle == NULL)
    {
        result = MU_FAILURE;
    }
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_003: [ If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added ]*/
    else if (should_add_diagnostic_info(diagSetting))
    {
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/

        IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData;
        if ((diagnosticData = prepare_message_diagnostic_data()) == NULL)
        {
            result = MU_FAILURE;
        }
        else
        {
            if (IoTHubMessage_SetDiagnosticPropertyData(messageHandle, diagnosticData) != IOTHUB_MESSAGE_OK)
            {
                /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property. ]*/
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }

            free(diagnosticData->diagnosticCreationTimeUtc);
            free(diagnosticData->diagnosticId);
            free(diagnosticData);
            diagnosticData = NULL;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

// Distributed tracing uses fixed-rate sampling, not true random sampling.
static bool should_add_distributedtrace_fixed_rate_sampling(IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA* distributedTraceSetting)
{
    bool result = false;
    
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_003: [ If samplingMode is disabled or samplingRate is equal to 0, message number should not be increased and no distributed_tracing properties added and return false ]*/
    if (distributedTraceSetting->samplingMode != IOTHUB_DISTRIBUTED_TRACING_SAMPLING_MODE_OFF && distributedTraceSetting->samplingRate > 0)
    {
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_004: [ If diagSamplingPercentage is equal to 100, distributed_tracing properties should be added to all messages]*/
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_005: [ If diagSamplingPercentage is between(0, 100), distributed_tracing properties should be added based on percentage]*/
        double number;
        double percentage;

        if (distributedTraceSetting->currentMessageNumber == UINT32_MAX)
        {
            distributedTraceSetting->currentMessageNumber %= distributedTraceSetting->samplingRate * 100;
        }
        ++distributedTraceSetting->currentMessageNumber;

        number = distributedTraceSetting->currentMessageNumber;
        percentage = distributedTraceSetting->samplingRate;
        result = (floor((number - 2) * percentage / 100.0) < floor((number - 1) * percentage / 100.0));
    }
    return result;
}

int IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary(IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA* distributedTraceSetting, IOTHUB_MESSAGE_HANDLE messageHandle)
{
    int result;
    STRING_HANDLE messagePropertyContent = NULL;
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_001: [ IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary should return nonezero if distributedTraceSetting or messageHandle is NULL. ]*/
    if (distributedTraceSetting == NULL || messageHandle == NULL)
    {
        LogError("Invalid parameter for Distributed Tracing, distributedTraceSetting=%p, messageHandle=%p", distributedTraceSetting, messageHandle);
        result = MU_FAILURE;
    }
    else if (should_add_distributedtrace_fixed_rate_sampling(distributedTraceSetting))
    {
        char* timeBuffer;

        timeBuffer = (char*)malloc(TIME_STRING_BUFFER_LEN);
        if (timeBuffer == NULL)
        {
            LogError("malloc for timeBuffer failed");
            result = MU_FAILURE;
        }
        else if (get_epoch_time(timeBuffer) == NULL)
        {
            LogError("Failed getting current time");
            free(timeBuffer);
            result = MU_FAILURE;
        }
        else if ((messagePropertyContent = STRING_construct(MESSAGE_DISTRIBUTED_TRACING_TIMESTAMP_KEY)) == NULL)
        {
            LogError("Failed to allocate messagePropertyContent");
            result = MU_FAILURE;
        }
        else if (STRING_concat(messagePropertyContent, (const char*)timeBuffer) != 0)
        {
            LogError("Failed to generate message system property string");
            free(timeBuffer);
            result = MU_FAILURE;
        }
        else if (IoTHubMessage_SetDistributedTracingSystemProperty(messageHandle, STRING_c_str(messagePropertyContent)) != IOTHUB_MESSAGE_OK)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_002: [ IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary should return nonezero if failing to add distributed tracing property. ]*/
            LogError("Failed to set distributed tracing system property");
            free(timeBuffer);
            result = MU_FAILURE;
        }
        else
        {
            free(timeBuffer);
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    STRING_delete(messagePropertyContent);

    return result;
}

static int parse_json_object_for_dtracing_property(JSON_Object* desiredJsonObject, const char* key, uint8_t currentValue, uint8_t* retValue, STRING_HANDLE* message, const char** statusCode, bool isPartialUpdate)
{
    int result = 0;
    if (json_object_dothas_value(desiredJsonObject, key))
    {
        JSON_Value* value = NULL;
        JSON_Value_Type valueType = JSONNull;

        if ((value = json_object_dotget_value(desiredJsonObject, key)) == NULL)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_010: [ IoTHubClient_DistributedTracing_UpdateFromTwin should keep sampling mode untouched when failing parse sampling mode from twin. ]*/
            if ((*message = STRING_construct_sprintf("Property %s cannot be parsed, leaving old setting.", key)) == NULL)
            {
                result = MU_FAILURE;
            }

            *statusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
        }
        else if ((valueType = json_value_get_type(value)) == JSONNull)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_014: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingMode = false when sampling mode in twin is null. ]*/
            *retValue = 0;
            if ((*message = STRING_construct_sprintf("Property %s is set to null, set it to default.", key)) == NULL)
            {
                result = MU_FAILURE;
            }

            *statusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
        }
        else if (valueType != JSONNumber)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_010: [ IoTHubClient_DistributedTracing_UpdateFromTwin should keep sampling mode untouched when failing parse sampling mode from twin. ]*/
            if ((*message = STRING_construct_sprintf("Cannot parse property %s from twin settings, leaving old setting.", key)) == NULL)
            {
                result = MU_FAILURE;
            }

            *statusCode = DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION;
        }
        else
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_015: [ IoTHubClient_DistributedTracing_UpdateFromTwin should keep sampling mode untouched if sampling mode parsed from twin is not between [0,3]. ]*/
            *retValue = (uint8_t)json_object_dotget_number(desiredJsonObject, key);
            *statusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
        }
    }
    else 
    {
        if (!isPartialUpdate)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_013: [ IoTHubClient_DistributedTracing_UpdateFromTwin should report diagnostic property does not exist if there is no sampling mode in complete twin. ]*/
            if ((*message = STRING_construct_sprintf("Property %s does not exist.", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY)) == NULL)
            {
                /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_014: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonzero if STRING_sprintf failed. ]*/
                result = MU_FAILURE;
            }

            *statusCode = DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION;
        }
        else
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_015: [ IoTHubClient_DistributedTracing_UpdateFromTwin should report current value, if sampling mode is not parsed from a twin partial update. ]*/
            *retValue = currentValue;
            *statusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
        }
    }

    return result;
}

int IoTHubClient_DistributedTracing_UpdateFromTwin(IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA* distributedTracingSetting, bool isPartialUpdate, const unsigned char* payLoad, STRING_HANDLE* reportedStatePayload)
{
    int result = 0;
    STRING_HANDLE modeMessage = NULL;
    STRING_HANDLE rateMessage = NULL;
    const char* modeStatusCode = NULL;
    const char* rateStatusCode = NULL;

    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_006: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonezero if arguments are NULL. ]*/
    if (distributedTracingSetting == NULL || payLoad == NULL)
    {
        LogError("Invalid parameter for IoTHubClient_DistributedTracing_UpdateFromTwin, distributedTracingSetting=%p, payLoad=%p", distributedTracingSetting, payLoad);
        result = MU_FAILURE;
    }
    else
    {
        uint8_t current_sampling_mode = distributedTracingSetting->samplingMode;
        uint8_t current_sampling_rate = distributedTracingSetting->samplingRate;
        JSON_Value* json = NULL;
        JSON_Object* jsonObject = NULL;
        JSON_Object* desiredJsonObject = NULL;
        
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_007: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonzero if payLoad is not a valid json string. ]*/
        if ((json = json_parse_string((const char *)payLoad)) == NULL)
        {
            LogError("Invalid JSON payload=%s", (const char *)payLoad);
            result = MU_FAILURE;
        }
        else if ((jsonObject = json_value_get_object(json)) == NULL)
        {
            result = MU_FAILURE;
        }
        else if ((desiredJsonObject = isPartialUpdate
                ? jsonObject
                : json_object_get_object(jsonObject, "desired")) == NULL)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_008: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonezero if device twin json doesn't contain a valid desired property. ]*/
            result = MU_FAILURE;
        }
        else 
        {
            uint8_t sampling_mode;
            if (parse_json_object_for_dtracing_property(desiredJsonObject, DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY, current_sampling_mode, &sampling_mode, &modeMessage, &modeStatusCode, isPartialUpdate) != 0)
            {
                LogError("Error calling parse_json_object_for_dtracing_property for distributed tracing reported status while parsing JSON for sampling mode");
                result = MU_FAILURE;
            }
            else if (modeStatusCode == DEVICE_TWIN_REPORTED_STATUS_SUCCESS)
            {
                if (sampling_mode < 0 || sampling_mode > 3)
                {
                    if ((modeMessage = STRING_construct_sprintf("The value of property %s must be between [0, 3].", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY)) == NULL)
                    {
                        LogError("Error calling STRING_construct_sprintf for distributed tracing reported status bounds error checking for sampling mode");
                        result = MU_FAILURE;
                    }

                    modeStatusCode = DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_012: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingMode correctly if sampling mode is valid. ]*/
                    distributedTracingSetting->samplingMode = sampling_mode;
                    if ((modeMessage = STRING_new()) == NULL)
                    {
                        LogError("Error calling STRING_new for distributed tracing reported status bounds error checking for sampling mode");
                        result = MU_FAILURE;
                    }

                    modeStatusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
                }
            }

            uint8_t sampling_rate;
            if (parse_json_object_for_dtracing_property(desiredJsonObject, DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY, current_sampling_rate, &sampling_rate, &rateMessage, &rateStatusCode, isPartialUpdate) != 0)
            {
                LogError("Error calling parse_json_object_for_dtracing_property for distributed tracing reported status while parsing JSON for sampling rate");
                result = MU_FAILURE;
            }
            else if (rateStatusCode == DEVICE_TWIN_REPORTED_STATUS_SUCCESS)
            {
                if (sampling_rate < 0 || sampling_rate > 100)
                {
                    if ((rateMessage = STRING_construct_sprintf("The value of property %s must be between [0, 100].", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY)) == NULL)
                    {
                        LogError("Error calling STRING_sprintf for distributed tracing reported status bounds error checking for sampling rate");
                        result = MU_FAILURE;
                    }

                    rateStatusCode = DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_013: [ IoTHubClient_DistributedTracing_UpdateFromTwin should reset the sampling sequence, after diagSamplingPercentage changes. ]*/
                    if (current_sampling_mode != sampling_mode || current_sampling_rate != sampling_rate)
                    {
                        distributedTracingSetting->currentMessageNumber = 0;
                    }

                    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_014: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingPercentage correctly if sampling rate is valid. ]*/
                    distributedTracingSetting->samplingRate = sampling_rate;

                    if ((rateMessage = STRING_new()) == NULL)
                    {
                        LogError("Error calling STRING_new for distributed tracing reported status bounds error checking for sampling rate");
                        result = MU_FAILURE;
                    }

                    rateStatusCode = DEVICE_TWIN_REPORTED_STATUS_SUCCESS;
                }
            }
        }

        if (current_sampling_mode != distributedTracingSetting->samplingMode || current_sampling_rate != distributedTracingSetting->samplingRate || 
            modeStatusCode != DEVICE_TWIN_REPORTED_STATUS_SUCCESS || rateStatusCode != DEVICE_TWIN_REPORTED_STATUS_SUCCESS)
        {
            if ((*reportedStatePayload = STRING_new()) == NULL)
            {
                LogError("Error creating message string");
                result = MU_FAILURE;
            }
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_015: [ IoTHubClient_DistributedTracing_UpdateFromTwin should report back the sampling rate and status of the property update. ]*/
            else if (STRING_sprintf(*reportedStatePayload, DISTRIBUTED_TRACING_REPORTED_TWIN_TEMPLATE, 
                                                            distributedTracingSetting->samplingMode, 
                                                            modeStatusCode, 
                                                            STRING_c_str(modeMessage), 
                                                            distributedTracingSetting->samplingRate, 
                                                            rateStatusCode, 
                                                            STRING_c_str(rateMessage)) != 0)
            {
                LogError("Error calling STRING_sprintf for distributed tracing reported status");
                STRING_delete(*reportedStatePayload);
                reportedStatePayload = NULL;
                result = MU_FAILURE;
            }
        }

        if (json != NULL)
            json_value_free(json);

        STRING_delete(modeMessage);
        STRING_delete(rateMessage);
    }

    return result;
}
