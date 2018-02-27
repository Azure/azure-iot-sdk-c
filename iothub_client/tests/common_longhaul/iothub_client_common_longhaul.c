// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_client_common_longhaul.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/uuid.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client_options.h"
#include "iothub_client.h"
#include "iothub_message.h"
#include "iothub_service_client_auth.h"
#include "iothub_messaging.h"
#include "iothubtransport_amqp_messenger.h"
#include "iothubtest.h"

#define TELEMETRY_MESSAGE_TEST_ID_FORMAT "longhaul-tests:%s"
#define TELEMETRY_MESSAGE_MESSAGE_ID_FORMAT "message-id:%u"
#define TELEMETRY_MESSAGE_FORMAT TELEMETRY_MESSAGE_TEST_ID_FORMAT ";" TELEMETRY_MESSAGE_MESSAGE_ID_FORMAT

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES)
DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES)

#define INDEFINITE_TIME                       ((time_t)-1)
#define MAX_TELEMETRY_TRAVEL_TIME_SECS        300.0
#define SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS 60

typedef struct IOTHUB_LONGHAUL_RESOURCES_TAG
{
    char* test_id;
    LOCK_HANDLE lock;
    IOTHUB_ACCOUNT_INFO_HANDLE iotHubAccountInfo;
    IOTHUB_CLIENT_STATISTICS_HANDLE iotHubClientStats;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_TEST_HANDLE iotHubTestHandle;
    IOTHUB_PROVISIONED_DEVICE* deviceInfo;
    unsigned int counter;
} IOTHUB_LONGHAUL_RESOURCES;

typedef struct SEND_TELEMETRY_CONTEXT_TAG
{
    unsigned int message_id;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul;
} SEND_TELEMETRY_CONTEXT;

static time_t add_seconds(time_t base_time, int seconds)
{
    time_t new_time;
    struct tm *bd_new_time;

    if ((bd_new_time = localtime(&base_time)) == NULL)
    {
        new_time = INDEFINITE_TIME;
    }
    else
    {
        bd_new_time->tm_sec += seconds;
        new_time = mktime(bd_new_time);
    }

    return new_time;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)userContextCallback;

    if (iothub_client_statistics_add_connection_status(iotHubLonghaul->iotHubClientStats, status, reason) != 0)
    {
        LogError("Failed adding connection status statistics (%s, %s)", 
            ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, status), ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT c2d_message_received_callback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    (void)message;
    (void)userContextCallback;
    // NEXT: implement c2d test 
    return IOTHUBMESSAGE_ACCEPTED;
}

static unsigned int generate_unique_id(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    unsigned int result;

    if (Lock(iotHubLonghaul->lock) != LOCK_OK)
    {
        LogError("Failed to lock");
        result = 0;
    }
    else
    {
        result = (++iotHubLonghaul->counter); // Increment first then assign.

        if (Unlock(iotHubLonghaul->lock) != LOCK_OK)
        {
            LogError("Failed to unlock (%d)", result);
        }
    }

    return result;
}

typedef int(*RUN_ON_LOOP_ACTION)(const void* context);

static int run_on_loop(RUN_ON_LOOP_ACTION action, size_t iterationDurationInSeconds, size_t totalDurationInSeconds, const void* action_context)
{
    int result;
    time_t start_time;

    if ((start_time = time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failed setting start time");
        result = __FAILURE__;
    }
    else
    {
        time_t current_time;
        time_t iteration_start_time;

        result = 0;

        do
        {
            if ((iteration_start_time = time(NULL)) == INDEFINITE_TIME)
            {
                LogError("Failed setting iteration start time");
                result = __FAILURE__;
                break;
            }
            else if (action(action_context) != 0)
            {
                LogError("Loop terminated by action function result");
                result = __FAILURE__;
                break;
            }
            else if ((current_time = time(NULL)) == INDEFINITE_TIME)
            {
                LogError("Failed getting current time");
                result = __FAILURE__;
                break;
            }
            else
            {
                double wait_time_secs = iterationDurationInSeconds - difftime(current_time, iteration_start_time);

                if (wait_time_secs > 0)
                {
                    ThreadAPI_Sleep((unsigned int)(1000 * wait_time_secs));
                }

                // We should get the current time again to be 100% precise, but we will optimize here since wait_time_secs is supposed to be way smaller than totalDurationInSeconds.
            }
        } while (difftime(current_time, start_time) < totalDurationInSeconds);
    }

    return result;
}

// Public APIs

IOTHUB_ACCOUNT_INFO_HANDLE longhaul_get_account_info(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle)
{
    IOTHUB_ACCOUNT_INFO_HANDLE result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;
        result = iotHubLonghaulRsrcs->iotHubAccountInfo;
    }

    return result;
}

