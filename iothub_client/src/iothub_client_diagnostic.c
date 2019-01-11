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

#define DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY "azureiot*com^dtracing^1*0*0.sampling_mode"
#define DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY "azureiot*com^dtracing^1*0*0.sampling_rate"
#define MESSAGE_DISTRIBUTED_TRACING_TIMESTAMP_KEY "timestamp="
#define DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION "401"
#define DEVICE_TWIN_REPORTED_STATUS_SUCCESS "204"

#define DISTRIBUTED_TRACING_REPORTED_TWIN_TEMPLATE "{ \"__iot:interfaces\": \
{ \"azureiot*com^dtracing^1*0*0\": { \"@id\": \"http://azureiot.com/dtracing/1.0.0\" } }, \
\"azureiot*com^dtracing^1*0*0\": { \
    \"sampling_mode\": { \
    \"value\": \"%s\", \
    \"status\" : { \
    \"code\": %s, \
    \"description\" : \"%s\" \
    } \
}, \
\"sampling_rate\": { \
    \"value\": \"%d\", \
    \"status\" : { \
    \"code\": %s, \
    \"description\" : \"%s\" \
    } \
} \
} \
}"

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

// Deprecated
static char get_base36_char(unsigned char value)
{
    return value <= 9 ? '0' + value : 'a' + value - 10;
}

// Deprecated
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

// Deprecated
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

// Deprecated
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

// Deprecated
int IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle)
{
    int result;
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if diagSetting or messageHandle is NULL. ]*/
    if (diagSetting == NULL || messageHandle == NULL)
    {
        result = __FAILURE__;
    }
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_003: [ If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added ]*/
    else if (should_add_diagnostic_info(diagSetting))
    {
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/

        IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData;
        if ((diagnosticData = prepare_message_diagnostic_data()) == NULL)
        {
            result = __FAILURE__;
        }
        else
        {
            if (IoTHubMessage_SetDiagnosticPropertyData(messageHandle, diagnosticData) != IOTHUB_MESSAGE_OK)
            {
                /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return nonezero if failing to add diagnostic property. ]*/
                result = __FAILURE__;
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
    if (distributedTraceSetting->samplingMode && distributedTraceSetting->samplingRate > 0)
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
    STRING_HANDLE messagePropertyContent;
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_001: [ IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary should return nonezero if distributedTraceSetting or messageHandle is NULL. ]*/
    if (distributedTraceSetting == NULL || messageHandle == NULL)
    {
        LogError("Invalid paramter for Distributed Tracing");
        result = __FAILURE__;
    }
    else if ((messagePropertyContent = STRING_new()) == NULL)
    {
        LogError("Failed to allocate messagePropertyContent");
        result = __FAILURE__;
    }
    else if (should_add_distributedtrace_fixed_rate_sampling(distributedTraceSetting))
    {
        char* timeBuffer;

        timeBuffer = (char*)malloc(TIME_STRING_BUFFER_LEN);
        if (timeBuffer == NULL)
        {
            LogError("malloc for timeBuffer failed");
            STRING_delete(messagePropertyContent);
            result = __FAILURE__;
        }
        else if (get_epoch_time(timeBuffer) == NULL)
        {
            LogError("Failed getting current time");
            STRING_delete(messagePropertyContent);
            free(timeBuffer);
            result = __FAILURE__;
        }
        else if (STRING_concat(messagePropertyContent, MESSAGE_DISTRIBUTED_TRACING_TIMESTAMP_KEY) != 0 ||
            STRING_concat(messagePropertyContent, (const char*)timeBuffer) != 0)
        {
            LogError("Failed to generate message system property string");
            STRING_delete(messagePropertyContent);
            free(timeBuffer);
            result = __FAILURE__;
        }
        else if (IoTHubMessage_SetDistributedTracingSystemProperty(messageHandle, STRING_c_str(messagePropertyContent)) != IOTHUB_MESSAGE_OK)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_38_002: [ IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary should return nonezero if failing to add distributed tracing property. ]*/
            LogError("Failed to set distributed tracing system property");
            STRING_delete(messagePropertyContent);
            free(timeBuffer);
            result = __FAILURE__;
        }
        else
        {
            STRING_delete(messagePropertyContent);
            free(timeBuffer);
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

static int parse_json_object_for_dtracing_property(JSON_Object* desiredJsonObject, const char* key, int* retValue, STRING_HANDLE message, STRING_HANDLE statusCode, bool isPartialUpdate)
{
    int result = 0;
    if (json_object_dothas_value(desiredJsonObject, key))
    {
        JSON_Value* value = json_object_dotget_value(desiredJsonObject, key);
        JSON_Value_Type valueType = json_value_get_type(value);
        if (valueType == JSONNull)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_013: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingMode = false when sampling mode in twin is null. ]*/
            *retValue = 1;
            if (STRING_sprintf(message, "Property %s is set to null, so set it to false.", key) != 0 ||
                STRING_sprintf(statusCode, DEVICE_TWIN_REPORTED_STATUS_SUCCESS) != 0)
            {
                result = __FAILURE__;
            }
        }
        else if (valueType != JSONNumber)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_010: [ IoTHubClient_DistributedTracing_UpdateFromTwin should keep sampling mode untouched when failing parse sampling mode from twin. ]*/
            if (STRING_sprintf(message, "Cannot parse property %s from twin settings.", key) != 0 ||
                STRING_sprintf(statusCode, DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION) != 0)
            {
                result = __FAILURE__;
            }
        }
        else
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_011: [ IoTHubClient_DistributedTracing_UpdateFromTwin should keep sampling mode untouched if sampling mode parsed from twin is not between [0,3]. ]*/
            *retValue = (int)json_object_dotget_number(desiredJsonObject, key);
        }
    }
    else if (!isPartialUpdate)
    {
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_013: [ IoTHubClient_DistributedTracing_UpdateFromTwin should report diagnostic property does not exist if there is no sampling mode in complete twin. ]*/
        if (STRING_sprintf(message, "Property %s does not exist.", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY) != 0 ||
            STRING_sprintf(statusCode, DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION) != 0)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_014: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonzero if STRING_sprintf failed. ]*/
            result = __FAILURE__;
        }
    }

    return result;
}

