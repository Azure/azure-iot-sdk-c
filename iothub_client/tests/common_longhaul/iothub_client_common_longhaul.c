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
#include "azure_c_shared_utility/lock.h"
#include "iothub_client_options.h"
#include "iothub_client.h"
#include "iothub_message.h"
#include "iothub_service_client_auth.h"
#include "iothub_messaging.h"
#include "iothub_devicemethod.h"
#include "iothub_devicetwin.h"
#include "internal/iothubtransport_amqp_messenger.h"
#include "iothubtest.h"
#include "parson.h"

#define MESSAGE_TEST_ID_FIELD          "longhaul-tests"
#define MESSAGE_ID_FIELD               "message-id"
#define LONGHAUL_DEVICE_METHOD_NAME    "longhaulDeviceMethod"
#define TWIN_FIELD_VERSION             "$version"
#define TWIN_DESIRED_BLOCK             "desired"
#define TWIN_PROPERTIES_BLOCK          "properties"
#define TWIN_REPORTED_BLOCK            "reported"
#define DOT                            "."


#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define INDEFINITE_TIME                         ((time_t)-1)
#define SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS   120
#define DEVICE_METHOD_SUB_WAIT_TIME_MS          (5 * 1000)

#define MAX_TELEMETRY_TRAVEL_TIME_SECS          300.0
#define MAX_C2D_TRAVEL_TIME_SECS                300.0
#define MAX_DEVICE_METHOD_TRAVEL_TIME_SECS      300
#define MAX_TWIN_DESIRED_PROP_TRAVEL_TIME_SECS  300.0
#define MAX_TWIN_REPORTED_PROP_TRAVEL_TIME_SECS 300.0

typedef struct IOTHUB_LONGHAUL_RESOURCES_TAG
{
    char* test_id;
    LOCK_HANDLE lock;
    IOTHUB_ACCOUNT_INFO_HANDLE iotHubAccountInfo;
    IOTHUB_CLIENT_STATISTICS_HANDLE iotHubClientStats;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    bool is_svc_cl_c2d_msgr_open;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubSvcMsgHandle;
    IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE iotHubSvcDevMethodHandle;
    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE iotHubSvcDevTwinHandle;
    IOTHUB_TEST_HANDLE iotHubTestHandle;
    IOTHUB_PROVISIONED_DEVICE* deviceInfo;
    unsigned int counter;
} IOTHUB_LONGHAUL_RESOURCES;

typedef struct SEND_TELEMETRY_CONTEXT_TAG
{
    unsigned int message_id;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul;
} SEND_TELEMETRY_CONTEXT;

typedef struct SEND_C2D_CONTEXT_TAG
{
    unsigned int message_id;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul;
} SEND_C2D_CONTEXT;

typedef struct SEND_TWIN_REPORTED_CONTEXT_TAG
{
    unsigned int update_id;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul;
} SEND_TWIN_REPORTED_CONTEXT;

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

static int parse_message(const char* data, char* test_id, unsigned int* message_id)
{
    int result;
    JSON_Value* root_value;

    if ((root_value = json_parse_string(data)) == NULL)
    {
        LogError("Failed parsing json string: %s", data == NULL ? "<NULL>" : data);
        result = MU_FAILURE;
    }
    else
    {
        JSON_Object* root_object;
        const char* test_id_ref;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed creating root json object: %s", data);
            result = MU_FAILURE;
        }
        else if ((test_id_ref = json_object_get_string(root_object, MESSAGE_TEST_ID_FIELD)) == NULL)
        {
            LogError("Failed getting message test id: %s", data);
            result = MU_FAILURE;
        }
        else
        {
            *message_id = (unsigned int)json_object_get_number(root_object, MESSAGE_ID_FIELD);

            (void)memcpy(test_id, test_id_ref, 36);
            test_id[36] = '\0';
            result = 0;
        }

        json_value_free(root_value);
    }

    return result;
}


static int parse_twin_desired_properties(const char* data, char* test_id, unsigned int* message_id, int* version)
{
    int result;
    JSON_Value* root_value;

    if ((root_value = json_parse_string(data)) == NULL)
    {
        LogError("Failed parsing json string: %s", data == NULL ? "<NULL>" : data);
        result = MU_FAILURE;
    }
    else
    {
        JSON_Object* root_object;
        const char* test_id_ref;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed creating root json object %s", data);
            result = MU_FAILURE;
        }
        else if ((test_id_ref = json_object_dotget_string(root_object, TWIN_DESIRED_BLOCK DOT MESSAGE_TEST_ID_FIELD)) == NULL)
        {
            LogError("Failed getting message test id %s", data);
            result = MU_FAILURE;
        }
        else
        {
            double raw_message_id = json_object_dotget_number(root_object, TWIN_DESIRED_BLOCK DOT MESSAGE_ID_FIELD);

            if (raw_message_id < 0)
            {
                LogError("Unexpected message id (%f)", raw_message_id);
                result = MU_FAILURE;
            }
            else
            {
                *message_id = (unsigned int)raw_message_id;

                if ((*version = (int)json_object_dotget_number(root_object, TWIN_DESIRED_BLOCK DOT TWIN_FIELD_VERSION)) < 0)
                {
                    LogError("Failed getting desired properties version (%d)", *message_id);
                    result = MU_FAILURE;
                }
                else
                {
                    (void)memcpy(test_id, test_id_ref, 36);
                    test_id[36] = '\0';

                    result = 0;
                }
            }
        }

        json_value_free(root_value);
    }

    return result;
}

