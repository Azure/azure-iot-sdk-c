// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/consolelogger.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/connection_string_parser.h"

#include "iothub_service_client_auth.h"
#include "iothub_message_monitoring.h"

DEFINE_ENUM_STRINGS(IOTHUB_MESSAGE_MONITORING_STATE, IOTHUB_MESSAGE_MONITORING_STATE_VALUES);

/* String containing Hostname, SharedAccessKeyName and SharedAccessKey in the format:                       */
/* "HostName=<host_name>;SharedAccessKeyName=<shared_access_key_name>;SharedAccessKey=<shared_access_key>" */
static const char* connectionString = "[IoTHub Connection String]";
static const char* deviceId = "[Device Id]";
static const char* consumerGroup = "$Default";
static const size_t partitionNumber = 0; // Enter your partition number.

static void stateChangedCallback(void* context, IOTHUB_MESSAGE_MONITORING_STATE new_state, IOTHUB_MESSAGE_MONITORING_STATE previous_state)
{
    (void)context;
    (void)printf("Message Monitoring state changed (new=%s, previous=%s)", ENUM_TO_STRING(IOTHUB_MESSAGE_MONITORING_STATE, new_state), ENUM_TO_STRING(IOTHUB_MESSAGE_MONITORING_STATE, previous_state));
}

static void messageReceivedCallback(void* context, IOTHUB_MESSAGE_HANDLE message)
{
    (void)context;

    IOTHUBMESSAGE_CONTENT_TYPE type;
    time_t now;
    
    type = IoTHubMessage_GetContentType(message);
    now = time(NULL);

    (void)printf("New message received at %s:\r\n", ctime((const time_t*)&now));

    if (type == IOTHUBMESSAGE_STRING)
    {
        const char* message_charptr = IoTHubMessage_GetString(message);

        (void)printf("Content: %s\r\n", message_charptr);
    }
    else
    {
        unsigned char* bytes;
        size_t size;

        (void)IoTHubMessage_GetByteArray(message, &bytes, &size);

        (void)printf("Content: %s\r\n", bytes);
        (void)printf("Size: %d\r\n", size);
    }

    {
        MAP_HANDLE properties = IoTHubMessage_Properties(message);

        const char* const* keys;
        const char* const* values;
        size_t count;

        (void)Map_GetInternals(properties, &keys, &values, &count);

        if (count > 0)
        {
            size_t i;
            (void)printf("Properties (%d):\r\n", count);

            for (i = 0; i < count; i++)
            {
                (void)printf("[%s]: %s\r\n", keys[i], values[i]);
            }
        }
    }
}

static void iothub_message_monitoring_sample_run(void)
{
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_MESSAGE_MONITORING_HANDLE iotHubMessageMonitoringHandle;
    IOTHUB_MESSAGE_FILTER messageFilter;
    bool enableLogging;

    (void)platform_init();

    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    
    iotHubMessageMonitoringHandle = IoTHubMessageMonitoring_Create(iotHubServiceClientHandle);

    enableLogging = true;
    (void)IoTHubMessageMonitoring_SetOption(iotHubMessageMonitoringHandle, OPTION_ENABLE_LOGGING, &enableLogging);

    messageFilter.receiveTimeStartUTC = time(NULL);

    (void)IoTHubMessageMonitoring_Open(iotHubMessageMonitoringHandle, consumerGroup, partitionNumber, &messageFilter, stateChangedCallback, NULL);

    (void)IoTHubMessageMonitoring_SetMessageCallback(iotHubMessageMonitoringHandle, messageReceivedCallback, NULL);

    (void)printf("Press any key to continue...\r\n");
    (void)getchar();

    IoTHubMessageMonitoring_Destroy(iotHubMessageMonitoringHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    platform_deinit();
}

int main(void)
{
    iothub_message_monitoring_sample_run();
    return 0;
}