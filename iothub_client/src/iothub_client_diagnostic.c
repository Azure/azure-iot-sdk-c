// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <math.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/buffer_.h"

#include "iothub_client_diagnostic.h"

static const int BASE_36 = 36;

static char* get_current_time_utc(char* timeBuffer, int bufferLen, const char* formatString)
{
    time_t epochTime;
    struct tm* utcTime;
    epochTime = time(NULL);
    utcTime = gmtime(&epochTime);
    strftime(timeBuffer, bufferLen, formatString, utcTime);
    return timeBuffer;
}

static char get_base36_char(unsigned char value)
{
    return value <= 9 ? '0' + value : 'a' + value - 10;
}

static char* generate_eight_random_characters(char *randomString)
{
    char* randomStringPos = randomString;
    for (int i = 0; i < 4; ++i)
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
        if (diagSetting->currentMessageNumber == UINT32_MAX)
        {
            diagSetting->currentMessageNumber %= diagSetting->diagSamplingPercentage * 100;
        }
        ++diagSetting->currentMessageNumber;

        double number = diagSetting->currentMessageNumber;
        double percentage = diagSetting->diagSamplingPercentage;
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
            generate_eight_random_characters(diagId);
            result->diagnosticId = diagId;

            char* timeBuffer = (char*)malloc(30);
            if (timeBuffer == NULL)
            {
                LogError("malloc for timeBuffer failed");
                free(result->diagnosticId);
                free(result);
                result = NULL;
            }
            else
            {
                get_current_time_utc(timeBuffer, 30, "%Y-%m-%dT%H:%M:%SZ");
                result->diagnosticCreationTimeUtc = timeBuffer;
            }
        }
    }
    return result;
}

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