int IoTHubClient_DistributedTracing_UpdateFromTwin(IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA* distributedTracingSetting, bool isPartialUpdate, const unsigned char* payLoad, STRING_HANDLE reportedStatePayload)
{
    int result = 0;
    STRING_HANDLE modeMessage = STRING_new();
    STRING_HANDLE rateMessage = STRING_new();
    STRING_HANDLE modeStatusCode = STRING_new();
    STRING_HANDLE rateStatusCode = STRING_new();

    if (modeStatusCode == NULL || rateStatusCode == NULL || modeMessage == NULL || rateMessage == NULL)
    {
        LogError("Error creating distributed tracing reported string");
        result = __FAILURE__;
    }
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_006: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonezero if arguments are NULL. ]*/
    else if (distributedTracingSetting == NULL || payLoad == NULL)
    {
        result = __FAILURE__;
    }
    else
    {
        JSON_Value* json;
        JSON_Object* jsonObject;
        JSON_Object* desiredJsonObject;
        
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_007: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonezero if payLoad is not a valid json string. ]*/
        if ((json = json_parse_string((const char *)payLoad)) == NULL)
        {
            result = __FAILURE__;
        }
        else if ((jsonObject = json_value_get_object(json)) == NULL)
        {
            result = __FAILURE__;
        }
        else if ((desiredJsonObject = isPartialUpdate
                ? jsonObject
                : json_object_get_object(jsonObject, "desired")) == NULL)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_008: [ IoTHubClient_DistributedTracing_UpdateFromTwin should return nonezero if device twin json doesn't contain a valid desired property. ]*/
            result = __FAILURE__;
        }
        else 
        {
            int sampling_mode;
            if (parse_json_object_for_dtracing_property(desiredJsonObject, DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY, &sampling_mode, modeMessage, modeStatusCode, isPartialUpdate) != 0)
            {
                LogError("Error calling parse_json_object_for_dtracing_property for distributed tracing reported status while parsing JSON for sampling mode");
                result = __FAILURE__;
            }
            else if (sampling_mode < 0 || sampling_mode > 3)
            {
                if (STRING_sprintf(modeMessage, "The value of property %s must be between [0, 3].", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_MODE_KEY) != 0 ||
                    STRING_sprintf(modeStatusCode, DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION) != 0)
                {
                    LogError("Error calling STRING_sprintf for distributed tracing reported status bounds error checking for sampling mode");
                    result = __FAILURE__;
                }
            }
            else
            {
                /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_012: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingMode correctly if sampling mode is valid. ]*/
                distributedTracingSetting->samplingMode = (sampling_mode > 1) ? true : false;
                if (STRING_sprintf(modeStatusCode, DEVICE_TWIN_REPORTED_STATUS_SUCCESS) != 0)
                {
                    LogError("Error calling STRING_sprintf for distributed tracing reported status success for sampling mode");
                    result = __FAILURE__;
                }
            }

            int sampling_rate;
            if (parse_json_object_for_dtracing_property(desiredJsonObject, DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY, &sampling_rate, rateMessage, rateStatusCode, isPartialUpdate) != 0)
            {
                LogError("Error calling parse_json_object_for_dtracing_property for distributed tracing reported status while parsing JSON for sampling rate");
                result = __FAILURE__;
            }
            else if (sampling_rate < 0 || sampling_rate > 100)
            {
                if (STRING_sprintf(rateMessage, "The value of property %s must be between [0, 100].", DEVICE_TWIN_DISTRIBUTED_TRACING_SAMPLING_RATE_KEY) != 0 ||
                    STRING_sprintf(rateStatusCode, DEVICE_TWIN_REPORTED_STATUS_FAILED_OPERATION) != 0)
                {
                    LogError("Error calling STRING_sprintf for distributed tracing reported status bounds error checking for sampling rate");
                    result = __FAILURE__;
                }
            }
            else
            {
                /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_012: [ IoTHubClient_DistributedTracing_UpdateFromTwin should set diagSamplingPercentage correctly if sampling rate is valid. ]*/
                distributedTracingSetting->samplingRate = (int)sampling_rate;
                if (STRING_sprintf(rateStatusCode, DEVICE_TWIN_REPORTED_STATUS_SUCCESS) != 0)
                {
                    LogError("Error calling STRING_sprintf for distributed tracing reported status success for sampling rate");
                    result = __FAILURE__;
                }
            }
        }

        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_014: [ IoTHubClient_DistributedTracing_UpdateFromTwin should report back the sampling rate and status of the property update. ]*/
        if (STRING_sprintf(reportedStatePayload, DISTRIBUTED_TRACING_REPORTED_TWIN_TEMPLATE, (distributedTracingSetting->samplingMode) ? "2" : "1", STRING_c_str(modeStatusCode), STRING_c_str(modeMessage), 
                                                                                                distributedTracingSetting->samplingRate, STRING_c_str(rateStatusCode), STRING_c_str(rateMessage)) != 0)
        {
            LogError("Error calling STRING_sprintf for distributed tracing reported status");
            result = __FAILURE__;
        }
        
        json_value_free(json);

        STRING_delete(modeMessage);
        STRING_delete(modeStatusCode);
        STRING_delete(rateMessage);
        STRING_delete(rateStatusCode);
    }

    return result;
}