IOTHUB_CLIENT_HANDLE longhaul_get_iothub_client_handle(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle)
{
    IOTHUB_CLIENT_HANDLE result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;
        result = iotHubLonghaulRsrcs->iotHubClientHandle;
    }

    return result;
}

IOTHUB_CLIENT_STATISTICS_HANDLE longhaul_get_statistics(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle)
{
    IOTHUB_CLIENT_STATISTICS_HANDLE result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;
        result = iotHubLonghaulRsrcs->iotHubClientStats;
    }

    return result;
}

void longhaul_tests_deinit(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaulRsrcs->lock != NULL)
        {
            Lock_Deinit(iotHubLonghaulRsrcs->lock);
            iotHubLonghaulRsrcs->lock = NULL;
        }

        if (iotHubLonghaulRsrcs->iotHubClientHandle != NULL)
        {
            IoTHubClient_Destroy(iotHubLonghaulRsrcs->iotHubClientHandle);
            iotHubLonghaulRsrcs->iotHubClientHandle = NULL;
        }

        if (iotHubLonghaulRsrcs->iotHubAccountInfo != NULL)
        {
            IoTHubAccount_deinit(iotHubLonghaulRsrcs->iotHubAccountInfo);
            iotHubLonghaulRsrcs->iotHubAccountInfo = NULL;
        }

        if (iotHubLonghaulRsrcs->iotHubClientStats != NULL)
        {
            iothub_client_statistics_destroy(iotHubLonghaulRsrcs->iotHubClientStats);
            iotHubLonghaulRsrcs->iotHubClientStats = NULL;
        }

        if (iotHubLonghaulRsrcs->test_id != NULL)
        {
            free(iotHubLonghaulRsrcs->test_id);
            iotHubLonghaulRsrcs->test_id = NULL;
        }

        platform_deinit();

        free((void*)handle);
    }
}