static int parse_twin_reported_properties(const char* data, int* prop_count, char* test_id, unsigned int* message_id, int* version)
{
    int result;
    JSON_Value* root_value;

    if ((root_value = json_parse_string(data)) == NULL)
    {
        LogError("Failed parsing json string: %s", data == NULL ? "<NULL>" : data);
        result = MU_FAILURE;
    }
    else
    {
        JSON_Object* root_object;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed getting root json object %s", data);
            result = MU_FAILURE;
        }
        else
        {
            JSON_Object* reported_root_object;

            if ((reported_root_object = json_object_dotget_object(root_object, TWIN_PROPERTIES_BLOCK DOT TWIN_REPORTED_BLOCK)) == NULL)
            {
                LogError("Failed getting json reported properties block %s", data);
                result = MU_FAILURE;
            }
            else
            {
                const char* test_id_ref;

                *version = (int)json_object_dotget_number(reported_root_object, TWIN_FIELD_VERSION);
                *prop_count = 0;

                if ((test_id_ref = json_object_dotget_string(reported_root_object, MESSAGE_TEST_ID_FIELD)) != NULL)
                {
                    (void)memcpy(test_id, test_id_ref, 36);
                    test_id[36] = '\0';

                    *prop_count = *prop_count + 1;
                }

                if (json_object_has_value(reported_root_object, MESSAGE_ID_FIELD) == 1)
                {
                    *message_id = (unsigned int)json_object_get_number(reported_root_object, MESSAGE_ID_FIELD);

                    *prop_count = *prop_count + 1;
                }

                result = 0;
            }
        }

        json_value_free(root_value);
    }

    return result;
}

static JSON_Value* create_message_as_json(const char* test_id, unsigned int message_id)
{
    JSON_Value* root_value;

    if ((root_value = json_value_init_object()) == NULL)
    {
        LogError("Failed creating root json value");
    }
    else
    {
        JSON_Object* root_object;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed creating root json object");
            json_value_free(root_value);
            root_value = NULL;
        }
        else if (json_object_set_string(root_object, MESSAGE_TEST_ID_FIELD, test_id) != JSONSuccess)
        {
            LogError("Failed setting test id");
            json_value_free(root_value);
            root_value = NULL;
        }
        else if (json_object_set_number(root_object, MESSAGE_ID_FIELD, (double)message_id) != JSONSuccess)
        {
            LogError("Failed setting message id");
            json_value_free(root_value);
            root_value = NULL;
        }
    }

    return root_value;
}

static char* create_message(const char* test_id, unsigned int message_id)
{
    char* result;
    JSON_Value* root_value;

    if ((root_value = create_message_as_json(test_id, message_id)) == NULL)
    {
        LogError("Failed creating root json value");
        result = NULL;
    }
    else
    {
        if ((result = json_serialize_to_string(root_value)) == NULL)
        {
            LogError("Failed serializing json to string");
        }

        json_value_free(root_value);
    }

    return result;
}

static IOTHUB_MESSAGE_HANDLE create_iothub_message(const char* test_id, unsigned int message_id)
{
    IOTHUB_MESSAGE_HANDLE result;
    char* msg_text;

    if ((msg_text = create_message(test_id, message_id)) == NULL)
    {
        LogError("Failed creating text for iothub message");
        result = NULL;
    }
    else
    {
        if ((result = IoTHubMessage_CreateFromString(msg_text)) == NULL)
        {
            LogError("Failed creating IOTHUB_MESSAGE_HANDLE");
        }

        free(msg_text);
    }

    return result;
}

