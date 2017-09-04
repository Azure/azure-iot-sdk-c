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
    if (value <= 9)
    {
        return '0' + value;
    }
    else
    {
        return 'a' + value - 10;
    }
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
    if (diagSetting->diagSamplingPercentage > 0)
    {
        if (diagSetting->currentMessageNumber == UINT32_MAX)
        {
            diagSetting->currentMessageNumber %= diagSetting->diagSamplingPercentage;
        }
        ++diagSetting->currentMessageNumber;

        double number = diagSetting->currentMessageNumber;
        double percentage = diagSetting->diagSamplingPercentage;
        return (floor((number - 2) * percentage / 100.0) < floor((number - 1) * percentage / 100.0));
    }
    return false;
}

bool IoTHubClient_Diagnostic_AddIfNecessary(IOTHUB_DIAGNOSTIC_SETTING_DATA* diagSetting, IOTHUB_MESSAGE_HANDLE messageHandle)
{
    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_001: [ IoTHubClient_Diagnostic_AddIfNecessary should return false if diagSetting or messageHandle is NULL. ]*/
    if (diagSetting == NULL || messageHandle == NULL)
    {
        return false;
    }

    /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_003: [ If diagSamplingPercentage is equal to 0, message number should not be increased and no diagnostic properties added ]*/
    if (should_add_diagnostic_info(diagSetting))
    {
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_004: [ If diagSamplingPercentage is equal to 100, diagnostic properties should be added to all messages]*/
        /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_005: [ If diagSamplingPercentage is between(0, 100), diagnostic properties should be added based on percentage]*/

        //generating random string
        char randomString[9];
        if (IoTHubMessage_SetDiagnosticId(messageHandle, generate_eight_random_characters(randomString)) != IOTHUB_MESSAGE_OK)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return false if failing to add diagnostic property. ]*/
            return false;
        }

        //TODO: get current time with milliseconds precision
        char timeBuffer[30];
        get_current_time_utc(timeBuffer, 30, "%Y-%m-%dT%H:%M:%SZ");
        if (IoTHubMessage_SetDiagnosticCreationTimeUtc(messageHandle, timeBuffer) != IOTHUB_MESSAGE_OK)
        {
            /* Codes_SRS_IOTHUB_DIAGNOSTIC_13_002: [ IoTHubClient_Diagnostic_AddIfNecessary should return false if failing to add diagnostic property. ]*/
            return false;
        }
    }

    return true;
}