IOTHUB_LONGHAUL_RESOURCES_HANDLE longhaul_tests_init()
{
    IOTHUB_LONGHAUL_RESOURCES* result;
    UUID uuid;

    if (UUID_generate(&uuid) != 0)
    {
        LogError("Failed to generate test ID number");
        result = NULL;
    }
    else
    {
        if ((result = (IOTHUB_LONGHAUL_RESOURCES*)malloc(sizeof(IOTHUB_LONGHAUL_RESOURCES))) == NULL)
        {
            LogError("Failed to allocate IOTHUB_LONGHAUL_RESOURCES struct");
        }
        else
        {
            (void)memset(result, 0, sizeof(IOTHUB_LONGHAUL_RESOURCES));

            if ((result->test_id = UUID_to_string(&uuid)) == NULL)
            {
                LogError("Failed to set test ID number");
                longhaul_tests_deinit(result);
                result = NULL;
            }
            else if (platform_init() != 0)
            {
                LogError("Platform init failed");
                longhaul_tests_deinit(result);
                result = NULL;
            }
            else if ((result->lock = Lock_Init()) == NULL)
            {
                LogError("Failed creating lock");
                longhaul_tests_deinit(result);
                result = NULL;
            }
            else if ((result->iotHubAccountInfo = IoTHubAccount_Init()) == NULL)
            {
                LogError("Failed initializing accounts");
                longhaul_tests_deinit(result);
                result = NULL;
            }
            else if ((result->iotHubClientStats = iothub_client_statistics_create()) == NULL)
            {
                LogError("Failed initializing statistics");
                longhaul_tests_deinit(result);
                result = NULL;
            }
            else
            {
                platform_init();
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_HANDLE longhaul_create_and_connect_device_client(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_HANDLE result;

    if (handle == NULL || deviceToUse == NULL)
    {
        LogError("Invalid argument (handle=%p, deviceToUse=%p)", handle, deviceToUse);
        result = NULL;
    }
    else if ((result = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol)) == NULL)
    {
        LogError("Could not create IoTHubClient");
    }
    else if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509 &&
        (IoTHubClient_SetOption(result, OPTION_X509_CERT, deviceToUse->certificate) != IOTHUB_CLIENT_OK ||
            IoTHubClient_SetOption(result, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication) != IOTHUB_CLIENT_OK))
    {
        LogError("Could not set the device x509 certificate or privateKey");
        IoTHubClient_Destroy(result);
        result = NULL;
    }
    else
    {
        bool trace = false;

        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;
        iotHubLonghaulRsrcs->iotHubClientHandle = result;

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        (void)IoTHubClient_SetOption(result, OPTION_TRUSTED_CERT, certificates);
#endif
        (void)IoTHubClient_SetOption(result, OPTION_LOG_TRACE, &trace);
        (void)IoTHubClient_SetOption(result, OPTION_PRODUCT_INFO, "C-SDK-LongHaul");

        if (IoTHubClient_SetConnectionStatusCallback(result, connection_status_callback, handle) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed setting the connection status callback");
            IoTHubClient_Destroy(result);
            iotHubLonghaulRsrcs->iotHubClientHandle = NULL;
            result = NULL;
        }
        else if (IoTHubClient_SetMessageCallback(result, c2d_message_received_callback, handle) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set the cloud-to-device message callback");
            IoTHubClient_Destroy(result);
            iotHubLonghaulRsrcs->iotHubClientHandle = NULL;
            result = NULL;
        }
        else
        {
            iotHubLonghaulRsrcs->deviceInfo = deviceToUse;
        }
    }

    return result;
}

static void on_service_client_messaging_opened(void* context)
{
    (void)context;
}

static int parse_incoming_message(const char* data, size_t size, char* test_id, unsigned int* message_id)
{
    int result;
    char data_copy[80];
    
    (void)memset(data_copy, 0, sizeof(data_copy));
    (void)memcpy(data_copy, data, size);
    data_copy[51] = '\0';

    if (sscanf(data_copy, TELEMETRY_MESSAGE_TEST_ID_FORMAT, test_id) != 1)
    {
        result = __FAILURE__;
    }
    else if (sscanf(data_copy + 52, TELEMETRY_MESSAGE_MESSAGE_ID_FORMAT, message_id) != 1)
    {
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int on_message_received(void* context, const char* data, size_t size)
{
    int result;

    if (data == NULL || size == 0)
    {
        LogError("Invalid message received (data=%p, size=%d)", data, size);
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
        unsigned int message_id;
        char tests_id[40];

        if (parse_incoming_message(data, size, tests_id, &message_id) == 0 &&
            strcmp(tests_id, iotHubLonghaul->test_id) == 0)
        {
            TELEMETRY_INFO info;
            info.message_id = message_id;
            info.time_received = time(NULL);

            if (info.time_received == INDEFINITE_TIME)
            {
                LogError("Failed setting the receive time for message %d", info.message_id);
            }

            if (iothub_client_statistics_add_telemetry_info(iotHubLonghaul->iotHubClientStats, TELEMETRY_RECEIVED, &info) != 0)
            {
                LogError("Failed adding receive info for message %d", info.message_id);
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
        else
        {
            result = __FAILURE__; // This is not the message we expected. Abandoning it.
        }
    }

    return result;
}


int longhaul_start_listening_for_telemetry_messages(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubTestHandle != NULL)
        {
            LogError("IoTHubTest already initialized");
            result = __FAILURE__;
        }
        else
        {
            if ((iotHubLonghaul->iotHubTestHandle = IoTHubTest_Initialize(
                IoTHubAccount_GetEventHubConnectionString(iotHubLonghaul->iotHubAccountInfo),
                IoTHubAccount_GetIoTHubConnString(iotHubLonghaul->iotHubAccountInfo),
                deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(iotHubLonghaul->iotHubAccountInfo),
                IoTHubAccount_GetEventhubAccessKey(iotHubLonghaul->iotHubAccountInfo),
                IoTHubAccount_GetSharedAccessSignature(iotHubLonghaul->iotHubAccountInfo),
                IoTHubAccount_GetEventhubConsumerGroup(iotHubLonghaul->iotHubAccountInfo))) == NULL)
            {
                LogError("Failed initializing IoTHubTest");
                result = __FAILURE__;
            }
            else
            {
                time_t time_start_range = add_seconds(time(NULL), SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS);

                if (time_start_range == INDEFINITE_TIME)
                {
                    LogError("Could not define the time start range");
                    IoTHubTest_Deinit(iotHubLonghaul->iotHubTestHandle);
                    iotHubLonghaul->iotHubTestHandle = NULL;
                    result = __FAILURE__;
                }
                else if (
                    IoTHubTest_ListenForEventAsync(
                        iotHubLonghaul->iotHubTestHandle,
                        IoTHubAccount_GetIoTHubPartitionCount(iotHubLonghaul->iotHubAccountInfo),
                        time_start_range, on_message_received, iotHubLonghaul) != IOTHUB_TEST_CLIENT_OK)
                {
                    LogError("Failed listening for device to cloud messages");
                    IoTHubTest_Deinit(iotHubLonghaul->iotHubTestHandle);
                    iotHubLonghaul->iotHubTestHandle = NULL;
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }

    return result;
}

int longhaul_stop_listening_for_telemetry_messages(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubTestHandle == NULL)
        {
            LogError("IoTHubTest not initialized");
            result = __FAILURE__;
        }
        else
        {
            if (IoTHubTest_ListenForEventAsync(iotHubLonghaul->iotHubTestHandle, 0, INDEFINITE_TIME, NULL, NULL) != IOTHUB_TEST_CLIENT_OK)
            {
                LogError("Failed stopping listening for device to cloud messages");
            }

            IoTHubTest_Deinit(iotHubLonghaul->iotHubTestHandle);
            iotHubLonghaul->iotHubTestHandle = NULL;

            result = 0;
        }
    }

    return result;
}

// Conveniency *run* functions

static void send_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (userContextCallback == NULL)
    {
        LogError("Invalid argument (userContextCallback is NULL)");
    }
    else
    {
        SEND_TELEMETRY_CONTEXT* message_info = (SEND_TELEMETRY_CONTEXT*)userContextCallback;

        TELEMETRY_INFO telemetry_info;
        telemetry_info.message_id = message_info->message_id;
        telemetry_info.send_callback_result = result;
        telemetry_info.time_sent = time(NULL);

        if (telemetry_info.time_sent == INDEFINITE_TIME)
        {
            LogError("Failed setting the time telemetry was sent");
        }
        else if (iothub_client_statistics_add_telemetry_info(message_info->iotHubLonghaul->iotHubClientStats, TELEMETRY_SENT, &telemetry_info) != 0)
        {
            LogError("Failed adding telemetry statistics info (message_id=%d)", message_info->message_id);
        }

        free(message_info);
    }
}

static IOTHUB_MESSAGE_HANDLE create_telemetry_message(const char* test_id, unsigned int message_id)
{
    IOTHUB_MESSAGE_HANDLE result;
    char msg_text[80];

    (void)memset(msg_text, 0, sizeof(msg_text));

    if (sprintf_s(msg_text, sizeof(msg_text), TELEMETRY_MESSAGE_FORMAT, test_id, message_id) <= 0)
    {
        LogError("Failed creating text for iothub message");
        result = NULL;
    }
    else if ((result = IoTHubMessage_CreateFromString(msg_text)) == NULL)
    {
        LogError("Failed creating IOTHUB_MESSAGE_HANDLE");
    }

    return result;
}

static SEND_TELEMETRY_CONTEXT* create_telemetry_message_context(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    SEND_TELEMETRY_CONTEXT* result;

    if ((result = (SEND_TELEMETRY_CONTEXT*)malloc(sizeof(SEND_TELEMETRY_CONTEXT))) == NULL)
    {
        LogError("Failed allocating telemetry message context");
    }
    else
    {
        result->message_id = iotHubLonghaul->counter;
        result->iotHubLonghaul = iotHubLonghaul;
    }

    return result;
}

static int send_telemetry(const void* context)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES* longhaulResources = (IOTHUB_LONGHAUL_RESOURCES*)context;
    unsigned int message_id;

    if ((message_id = generate_unique_id(longhaulResources)) == 0)
    {
        LogError("Failed generating telemetry message id");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE message;

        if ((message = create_telemetry_message(longhaulResources->test_id, message_id)) == NULL)
        {
            LogError("Failed creating telemetry message");
            result = __FAILURE__;
        }
        else
        {
            SEND_TELEMETRY_CONTEXT* message_info;

            if ((message_info = (SEND_TELEMETRY_CONTEXT*)malloc(sizeof(SEND_TELEMETRY_CONTEXT))) == NULL)
            {
                LogError("Failed allocating context for telemetry message");
                result = __FAILURE__;
            }
            else
            {
                TELEMETRY_INFO telemetry_info;
                telemetry_info.message_id = message_id;
                telemetry_info.time_queued = time(NULL);

                message_info->message_id = message_id;
                message_info->iotHubLonghaul = longhaulResources;

                if (IoTHubClient_SendEventAsync(longhaulResources->iotHubClientHandle, message, send_confirmation_callback, message_info) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed sending telemetry message");
                    free(message_info);
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }

                telemetry_info.send_result = result;

                if (iothub_client_statistics_add_telemetry_info(longhaulResources->iotHubClientStats, TELEMETRY_QUEUED, &telemetry_info) != 0)
                {
                    LogError("Failed adding telemetry statistics info (message_id=%d)", message_id);
                    result = __FAILURE__;
                }
            }

            IoTHubMessage_Destroy(message);
        }
    }

    return result;
}

int longhaul_run_telemetry_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalig argument (handle is NULL)");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaulRsrcs->iotHubClientHandle == NULL || iotHubLonghaulRsrcs->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = __FAILURE__;
        }
        else
        {
            if (longhaul_start_listening_for_telemetry_messages(handle, iotHubLonghaulRsrcs->deviceInfo) != 0)
            {
                LogError("Failed listening for telemetry messages");
                result = __FAILURE__;
            }
            else
            {
                int loop_result;
                IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

                loop_result = run_on_loop(send_telemetry, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaulRsrcs);

                ThreadAPI_Sleep(iterationDurationInSeconds * 1000 * 10); // Extra time for the last messages.

                stats_handle = longhaul_get_statistics(iotHubLonghaulRsrcs);

                LogInfo("Longhaul telemetry stats: %s", iothub_client_statistics_to_json(stats_handle));

                if (loop_result != 0)
                {
                    result = __FAILURE__;
                }
                else
                {
                    IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY summary;

                    if (iothub_client_statistics_get_telemetry_summary(stats_handle, &summary) != 0)
                    {
                        LogError("Failed gettting statistics summary");
                        result = __FAILURE__;
                    }
                    else
                    {
                        LogInfo("Summary: Messages sent=%d, received=%d; travel time: min=%f secs, max=%f secs", 
                            summary.messages_sent, summary.messages_received, summary.min_travel_time_secs, summary.max_travel_time_secs);
                     
                        if (summary.messages_sent == 0 || summary.messages_received != summary.messages_sent || summary.max_travel_time_secs > MAX_TELEMETRY_TRAVEL_TIME_SECS)
                        {
                            result = __FAILURE__;
                        }
                        else
                        {
                            result = 0;
                        }
                    }
                }

                (void)longhaul_stop_listening_for_telemetry_messages(iotHubLonghaulRsrcs);
            }
        }
    }

    return result;
}

int longhaul_run_c2d_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    (void)handle;
    (void)iterationDurationInSeconds;
    (void)totalDurationInSeconds;
    // TO BE SENT ON A SEPARATE CODE REVIEW
    return 0;
}

int longhaul_run_device_methods_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    (void)handle;
    (void)iterationDurationInSeconds;
    (void)totalDurationInSeconds;
    // TO BE SENT ON A SEPARATE CODE REVIEW
    return 0;
}

int longhaul_run_twin_desired_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    (void)handle;
    (void)iterationDurationInSeconds;
    (void)totalDurationInSeconds;
    // TO BE SENT ON A SEPARATE CODE REVIEW
    return 0;
}

int longhaul_run_twin_reported_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    (void)handle;
    (void)iterationDurationInSeconds;
    (void)totalDurationInSeconds;
    // TO BE SENT ON A SEPARATE CODE REVIEW
    return 0;
}