static char* create_twin_desired_properties_update(const char* test_id, unsigned int message_id)
{
    char* result;

    JSON_Value* root_value;

    if ((root_value = json_value_init_object()) == NULL)
    {
        LogError("Failed creating root json value");
        result = NULL;
    }
    else
    {
        JSON_Object* root_object;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed creating root json object");
            result = NULL;
        }
        else
        {
            JSON_Value* message;

            if ((message = create_message_as_json(test_id, message_id)) == NULL)
            {
                LogError("Failed creating device twin update");
                result = NULL;
            }
            else if (json_object_dotset_value(root_object, "properties.desired", message) != JSONSuccess)
            {
                LogError("Failed setting desired properties");
                json_value_free(message);
                result = NULL;
            }
            else if ((result = json_serialize_to_string(root_value)) == NULL)
            {
                LogError("Failed serializing json to string");
            }
        }

        json_value_free(root_value);
    }

    return result;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)userContextCallback;

    if (iothub_client_statistics_add_connection_status(iotHubLonghaul->iotHubClientStats, status, reason) != 0)
    {
        LogError("Failed adding connection status statistics (%s, %s)",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, status), MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT on_c2d_message_received(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;

    if (message == NULL || userContextCallback == NULL)
    {
        LogError("Invalid argument (message=%p, userContextCallback=%p)", message, userContextCallback);
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        STRING_HANDLE parse_string;
        const unsigned char* data;
        size_t size;
        if (IoTHubMessage_GetByteArray(message, &data, &size) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failed getting string out of IOTHUB_MESSAGE_HANDLE");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else if ((parse_string = STRING_from_byte_array(data, size)) == NULL)
        {
            LogError("Failed constructing string from byte array");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)userContextCallback;
            unsigned int message_id;
            char tests_id[40];

            if (parse_message(STRING_c_str(parse_string), tests_id, &message_id) == 0 &&
                strcmp(tests_id, iotHubLonghaul->test_id) == 0)
            {
                C2D_MESSAGE_INFO info;
                info.message_id = message_id;
                info.time_received = time(NULL);

                if (info.time_received == INDEFINITE_TIME)
                {
                    LogError("Failed setting the receive time for c2d message %lu", (unsigned long)info.message_id);
                }

                if (iothub_client_statistics_add_c2d_info(iotHubLonghaul->iotHubClientStats, C2D_RECEIVED, &info) != 0)
                {
                    LogError("Failed adding receive info for c2d message %lu", (unsigned long)info.message_id);
                }

                result = IOTHUBMESSAGE_ACCEPTED;
            }
            else
            {
                result = IOTHUBMESSAGE_ABANDONED;
            }

            STRING_delete(parse_string);
        }
    }

    return result;
}

static int on_device_method_received(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    int result;
    STRING_HANDLE parse_string;

    if (method_name == NULL || payload == NULL || size == 0 || response == NULL || response_size == NULL || userContextCallback == NULL)
    {
        LogError("Invalid argument (method_name=%p, payload=%p, size=%zu, response=%p, response_size=%p, userContextCallback=%p)",
            method_name, payload, size, response, response_size, userContextCallback);
        result = -1;
    }
    else if (strcmp(method_name, LONGHAUL_DEVICE_METHOD_NAME) != 0)
    {
        LogError("Unexpected device method received (%s)", method_name);
        result = -1;
    }
    else if ((parse_string = STRING_from_byte_array(payload, size)) == NULL)
    {
        LogError("Failed constructing string from byte array");
        result = -1;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)userContextCallback;
        unsigned int method_id;
        char tests_id[40];

        if (parse_message(STRING_c_str(parse_string), tests_id, &method_id) == 0 &&
            strcmp(tests_id, iotHubLonghaul->test_id) == 0)
        {
            const char* default_response = "{ \"Response\": \"This is the response from the device\" }";

            DEVICE_METHOD_INFO info;
            info.method_id = method_id;
            info.time_received = time(NULL);

            if (info.time_received == INDEFINITE_TIME)
            {
                LogError("Failed setting the receive time for method %lu", (unsigned long)info.method_id);
            }

            if (iothub_client_statistics_add_device_method_info(iotHubLonghaul->iotHubClientStats, DEVICE_METHOD_RECEIVED, &info) != 0)
            {
                LogError("Failed adding receive info for method %lu", (unsigned long)info.method_id);
                result = -1;
            }
            else
            {
                result = 200;
            }

            if (mallocAndStrcpy_s((char**)response, default_response) != 0)
            {
                LogError("Failed setting device method response");
                *response_size = 0;
            }
            else
            {
                *response_size = strlen((const char*)*response);
            }
        }
        else
        {
            result = -1;
        }
        STRING_delete(parse_string);
    }

    return result;
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
        result = MU_FAILURE;
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
                result = MU_FAILURE;
                break;
            }
            else if (action(action_context) != 0)
            {
                LogError("Loop terminated by action function result");
                result = MU_FAILURE;
                break;
            }
            else if ((current_time = time(NULL)) == INDEFINITE_TIME)
            {
                LogError("Failed getting current time");
                result = MU_FAILURE;
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

        if (iotHubLonghaulRsrcs->iotHubSvcMsgHandle != NULL)
        {
            IoTHubMessaging_Close(iotHubLonghaulRsrcs->iotHubSvcMsgHandle);
            IoTHubMessaging_Destroy(iotHubLonghaulRsrcs->iotHubSvcMsgHandle);
        }

        if (iotHubLonghaulRsrcs->iotHubSvcDevTwinHandle != NULL)
        {
            IoTHubDeviceTwin_Destroy(iotHubLonghaulRsrcs->iotHubSvcDevTwinHandle);
        }

        if (iotHubLonghaulRsrcs->iotHubSvcDevMethodHandle != NULL)
        {
            IoTHubDeviceMethod_Destroy(iotHubLonghaulRsrcs->iotHubSvcDevMethodHandle);
        }

        if (iotHubLonghaulRsrcs->iotHubServiceClientHandle != NULL)
        {
            IoTHubServiceClientAuth_Destroy(iotHubLonghaulRsrcs->iotHubServiceClientHandle);
        }

        if (iotHubLonghaulRsrcs->iotHubClientHandle != NULL)
        {
            IoTHubClient_Destroy(iotHubLonghaulRsrcs->iotHubClientHandle);
        }

        if (iotHubLonghaulRsrcs->iotHubAccountInfo != NULL)
        {
            IoTHubAccount_deinit(iotHubLonghaulRsrcs->iotHubAccountInfo);
        }

        if (iotHubLonghaulRsrcs->iotHubClientStats != NULL)
        {
            iothub_client_statistics_destroy(iotHubLonghaulRsrcs->iotHubClientStats);
        }

        if (iotHubLonghaulRsrcs->test_id != NULL)
        {
            free(iotHubLonghaulRsrcs->test_id);
        }

        if (iotHubLonghaulRsrcs->lock != NULL)
        {
            Lock_Deinit(iotHubLonghaulRsrcs->lock);
        }

        platform_deinit();

        free((void*)handle);
    }
}

IOTHUB_LONGHAUL_RESOURCES_HANDLE longhaul_tests_init()
{
    IOTHUB_LONGHAUL_RESOURCES* result;
    UUID_T uuid;

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

            if ((result->test_id = UUID_to_string((const UUID_T*)&uuid)) == NULL)
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
            else if ((result->iotHubAccountInfo = IoTHubAccount_Init(false)) == NULL)
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
                LogInfo("Longhaul Test ID: %s", result->test_id);
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_HANDLE longhaul_initialize_device_client(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
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
        else if (IoTHubClient_SetMessageCallback(result, on_c2d_message_received, handle) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set the cloud-to-device message callback");
            IoTHubClient_Destroy(result);
            iotHubLonghaulRsrcs->iotHubClientHandle = NULL;
            result = NULL;
        }
        else if (IoTHubClient_SetDeviceMethodCallback(result, on_device_method_received, handle) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to set the device method callback");
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

static int on_message_received(void* context, const char* data, size_t size)
{
    int result;

    if (data == NULL || size == 0)
    {
        LogError("Invalid message received (data=%s, size=%lu)", data, (unsigned long)size);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
        unsigned int message_id;
        char tests_id[40];

        if (parse_message(data, tests_id, &message_id) == 0 &&
            strcmp(tests_id, iotHubLonghaul->test_id) == 0)
        {
            TELEMETRY_INFO info;
            info.message_id = message_id;
            info.time_received = time(NULL);

            if (info.time_received == INDEFINITE_TIME)
            {
                LogError("Failed setting the receive time for message %lu", (unsigned long)info.message_id);
            }

            if (iothub_client_statistics_add_telemetry_info(iotHubLonghaul->iotHubClientStats, TELEMETRY_RECEIVED, &info) != 0)
            {
                LogError("Failed adding receive info for message %lu", (unsigned long)info.message_id);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        else
        {
            result = MU_FAILURE; // This is not the message we expected. Abandoning it.
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
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubTestHandle != NULL)
        {
            LogError("IoTHubTest already initialized");
            result = MU_FAILURE;
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
                result = MU_FAILURE;
            }
            else
            {
                time_t time_start_range = add_seconds(time(NULL), SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS);

                if (time_start_range == INDEFINITE_TIME)
                {
                    LogError("Could not define the time start range");
                    IoTHubTest_Deinit(iotHubLonghaul->iotHubTestHandle);
                    iotHubLonghaul->iotHubTestHandle = NULL;
                    result = MU_FAILURE;
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
                    result = MU_FAILURE;
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
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubTestHandle == NULL)
        {
            LogError("IoTHubTest not initialized");
            result = MU_FAILURE;
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

static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE longhaul_initialize_service_client(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    if (iotHubLonghaul->iotHubServiceClientHandle == NULL)
    {
        const char* connection_string;

        if ((connection_string = IoTHubAccount_GetIoTHubConnString(iotHubLonghaul->iotHubAccountInfo)) == NULL)
        {
            LogError("Failed retrieving the IoT hub connection string");
        }
        else
        {
            iotHubLonghaul->iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string);
        }
    }

    return iotHubLonghaul->iotHubServiceClientHandle;
}

static void on_svc_client_c2d_messaging_open_complete(void* context)
{
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;

    if (Lock(iotHubLonghaul->lock) != LOCK_OK)
    {
        LogError("Failed locking");
    }
    else
    {
        iotHubLonghaul->is_svc_cl_c2d_msgr_open = true;

        if (Unlock(iotHubLonghaul->lock) != LOCK_OK)
        {
            LogError("Failed unlocking");
        }
    }
}

static IOTHUB_MESSAGING_CLIENT_HANDLE longhaul_initialize_service_c2d_messaging_client(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    IOTHUB_MESSAGING_CLIENT_HANDLE result;

    if (iotHubLonghaul->iotHubSvcMsgHandle != NULL)
    {
        LogError("IoT Hub Service C2D messaging already initialized");
        result = NULL;
    }
    else if ((iotHubLonghaul->iotHubSvcMsgHandle = IoTHubMessaging_Create(iotHubLonghaul->iotHubServiceClientHandle)) == NULL)
    {
        LogError("Failed creating the IoT Hub Service C2D messenger");
        result = NULL;
    }
    else if (IoTHubMessaging_Open(iotHubLonghaul->iotHubSvcMsgHandle, on_svc_client_c2d_messaging_open_complete, iotHubLonghaul) != IOTHUB_MESSAGING_OK)
    {
        LogError("Failed opening the IoT Hub Service C2D messenger");
        IoTHubMessaging_Destroy(iotHubLonghaul->iotHubSvcMsgHandle);
        iotHubLonghaul->iotHubSvcMsgHandle = NULL;
        result = NULL;
    }
    else
    {
        result = iotHubLonghaul->iotHubSvcMsgHandle;
    }

    return result;
}

static IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE longhaul_initialize_service_device_method_client(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE result;

    if (iotHubLonghaul->iotHubSvcDevMethodHandle != NULL)
    {
        LogError("IoT Hub Service device method client already initialized");
        result = NULL;
    }
    else if ((iotHubLonghaul->iotHubSvcDevMethodHandle = IoTHubDeviceMethod_Create(iotHubLonghaul->iotHubServiceClientHandle)) == NULL)
    {
        LogError("Failed creating the IoT Hub Service device method client");
        result = NULL;
    }
    else
    {
        result = iotHubLonghaul->iotHubSvcDevMethodHandle;
    }

    return result;
}

static IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE longhaul_initialize_service_device_twin_client(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE result;

    if (iotHubLonghaul->iotHubSvcDevTwinHandle != NULL)
    {
        LogError("IoT Hub Service device twin client already initialized");
        result = NULL;
    }
    else if ((iotHubLonghaul->iotHubSvcDevTwinHandle = IoTHubDeviceTwin_Create(iotHubLonghaul->iotHubServiceClientHandle)) == NULL)
    {
        LogError("Failed creating the IoT Hub Service device twin client");
        result = NULL;
    }
    else
    {
        result = iotHubLonghaul->iotHubSvcDevTwinHandle;
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

static SEND_TELEMETRY_CONTEXT* create_iothub_message_context(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
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
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE message;

        if ((message = create_iothub_message(longhaulResources->test_id, message_id)) == NULL)
        {
            LogError("Failed creating telemetry message");
            result = MU_FAILURE;
        }
        else
        {
            SEND_TELEMETRY_CONTEXT* message_info;

            if ((message_info = (SEND_TELEMETRY_CONTEXT*)malloc(sizeof(SEND_TELEMETRY_CONTEXT))) == NULL)
            {
                LogError("Failed allocating context for telemetry message");
                result = MU_FAILURE;
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
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }

                telemetry_info.send_result = result;

                if (iothub_client_statistics_add_telemetry_info(longhaulResources->iotHubClientStats, TELEMETRY_QUEUED, &telemetry_info) != 0)
                {
                    LogError("Failed adding telemetry statistics info (message_id=%d)", message_id);
                    result = MU_FAILURE;
                }
            }

            IoTHubMessage_Destroy(message);
        }
    }

    return result;
}

static void on_c2d_message_sent(void* context, IOTHUB_MESSAGING_RESULT messagingResult)
{
    if (context == NULL)
    {
        LogError("Invalid argument (%p, %s)", context, MU_ENUM_TO_STRING(IOTHUB_MESSAGING_RESULT, messagingResult));
    }
    else
    {
        SEND_C2D_CONTEXT* send_context = (SEND_C2D_CONTEXT*)context;

        C2D_MESSAGE_INFO info;
        info.message_id = send_context->message_id;
        info.send_callback_result = messagingResult;
        info.time_sent = time(NULL);

        if (info.time_sent == INDEFINITE_TIME)
        {
            LogError("Failed setting the send time for message %lu", (unsigned long)info.message_id);
        }

        if (iothub_client_statistics_add_c2d_info(send_context->iotHubLonghaul->iotHubClientStats, C2D_SENT, &info) != 0)
        {
            LogError("Failed adding send info for c2d message %lu", (unsigned long)info.message_id);
        }

        free(send_context);
    }
}

static int send_c2d(const void* context)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
    unsigned int message_id;

    if ((message_id = generate_unique_id(iotHubLonghaul)) == 0)
    {
        LogError("Failed generating c2d message id");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE message;

        if ((message = create_iothub_message(iotHubLonghaul->test_id, message_id)) == NULL)
        {
            LogError("Failed creating C2D message text");
            result = MU_FAILURE;
        }
        else
        {
            SEND_C2D_CONTEXT* send_context;

            if ((send_context = (SEND_C2D_CONTEXT*)malloc(sizeof(SEND_C2D_CONTEXT))) == NULL)
            {
                LogError("Failed allocating context for sending c2d message");
                result = MU_FAILURE;
            }
            else
            {
                send_context->message_id = message_id;
                send_context->iotHubLonghaul = iotHubLonghaul;

                if (IoTHubMessaging_SendAsync(iotHubLonghaul->iotHubSvcMsgHandle, iotHubLonghaul->deviceInfo->deviceId, message, on_c2d_message_sent, send_context) != IOTHUB_MESSAGING_OK)
                {
                    LogError("Failed sending c2d message");
                    free(send_context);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }

                {
                    C2D_MESSAGE_INFO c2d_msg_info;
                    c2d_msg_info.message_id = message_id;
                    c2d_msg_info.time_queued = time(NULL);
                    c2d_msg_info.send_result = result;

                    if (iothub_client_statistics_add_c2d_info(iotHubLonghaul->iotHubClientStats, C2D_QUEUED, &c2d_msg_info) != 0)
                    {
                        LogError("Failed adding c2d message statistics info (message_id=%d)", message_id);
                        result = MU_FAILURE;
                    }
                }
            }

            IoTHubMessage_Destroy(message);
        }
    }

    return result;
}

static int invoke_device_method(const void* context)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
    unsigned int method_id;

    if ((method_id = generate_unique_id(iotHubLonghaul)) == 0)
    {
        LogError("Failed generating device method id");
        result = MU_FAILURE;
    }
    else
    {
        char* message;

        if ((message = create_message(iotHubLonghaul->test_id, method_id)) == NULL)
        {
            LogError("Failed creating C2D message text");
            result = MU_FAILURE;
        }
        else
        {
            int responseStatus;
            unsigned char* responsePayload;
            size_t responseSize;

            DEVICE_METHOD_INFO device_method_info;
            device_method_info.method_id = method_id;
            device_method_info.time_invoked = time(NULL);

            if ((device_method_info.method_result = IoTHubDeviceMethod_Invoke(
                iotHubLonghaul->iotHubSvcDevMethodHandle,
                iotHubLonghaul->deviceInfo->deviceId,
                LONGHAUL_DEVICE_METHOD_NAME,
                message,
                MAX_DEVICE_METHOD_TRAVEL_TIME_SECS,
                &responseStatus, &responsePayload, &responseSize)) != IOTHUB_DEVICE_METHOD_OK)
            {
                LogError("Failed invoking device method");
            }

            if (iothub_client_statistics_add_device_method_info(iotHubLonghaul->iotHubClientStats, DEVICE_METHOD_INVOKED, &device_method_info) != 0)
            {
                LogError("Failed adding device method statistics info (method_id=%d)", method_id);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }

            free(message);
        }
    }

    return result;
}

static int get_twin_desired_version(const char* twin)
{
    int result;
    JSON_Value* root_value;

    if ((root_value = json_parse_string(twin)) == NULL)
    {
        LogError("Failed parsing twin document");
        result = -1;
    }
    else
    {
        JSON_Object* root_object;

        if ((root_object = json_value_get_object(root_value)) == NULL)
        {
            LogError("Failed getting json root object");
            result = -1;
        }
        else
        {
            result = (int)json_object_dotget_number(root_object, TWIN_PROPERTIES_BLOCK DOT TWIN_DESIRED_BLOCK DOT TWIN_FIELD_VERSION);
        }

        json_value_free(root_value);
    }


    return result;
}

static int update_device_twin_desired_property(const void* context)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
    unsigned int update_id;

    if ((update_id = generate_unique_id(iotHubLonghaul)) == 0)
    {
        LogError("Failed generating device method id");
        result = MU_FAILURE;
    }
    else
    {
        char* message;

        if ((message = create_twin_desired_properties_update(iotHubLonghaul->test_id, update_id)) == NULL)
        {
            LogError("Failed creating twin desired property update");
            result = MU_FAILURE;
        }
        else
        {
            if (Lock(iotHubLonghaul->lock) != LOCK_OK)
            {
                LogError("Failed locking (%s, %d)", iotHubLonghaul->test_id, update_id);
                result = MU_FAILURE;
            }
            else
            {
                char* update_response;
                DEVICE_TWIN_DESIRED_INFO device_twin_info;
                device_twin_info.update_id = update_id;
                device_twin_info.time_updated = time(NULL);

                if ((update_response = IoTHubDeviceTwin_UpdateTwin(
                    iotHubLonghaul->iotHubSvcDevTwinHandle,
                    iotHubLonghaul->deviceInfo->deviceId,
                    message)) == NULL)
                {
                    LogError("Failed sending twin desired properties update");
                    device_twin_info.update_result = -1;
                }
                else
                {
                    device_twin_info.update_result = get_twin_desired_version(update_response);
                    free(update_response);
                }

                if (iothub_client_statistics_add_device_twin_desired_info(iotHubLonghaul->iotHubClientStats, DEVICE_TWIN_UPDATE_SENT, &device_twin_info) != 0)
                {
                    LogError("Failed adding twin desired properties statistics info (update_id=%d)", update_id);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }

                if (Unlock(iotHubLonghaul->lock) != LOCK_OK)
                {
                    LogError("Failed unlocking (%s, %d)", iotHubLonghaul->test_id, update_id);
                }
            }

            free(message);
        }
    }

    return result;
}

static void on_twin_report_state_completed(int status_code, void* userContextCallback)
{
    if (userContextCallback == NULL)
    {
        LogError("Invalid argument (userContextCallback=NULL)");
    }
    else
    {
        SEND_TWIN_REPORTED_CONTEXT* send_context = (SEND_TWIN_REPORTED_CONTEXT*)userContextCallback;
        DEVICE_TWIN_REPORTED_INFO device_twin_info;
        device_twin_info.update_id = send_context->update_id;
        device_twin_info.time_sent = time(NULL);
        device_twin_info.send_status_code = status_code;

        if (iothub_client_statistics_add_device_twin_reported_info(send_context->iotHubLonghaul->iotHubClientStats, DEVICE_TWIN_UPDATE_SENT, &device_twin_info) != 0)
        {
            LogError("Failed adding device twin reported properties statistics info (update_id=%d)", send_context->update_id);
        }

        free(send_context);
    }
}

static void check_for_reported_properties_update_on_service_side(IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul)
{
    char* twin;

    if ((twin = IoTHubDeviceTwin_GetTwin(
        iotHubLonghaul->iotHubSvcDevTwinHandle,
        iotHubLonghaul->deviceInfo->deviceId)) == NULL)
    {
        LogError("Failed getting the twin document on service side");
    }
    else
    {
        int property_count;
        unsigned int message_id;
        char test_id[40];
        int version;

        if (parse_twin_reported_properties(twin, &property_count, test_id, &message_id, &version) != 0)
        {
            LogError("Failed parsing the device twin reported properties");
        }
        else if (property_count == 2) //i.e., if not empty.
        {
            if (strcmp(test_id, iotHubLonghaul->test_id) == 0)
            {
                DEVICE_TWIN_REPORTED_INFO info;
                info.update_id = message_id;
                info.time_received = time(NULL);

                if (info.time_received == INDEFINITE_TIME)
                {
                    LogError("Failed setting the receive time for twin update %lu", (unsigned long)info.update_id);
                }

                if (iothub_client_statistics_add_device_twin_reported_info(iotHubLonghaul->iotHubClientStats, DEVICE_TWIN_UPDATE_RECEIVED, &info) != 0)
                {
                    LogError("Failed adding receive info for twin update %lu", (unsigned long)info.update_id);
                }
            }
        }
    }
}

static int update_device_twin_reported_property(const void* context)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)context;
    unsigned int update_id;

    check_for_reported_properties_update_on_service_side(iotHubLonghaul);

    if ((update_id = generate_unique_id(iotHubLonghaul)) == 0)
    {
        LogError("Failed generating device method id");
        result = MU_FAILURE;
    }
    else
    {
        SEND_TWIN_REPORTED_CONTEXT* send_context;

        if ((send_context = malloc(sizeof(SEND_TWIN_REPORTED_CONTEXT))) == NULL)
        {
            LogError("Failed allocating context for sending twin reported update");
            result = MU_FAILURE;
        }
        else
        {
            char* message;

            send_context->update_id = update_id;
            send_context->iotHubLonghaul = iotHubLonghaul;

            if ((message = create_message(iotHubLonghaul->test_id, update_id)) == NULL)
            {
                LogError("Failed creating twin reported property update");
                free(send_context);
                result = MU_FAILURE;
            }
            else
            {
                DEVICE_TWIN_REPORTED_INFO device_twin_info;
                device_twin_info.update_id = update_id;
                device_twin_info.time_queued = time(NULL);

                if ((device_twin_info.update_result = IoTHubClient_SendReportedState(iotHubLonghaul->iotHubClientHandle, (const unsigned char*)message, strlen(message), on_twin_report_state_completed, send_context)) != IOTHUB_CLIENT_OK)
                {
                    LogError("Failed sending twin reported properties update");
                    free(send_context);
                }

                if (iothub_client_statistics_add_device_twin_reported_info(iotHubLonghaul->iotHubClientStats, DEVICE_TWIN_UPDATE_QUEUED, &device_twin_info) != 0)
                {
                    LogError("Failed adding device twin reported properties statistics info (update_id=%d)", update_id);
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }

                free(message);
            }
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
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaulRsrcs = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaulRsrcs->iotHubClientHandle == NULL || iotHubLonghaulRsrcs->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = MU_FAILURE;
        }
        else
        {
            if (longhaul_start_listening_for_telemetry_messages(handle, iotHubLonghaulRsrcs->deviceInfo) != 0)
            {
                LogError("Failed listening for telemetry messages");
                result = MU_FAILURE;
            }
            else
            {
                int loop_result;
                IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

                loop_result = run_on_loop(send_telemetry, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaulRsrcs);

                ThreadAPI_Sleep((unsigned int)iterationDurationInSeconds * 1000 * 10); // Extra time for the last messages.

                stats_handle = longhaul_get_statistics(iotHubLonghaulRsrcs);

                LogInfo("Longhaul telemetry stats: %s", iothub_client_statistics_to_json(stats_handle));

                if (loop_result != 0)
                {
                    result = MU_FAILURE;
                }
                else
                {
                    IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY summary;

                    if (iothub_client_statistics_get_telemetry_summary(stats_handle, &summary) != 0)
                    {
                        LogError("Failed gettting statistics summary");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        LogInfo("Summary: Messages sent=%lu, received=%lu; travel time: min=%f secs, max=%f secs",
                            (unsigned long)summary.messages_sent, (unsigned long)summary.messages_received, summary.min_travel_time_secs, summary.max_travel_time_secs);

                        if (summary.messages_sent == 0 || summary.messages_received != summary.messages_sent || summary.max_travel_time_secs > MAX_TELEMETRY_TRAVEL_TIME_SECS)
                        {
                            result = MU_FAILURE;
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
    int result;

    if (handle == NULL)
    {
        LogError("Invalig argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubClientHandle == NULL || iotHubLonghaul->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_client(iotHubLonghaul) == NULL)
        {
            LogError("Cannot send C2D messages, failed to initialize IoT hub service client");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_c2d_messaging_client(iotHubLonghaul) == NULL)
        {
            LogError("Cannot send C2D messages, failed to initialize IoT hub service client c2d messenger");
            result = MU_FAILURE;
        }
        else
        {
            int loop_result;
            IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

            loop_result = run_on_loop(send_c2d, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaul);

            ThreadAPI_Sleep((unsigned int)iterationDurationInSeconds * 1000 * 10); // Extra time for the last messages.

            stats_handle = longhaul_get_statistics(iotHubLonghaul);

            LogInfo("Longhaul Cloud-to-Device stats: %s", iothub_client_statistics_to_json(stats_handle));

            if (loop_result != 0)
            {
                result = MU_FAILURE;
            }
            else
            {
                IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY summary;

                if (iothub_client_statistics_get_c2d_summary(stats_handle, &summary) != 0)
                {
                    LogError("Failed gettting statistics summary");
                    result = MU_FAILURE;
                }
                else
                {
                    LogInfo("Summary: Messages sent=%lu, received=%lu; travel time: min=%f secs, max=%f secs",
                        (unsigned long)summary.messages_sent, (unsigned long)summary.messages_received, summary.min_travel_time_secs, summary.max_travel_time_secs);

                    if (summary.messages_sent == 0 || summary.messages_received != summary.messages_sent || summary.max_travel_time_secs > MAX_C2D_TRAVEL_TIME_SECS)
                    {
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }

    return result;
}

int longhaul_run_device_methods_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalig argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubClientHandle == NULL || iotHubLonghaul->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_client(iotHubLonghaul) == NULL)
        {
            LogError("Cannot invoke device methods, failed to initialize IoT hub service client");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_device_method_client(iotHubLonghaul) == NULL)
        {
            LogError("Cannot invoke device methods, failed to initialize IoT hub service device method client");
            result = MU_FAILURE;
        }
        else
        {
            int loop_result;
            IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

            // Wait for the service to ack the device subscription...
            ThreadAPI_Sleep(DEVICE_METHOD_SUB_WAIT_TIME_MS);

            loop_result = run_on_loop(invoke_device_method, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaul);

            stats_handle = longhaul_get_statistics(iotHubLonghaul);

            LogInfo("Longhaul Device Methods stats: %s", iothub_client_statistics_to_json(stats_handle));

            if (loop_result != 0)
            {
                result = MU_FAILURE;
            }
            else
            {
                IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY summary;

                if (iothub_client_statistics_get_device_method_summary(stats_handle, &summary) != 0)
                {
                    LogError("Failed gettting statistics summary");
                    result = MU_FAILURE;
                }
                else
                {
                    LogInfo("Summary: Methods invoked=%lu, received=%lu; travel time: min=%f secs, max=%f secs",
                        (unsigned long)summary.methods_invoked, (unsigned long)summary.methods_received, summary.min_travel_time_secs, summary.max_travel_time_secs);

                    if (summary.methods_invoked == 0 || summary.methods_received != summary.methods_invoked || summary.max_travel_time_secs > MAX_DEVICE_METHOD_TRAVEL_TIME_SECS)
                    {
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }

    return result;
}

static void on_device_twin_update_received(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    if (payLoad == NULL || userContextCallback == NULL)
    {
        LogError("Invalid argument (payLoad=%p, userContextCallback=%p)", payLoad, userContextCallback);
    }
    else
    {
        STRING_HANDLE parse_string;
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)userContextCallback;
        unsigned int message_id = 0;
        char tests_id[40];
        int version;

        if ((parse_string = STRING_from_byte_array(payLoad, size)) == NULL)
        {
            LogError("Failed constructing string from byte array");
        }
        else
        {
            if (update_state == DEVICE_TWIN_UPDATE_COMPLETE &&
                parse_twin_desired_properties(STRING_c_str(parse_string), tests_id, &message_id, &version) != 0)
            {
                LogError("Failed parsing complete twin update data");
            }
            else if (update_state == DEVICE_TWIN_UPDATE_PARTIAL &&
                parse_message(STRING_c_str(parse_string), tests_id, &message_id) != 0)
            {
                LogError("Failed parsing twin update data");
            }
            else if (strcmp(tests_id, iotHubLonghaul->test_id) == 0)
            {
                DEVICE_TWIN_DESIRED_INFO info;
                info.update_id = message_id;
                info.time_received = time(NULL);

                if (info.time_received == INDEFINITE_TIME)
                {
                    LogError("Failed setting the receive time for twin update %lu", (unsigned long)info.update_id);
                }

                if (Lock(iotHubLonghaul->lock) != LOCK_OK)
                {
                    LogError("Failed locking (%s, %d)", iotHubLonghaul->test_id, message_id);
                }
                else
                {
                    if (iothub_client_statistics_add_device_twin_desired_info(iotHubLonghaul->iotHubClientStats, DEVICE_TWIN_UPDATE_RECEIVED, &info) != 0)
                    {
                        LogError("Failed adding receive info for twin update %lu", (unsigned long)info.update_id);
                    }

                    if (Unlock(iotHubLonghaul->lock) != LOCK_OK)
                    {
                        LogError("Failed unlocking (%s, %d)", iotHubLonghaul->test_id, message_id);
                    }
                }
            }
            STRING_delete(parse_string);
        }
    }
}

int longhaul_run_twin_desired_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalig argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubClientHandle == NULL || iotHubLonghaul->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = MU_FAILURE;
        }
        else if (IoTHubClient_SetDeviceTwinCallback(iotHubLonghaul->iotHubClientHandle, on_device_twin_update_received, iotHubLonghaul) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed subscribing device client for twin desired properties updates");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_client(iotHubLonghaul) == NULL)
        {
            LogError("Failed to initialize IoT hub service client");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_device_twin_client(iotHubLonghaul) == NULL)
        {
            LogError("Failed to initialize IoT hub service device twin client");
            result = MU_FAILURE;
        }
        else
        {
            int loop_result;
            IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

            loop_result = run_on_loop(update_device_twin_desired_property, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaul);

            stats_handle = longhaul_get_statistics(iotHubLonghaul);

            LogInfo("Longhaul Device Twin Desired Properties stats: %s", iothub_client_statistics_to_json(stats_handle));

            if (loop_result != 0)
            {
                result = MU_FAILURE;
            }
            else
            {
                IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY summary;

                if (iothub_client_statistics_get_device_twin_desired_summary(stats_handle, &summary) != 0)
                {
                    LogError("Failed gettting statistics summary");
                    result = MU_FAILURE;
                }
                else
                {
                    LogInfo("Summary: Updates sent=%lu, received=%lu; travel time: min=%f secs, max=%f secs",
                        (unsigned long)summary.updates_sent, (unsigned long)summary.updates_received, summary.min_travel_time_secs, summary.max_travel_time_secs);

                    if (summary.updates_sent == 0 || summary.updates_received != summary.updates_sent || summary.max_travel_time_secs > MAX_TWIN_DESIRED_PROP_TRAVEL_TIME_SECS)
                    {
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }

    return result;
}

int longhaul_run_twin_reported_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, size_t iterationDurationInSeconds, size_t totalDurationInSeconds)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalig argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_LONGHAUL_RESOURCES* iotHubLonghaul = (IOTHUB_LONGHAUL_RESOURCES*)handle;

        if (iotHubLonghaul->iotHubClientHandle == NULL || iotHubLonghaul->deviceInfo == NULL)
        {
            LogError("IoTHubClient not initialized.");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_client(iotHubLonghaul) == NULL)
        {
            LogError("Failed to initialize IoT hub service client");
            result = MU_FAILURE;
        }
        else if (longhaul_initialize_service_device_twin_client(iotHubLonghaul) == NULL)
        {
            LogError("Failed to initialize IoT hub service device twin client");
            result = MU_FAILURE;
        }
        else
        {
            int loop_result;
            IOTHUB_CLIENT_STATISTICS_HANDLE stats_handle;

            loop_result = run_on_loop(update_device_twin_reported_property, iterationDurationInSeconds, totalDurationInSeconds, iotHubLonghaul);

            // One last check...
            ThreadAPI_Sleep((unsigned int)iterationDurationInSeconds * 1000);
            check_for_reported_properties_update_on_service_side(iotHubLonghaul);

            stats_handle = longhaul_get_statistics(iotHubLonghaul);

            LogInfo("Longhaul Device Twin Reported Properties stats: %s", iothub_client_statistics_to_json(stats_handle));

            if (loop_result != 0)
            {
                result = MU_FAILURE;
            }
            else
            {
                IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY summary;

                if (iothub_client_statistics_get_device_twin_reported_summary(stats_handle, &summary) != 0)
                {
                    LogError("Failed gettting statistics summary");
                    result = MU_FAILURE;
                }
                else
                {
                    LogInfo("Summary: Updates sent=%lu, received=%lu; travel time: min=%f secs, max=%f secs",
                        (unsigned long)summary.updates_sent, (unsigned long)summary.updates_received, summary.min_travel_time_secs, summary.max_travel_time_secs);

                    if (summary.updates_sent == 0 || summary.updates_received != summary.updates_sent || summary.max_travel_time_secs > MAX_TWIN_REPORTED_PROP_TRAVEL_TIME_SECS)
                    {
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }

    return result;
}
