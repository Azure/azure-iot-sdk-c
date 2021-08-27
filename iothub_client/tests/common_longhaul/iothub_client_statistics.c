// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include <limits.h>
#include "iothub_client_statistics.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"

#define INDEFINITE_TIME ((time_t)-1)
#define SPAN_3_MINUTES_IN_SECONDS (60 * 3)

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TELEMETRY_EVENT_TYPE, TELEMETRY_EVENT_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(C2D_EVENT_TYPE, C2D_EVENT_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_METHOD_EVENT_TYPE, DEVICE_METHOD_EVENT_TYPE_VALUES)
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(DEVICE_TWIN_EVENT_TYPE, DEVICE_TWIN_EVENT_TYPE_VALUES)


typedef struct CONNECTION_STATUS_INFO_TAG
{
    time_t time;
    IOTHUB_CLIENT_CONNECTION_STATUS status;
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason;
} CONNECTION_STATUS_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_TAG
{
    SINGLYLINKEDLIST_HANDLE connection_status_history;
    SINGLYLINKEDLIST_HANDLE telemetry_events;
    SINGLYLINKEDLIST_HANDLE c2d_messages;
    SINGLYLINKEDLIST_HANDLE device_methods;
    SINGLYLINKEDLIST_HANDLE twin_reported_properties;
    SINGLYLINKEDLIST_HANDLE twin_desired_properties;
} IOTHUB_CLIENT_STATISTICS;

static bool destroy_connection_status_info(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_telemetry_event(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_c2d_message_info(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_device_method_info(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_twin_reported_property_info(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_twin_desired_property_info(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    free((void*)item);
    *continue_processing = true;
    return true;
}

void iothub_client_statistics_destroy(IOTHUB_CLIENT_STATISTICS_HANDLE handle)
{
    if (handle != NULL)
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;

        if (stats->connection_status_history != NULL)
        {
            if (singlylinkedlist_remove_if(stats->connection_status_history, destroy_connection_status_info, NULL) != 0)
            {
                LogError("Failed releasing connection status history items");
            }
            singlylinkedlist_destroy(stats->connection_status_history);
        }

        if (stats->telemetry_events != NULL)
        {
            if (singlylinkedlist_remove_if(stats->telemetry_events, destroy_telemetry_event, NULL) != 0)
            {
                LogError("Failed releasing telemetry events");
            }
            singlylinkedlist_destroy(stats->telemetry_events);
        }

        if (stats->c2d_messages != NULL)
        {
            if (singlylinkedlist_remove_if(stats->c2d_messages, destroy_c2d_message_info, NULL) != 0)
            {
                LogError("Failed releasing c2d message events");
            }
            singlylinkedlist_destroy(stats->c2d_messages);
        }

        if (stats->device_methods != NULL)
        {
            if (singlylinkedlist_remove_if(stats->device_methods, destroy_device_method_info, NULL) != 0)
            {
                LogError("Failed releasing device method events");
            }
            singlylinkedlist_destroy(stats->device_methods);
        }

        if (stats->twin_reported_properties != NULL)
        {
            if (singlylinkedlist_remove_if(stats->twin_reported_properties, destroy_twin_reported_property_info, NULL) != 0)
            {
                LogError("Failed releasing twin reported property event");
            }
            singlylinkedlist_destroy(stats->twin_reported_properties);
        }

        if (stats->twin_desired_properties != NULL)
        {
            if (singlylinkedlist_remove_if(stats->twin_desired_properties, destroy_twin_desired_property_info, NULL) != 0)
            {
                LogError("Failed releasing twin desired property event");
            }
            singlylinkedlist_destroy(stats->twin_desired_properties);
        }

        free(handle);
    }
}

IOTHUB_CLIENT_STATISTICS_HANDLE iothub_client_statistics_create(void)
{
    IOTHUB_CLIENT_STATISTICS* stats;

    if ((stats = ((IOTHUB_CLIENT_STATISTICS*)malloc(sizeof(IOTHUB_CLIENT_STATISTICS)))) == NULL)
    {
        LogError("Failed allocating IOTHUB_CLIENT_STATISTICS");
    }
    else
    {
        memset(stats, 0, sizeof(IOTHUB_CLIENT_STATISTICS));

        if ((stats->connection_status_history = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for connection status info");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
        else if ((stats->telemetry_events = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for telemetry events");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
        else if ((stats->c2d_messages = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for c2d messages");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
        else if ((stats->device_methods = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for device methods");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
        else if ((stats->twin_reported_properties = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for twin reported properties");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
        else if ((stats->twin_desired_properties = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed creating list for twin desired properties");
            iothub_client_statistics_destroy(stats);
            stats = NULL;
        }
    }

    return stats;
}

static void serialize_connection_status(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* conn_status_array = json_value_get_array((const JSON_Value*)action_context);

    if (conn_status_array == NULL)
    {
        LogError("Failed to retrieve the connection status json array");
    }
    else
    {
        CONNECTION_STATUS_INFO* info = (CONNECTION_STATUS_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating connection status json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_string(info_json_obj, "time", (info->time == INDEFINITE_TIME ? "undefined" : ctime((&info->time)))) != JSONSuccess)
                {
                    LogError("Failed serializing connection status time");
                    json_value_free(info_json);
                }
                else if (json_object_set_string(info_json_obj, "status", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, info->status)) != JSONSuccess)
                {
                    LogError("Failed serializing connection status");
                    json_value_free(info_json);
                }
                else if (json_object_set_string(info_json_obj, "reason", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, info->reason)) != JSONSuccess)
                {
                    LogError("Failed serializing connection status reason");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(conn_status_array, info_json) != 0)
                {
                    LogError("Failed appending connection status json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

static void serialize_telemetry_event(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* telemetry_array = json_value_get_array((const JSON_Value*)action_context);

    if (telemetry_array == NULL)
    {
        LogError("Failed to retrieve the telemetry events json array");
    }
    else
    {
        TELEMETRY_INFO* info = (TELEMETRY_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating telemetry event json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_number(info_json_obj, "id", (double)info->message_id) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event id");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "enqueue.time", (info->time_queued == INDEFINITE_TIME ? "undefined" : ctime(&info->time_queued))) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event enqueue time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_number(info_json_obj, "enqueue.result", (double)info->send_result) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event enqueue result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.time", (info->time_sent == INDEFINITE_TIME ? "undefined" : ctime(&info->time_sent))) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event send time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.result", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, info->send_callback_result)) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "receive.time", (info->time_received == INDEFINITE_TIME ? "undefined" : ctime(&info->time_received))) != JSONSuccess)
                {
                    LogError("Failed serializing telemetry event receive time");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(telemetry_array, info_json) != 0)
                {
                    LogError("Failed appending telemetry event json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

static void serialize_c2d_event(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* c2d_array = json_value_get_array((const JSON_Value*)action_context);

    if (c2d_array == NULL)
    {
        LogError("Failed to retrieve the c2d events json array");
    }
    else
    {
        C2D_MESSAGE_INFO* info = (C2D_MESSAGE_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating c2d event json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_number(info_json_obj, "id", (double)info->message_id) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event id");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "enqueue.time", (info->time_queued == INDEFINITE_TIME ? "undefined" : ctime(&info->time_queued))) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event enqueue time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_number(info_json_obj, "enqueue.result", (double)info->send_result) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event enqueue result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.time", (info->time_sent == INDEFINITE_TIME ? "undefined" : ctime(&info->time_sent))) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event send time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.result", MU_ENUM_TO_STRING(IOTHUB_MESSAGING_RESULT, info->send_callback_result)) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "receive.time", (info->time_received == INDEFINITE_TIME ? "undefined" : ctime(&info->time_received))) != JSONSuccess)
                {
                    LogError("Failed serializing c2d event receive time");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(c2d_array, info_json) != 0)
                {
                    LogError("Failed appending c2d event json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

static void serialize_device_method_event(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* c2d_array = json_value_get_array((const JSON_Value*)action_context);

    if (c2d_array == NULL)
    {
        LogError("Failed to retrieve the device method events json array");
    }
    else
    {
        DEVICE_METHOD_INFO* info = (DEVICE_METHOD_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating device method event json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_number(info_json_obj, "id", (double)info->method_id) != JSONSuccess)
                {
                    LogError("Failed serializing device method event id");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.time", (info->time_invoked == INDEFINITE_TIME ? "undefined" : ctime(&info->time_invoked))) != JSONSuccess)
                {
                    LogError("Failed serializing device method event send time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.result", MU_ENUM_TO_STRING(IOTHUB_DEVICE_METHOD_RESULT, info->method_result)) != JSONSuccess)
                {
                    LogError("Failed serializing device method event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "receive.time", (info->time_received == INDEFINITE_TIME ? "undefined" : ctime(&info->time_received))) != JSONSuccess)
                {
                    LogError("Failed serializing device method event receive time");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(c2d_array, info_json) != 0)
                {
                    LogError("Failed appending device method event json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

static void serialize_device_twin_reported_event(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* device_twin_array = json_value_get_array((const JSON_Value*)action_context);

    if (device_twin_array == NULL)
    {
        LogError("Failed to retrieve the device twin events json array");
    }
    else
    {
        DEVICE_TWIN_REPORTED_INFO* info = (DEVICE_TWIN_REPORTED_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating device twin event json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_number(info_json_obj, "id", (double)info->update_id) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event id");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "enqueue.time", (info->time_queued == INDEFINITE_TIME ? "undefined" : ctime(&info->time_queued))) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event enqueue time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "enqueue.result", MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, info->update_result)) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.time", (info->time_sent == INDEFINITE_TIME ? "undefined" : ctime(&info->time_sent))) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event send time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_number(info_json_obj, "send.result", info->send_status_code) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "receive.time", (info->time_received == INDEFINITE_TIME ? "undefined" : ctime(&info->time_received))) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event receive time");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(device_twin_array, info_json) != 0)
                {
                    LogError("Failed appending device twin event json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

static void serialize_device_twin_desired_event(const void* item, const void* action_context, bool* continue_processing)
{
    JSON_Array* device_twin_array = json_value_get_array((const JSON_Value*)action_context);

    if (device_twin_array == NULL)
    {
        LogError("Failed to retrieve the device twin events json array");
    }
    else
    {
        DEVICE_TWIN_DESIRED_INFO* info = (DEVICE_TWIN_DESIRED_INFO*)item;
        JSON_Value* info_json;

        if ((info_json = json_value_init_object()) == NULL)
        {
            LogError("Failed creating device twin event json");
        }
        else
        {
            JSON_Object* info_json_obj;

            if ((info_json_obj = json_value_get_object(info_json)) == NULL)
            {
                LogError("Failed getting json object");
                json_value_free(info_json);
            }
            else
            {
                if (json_object_set_number(info_json_obj, "id", (double)info->update_id) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event id");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "send.time", (info->time_updated == INDEFINITE_TIME ? "undefined" : ctime(&info->time_updated))) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event send time");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_number(info_json_obj, "send.result", info->update_result) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event send result");
                    json_value_free(info_json);
                }
                else if (json_object_dotset_string(info_json_obj, "receive.time", (info->time_received == INDEFINITE_TIME ? "undefined" : ctime(&info->time_received))) != JSONSuccess)
                {
                    LogError("Failed serializing device twin event receive time");
                    json_value_free(info_json);
                }
                else if (json_array_append_value(device_twin_array, info_json) != 0)
                {
                    LogError("Failed appending device twin event json");
                    json_value_free(info_json);
                }
            }
        }
    }

    *continue_processing = true;
}

char* iothub_client_statistics_to_json(IOTHUB_CLIENT_STATISTICS_HANDLE handle)
{
    char* result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS* stats = (IOTHUB_CLIENT_STATISTICS*)handle;
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
                JSON_Value* conn_status_array;
                JSON_Value* telemetry_array;
                JSON_Value* c2d_array;
                JSON_Value* methods_array;
                JSON_Value* twin_desired_array;
                JSON_Value* twin_reported_array;

                // Connection Status
                if ((conn_status_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for connection status");
                }
                else if (singlylinkedlist_foreach(stats->connection_status_history, serialize_connection_status, conn_status_array) != 0)
                {
                    LogError("Failed adding connection status instances to json array");
                }
                else if ((json_object_set_value(root_object, "connection status", conn_status_array)) != JSONSuccess)
                {
                    LogError("Failed adding connection status array to json object");
                }

                // Telemetry
                if ((telemetry_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for telemetry events");
                }
                else if (singlylinkedlist_foreach(stats->telemetry_events, serialize_telemetry_event, telemetry_array) != 0)
                {
                    LogError("Failed adding telemetry event to json array");
                }
                else if ((json_object_set_value(root_object, "telemetry", telemetry_array)) != JSONSuccess)
                {
                    LogError("Failed adding telemetry events array to json object");
                }

                // C2D
                if ((c2d_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for c2d events");
                }
                else if (singlylinkedlist_foreach(stats->c2d_messages, serialize_c2d_event, c2d_array) != 0)
                {
                    LogError("Failed adding c2d event to json array");
                }
                else if ((json_object_set_value(root_object, "c2d", c2d_array)) != JSONSuccess)
                {
                    LogError("Failed adding c2d events array to json object");
                }

                // Device Methods
                if ((methods_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for device method events");
                }
                else if (singlylinkedlist_foreach(stats->device_methods, serialize_device_method_event, methods_array) != 0)
                {
                    LogError("Failed adding device method event to json array");
                }
                else if ((json_object_set_value(root_object, "device methods", methods_array)) != JSONSuccess)
                {
                    LogError("Failed adding device method events array to json object");
                }

                // Device Twin Desired Properties
                if ((twin_desired_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for device twin desired properties events");
                }
                else if (singlylinkedlist_foreach(stats->twin_desired_properties, serialize_device_twin_desired_event, twin_desired_array) != 0)
                {
                    LogError("Failed adding device twin event to json array");
                }
                else if ((json_object_dotset_value(root_object, "device twin.desired properties", twin_desired_array)) != JSONSuccess)
                {
                    LogError("Failed adding device twin desired properties events array to json object");
                }

                // Device Twin Reported Properties
                if ((twin_reported_array = json_value_init_array()) == NULL)
                {
                    LogError("Failed creating json array for device twin reported properties events");
                }
                else if (singlylinkedlist_foreach(stats->twin_reported_properties, serialize_device_twin_reported_event, twin_reported_array) != 0)
                {
                    LogError("Failed adding device twin event to json array");
                }
                else if ((json_object_dotset_value(root_object, "device twin.reported properties", twin_reported_array)) != JSONSuccess)
                {
                    LogError("Failed adding device twin reported properties events array to json object");
                }

                if ((result = json_serialize_to_string_pretty(root_value)) == NULL)
                {
                    LogError("Failed serializing json to string");
                }
            }

            json_value_free(root_value);
        }

    }

    return result;
}

int iothub_client_statistics_add_connection_status(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid argument (handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        CONNECTION_STATUS_INFO* conn_status;

        if ((conn_status = (CONNECTION_STATUS_INFO*)malloc(sizeof(CONNECTION_STATUS_INFO))) == NULL)
        {
            LogError("Failed allocating CONNECTION_STATUS_INFO");
            result = MU_FAILURE;
        }
        else
        {
            IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;

            if (singlylinkedlist_add(stats->connection_status_history, conn_status) == NULL)
            {
                LogError("Failed adding CONNECTION_STATUS_INFO");
                result = MU_FAILURE;
            }
            else
            {
                conn_status->status = status;
                conn_status->reason = reason;

                if ((conn_status->time = time(NULL)) == INDEFINITE_TIME)
                {
                    LogError("Failed setting the connection status info time");
                }

                result = 0;
            }
        }
    }

    return result;
}

static bool find_telemetry_info_by_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    TELEMETRY_INFO* match_info = (TELEMETRY_INFO*)match_context;
    TELEMETRY_INFO* item_info = (TELEMETRY_INFO*)singlylinkedlist_item_get_value(list_item);

    return (item_info->message_id == match_info->message_id);
}

int iothub_client_statistics_add_telemetry_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, TELEMETRY_EVENT_TYPE type, TELEMETRY_INFO* info)
{
    int result;

    if (handle == NULL || info == NULL || (type != TELEMETRY_QUEUED && type != TELEMETRY_SENT && type != TELEMETRY_RECEIVED))
    {
        LogError("Invalid argument (handle=%p, type=%s, info=%p)", handle, MU_ENUM_TO_STRING(TELEMETRY_EVENT_TYPE, type), info);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        TELEMETRY_INFO* queued_info;
        LIST_ITEM_HANDLE list_item = singlylinkedlist_find(stats->telemetry_events, find_telemetry_info_by_id, info);

        if (list_item == NULL)
        {
            if (type != TELEMETRY_QUEUED)
            {
                LogError("Telemetry info not found for message %lu (%d)", (unsigned long)info->message_id, (int)type);
                result = MU_FAILURE;
            }
            else if ((queued_info = (TELEMETRY_INFO*)malloc(sizeof(TELEMETRY_INFO))) == NULL)
            {
                LogError("Failed cloning the TELEMETRY_INFO");
                result = MU_FAILURE;
            }
            else if (singlylinkedlist_add(stats->telemetry_events, queued_info) == NULL)
            {
                LogError("Failed adding telemetry info (message id: %lu)", (unsigned long)info->message_id);
                free(queued_info);
                result = MU_FAILURE;
            }
            else
            {
                queued_info->message_id = info->message_id;
                queued_info->time_queued = info->time_queued;
                queued_info->send_result = info->send_result;
                queued_info->time_sent = INDEFINITE_TIME;
                queued_info->send_callback_result = IOTHUB_CLIENT_CONFIRMATION_ERROR;
                queued_info->time_received = INDEFINITE_TIME;

                result = 0;
            }
        }
        else
        {
            if ((queued_info = (TELEMETRY_INFO*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("Failed retrieving queued telemetry info (message id: %lu)", (unsigned long)info->message_id);
                result = MU_FAILURE;
            }
            else
            {
                if (type == TELEMETRY_SENT)
                {
                    queued_info->time_sent = info->time_sent;
                    queued_info->send_callback_result = info->send_callback_result;
                }
                else if (type == TELEMETRY_RECEIVED)
                {
                    queued_info->time_received = info->time_received;
                }

                result = 0;
            }
        }
    }

    return result;
}

int iothub_client_statistics_get_telemetry_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY* summary)
{
    int result;

    if (handle == NULL || summary == NULL)
    {
        LogError("Invalid argument (handle=%p, summary=%p)", handle, summary);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        LIST_ITEM_HANDLE list_item;

        (void)memset(summary, 0, sizeof(IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY));
        summary->min_travel_time_secs = LONG_MAX;

        list_item = singlylinkedlist_get_head_item(stats->telemetry_events);

        while (list_item != NULL)
        {
            TELEMETRY_INFO* telemetry_info = (TELEMETRY_INFO*)singlylinkedlist_item_get_value(list_item);

            summary->messages_sent = summary->messages_sent + 1;

            if (telemetry_info->time_received != INDEFINITE_TIME)
            {
                double travel_time = difftime(telemetry_info->time_received, telemetry_info->time_sent);

                if (travel_time < summary->min_travel_time_secs)
                {
                    summary->min_travel_time_secs = travel_time;
                }

                if (travel_time > summary->max_travel_time_secs)
                {
                    summary->max_travel_time_secs = travel_time;
                }

                summary->messages_received = summary->messages_received + 1;
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }

        result = 0;
    }

    return result;
}

static bool find_c2d_message_info_by_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    C2D_MESSAGE_INFO* match_info = (C2D_MESSAGE_INFO*)match_context;
    C2D_MESSAGE_INFO* item_info = (C2D_MESSAGE_INFO*)singlylinkedlist_item_get_value(list_item);

    return (item_info->message_id == match_info->message_id);
}

int iothub_client_statistics_add_c2d_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, C2D_EVENT_TYPE type, C2D_MESSAGE_INFO* info)
{
    int result;

    if (handle == NULL || info == NULL)
    {
        LogError("Invalid argument (handle=%p, type=%s, info=%p)", handle, MU_ENUM_TO_STRING(C2D_EVENT_TYPE, type), info);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        C2D_MESSAGE_INFO* queued_info;
        LIST_ITEM_HANDLE list_item = singlylinkedlist_find(stats->c2d_messages, find_c2d_message_info_by_id, info);

        if (list_item == NULL)
        {
            if (type != C2D_QUEUED)
            {
                LogError("C2D message info not found for message %lu (%d)", (unsigned long)info->message_id, (int)type);
                result = MU_FAILURE;
            }
            else if ((queued_info = (C2D_MESSAGE_INFO*)malloc(sizeof(C2D_MESSAGE_INFO))) == NULL)
            {
                LogError("Failed cloning the C2D_MESSAGE_INFO");
                result = MU_FAILURE;
            }
            else if (singlylinkedlist_add(stats->c2d_messages, queued_info) == NULL)
            {
                LogError("Failed adding c2d message info (message id: %lu)", (unsigned long)info->message_id);
                free(queued_info);
                result = MU_FAILURE;
            }
            else
            {
                queued_info->message_id = info->message_id;
                queued_info->time_queued = info->time_queued;
                queued_info->send_result = info->send_result;
                queued_info->time_received = INDEFINITE_TIME;
                queued_info->time_sent = INDEFINITE_TIME;

                result = 0;
            }
        }
        else
        {
            if ((queued_info = (C2D_MESSAGE_INFO*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("Failed retrieving queued c2d message info (message id: %lu)", (unsigned long)info->message_id);
                result = MU_FAILURE;
            }
            else
            {
                if (type == C2D_SENT)
                {
                    queued_info->time_sent = info->time_sent;
                    queued_info->send_callback_result = info->send_callback_result;
                    result = 0;
                }
                else if (type == C2D_RECEIVED)
                {
                    queued_info->time_received = info->time_received;
                    result = 0;
                }
                else
                {
                    LogError("C2D message %lu in queue, invalid event type (%d)", (unsigned long)info->message_id, (int)type);
                    result = MU_FAILURE;
                }
            }
        }
    }

    return result;
}

int iothub_client_statistics_get_c2d_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY* summary)
{
    int result;

    if (handle == NULL || summary == NULL)
    {
        LogError("Invalid argument (handle=%p, summary=%p)", handle, summary);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        LIST_ITEM_HANDLE list_item;

        (void)memset(summary, 0, sizeof(IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY));
        summary->min_travel_time_secs = LONG_MAX;

        list_item = singlylinkedlist_get_head_item(stats->c2d_messages);

        while (list_item != NULL)
        {
            C2D_MESSAGE_INFO* c2d_msg_info = (C2D_MESSAGE_INFO*)singlylinkedlist_item_get_value(list_item);

            if (c2d_msg_info->time_sent != INDEFINITE_TIME)
            {
                summary->messages_sent = summary->messages_sent + 1;

                if (c2d_msg_info->time_received != INDEFINITE_TIME)
                {
                    double travel_time = difftime(c2d_msg_info->time_received, c2d_msg_info->time_sent);

                    if (travel_time < summary->min_travel_time_secs)
                    {
                        summary->min_travel_time_secs = travel_time;
                    }

                    if (travel_time > summary->max_travel_time_secs)
                    {
                        summary->max_travel_time_secs = travel_time;
                    }

                    summary->messages_received = summary->messages_received + 1;
                }
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }

        result = 0;
    }

    return result;
}


static bool find_device_method_info_by_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    DEVICE_METHOD_INFO* match_info = (DEVICE_METHOD_INFO*)match_context;
    DEVICE_METHOD_INFO* item_info = (DEVICE_METHOD_INFO*)singlylinkedlist_item_get_value(list_item);

    return (item_info->method_id == match_info->method_id);
}

int iothub_client_statistics_add_device_method_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_METHOD_EVENT_TYPE type, DEVICE_METHOD_INFO* info)
{
    int result;

    if (handle == NULL || info == NULL)
    {
        LogError("Invalid argument (handle=%p, type=%s, info=%p)", handle, MU_ENUM_TO_STRING(DEVICE_METHOD_EVENT_TYPE, type), info);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        DEVICE_METHOD_INFO* queued_info;
        LIST_ITEM_HANDLE list_item = singlylinkedlist_find(stats->device_methods, find_device_method_info_by_id, info);

        result = MU_FAILURE;

        if (list_item == NULL)
        {
            if ((queued_info = (DEVICE_METHOD_INFO*)malloc(sizeof(DEVICE_METHOD_INFO))) == NULL)
            {
                LogError("Failed cloning the DEVICE_METHOD_INFO");
            }
            else if (singlylinkedlist_add(stats->device_methods, queued_info) == NULL)
            {
                LogError("Failed adding device methods info (method id: %lu)", (unsigned long)info->method_id);
                free(queued_info);
                queued_info = NULL;
            }
            else
            {
                memset(queued_info, 0, sizeof(DEVICE_METHOD_INFO));
                queued_info->method_id = info->method_id;
                queued_info->time_invoked = INDEFINITE_TIME;
                queued_info->time_received = INDEFINITE_TIME;
            }
        }
        else
        {
            if ((queued_info = (DEVICE_METHOD_INFO*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("Failed retrieving queued device method info (method id: %lu)", (unsigned long)info->method_id);
            }
        }

        if (queued_info != NULL)
        {
            if (type == DEVICE_METHOD_INVOKED)
            {
                queued_info->time_invoked = info->time_invoked;
                queued_info->method_result = info->method_result;
                result = 0;
            }
            else if (type == DEVICE_METHOD_RECEIVED)
            {
                queued_info->time_received = info->time_received;
                result = 0;
            }
            else
            {
                LogError("Device method %lu in queue; invalid event type (%d)", (unsigned long)info->method_id, (int)type);
            }
        }
    }

    return result;
}

int iothub_client_statistics_get_device_method_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY* summary)
{
    int result;

    if (handle == NULL || summary == NULL)
    {
        LogError("Invalid argument (handle=%p, summary=%p)", handle, summary);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        LIST_ITEM_HANDLE list_item;

        (void)memset(summary, 0, sizeof(IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY));
        summary->min_travel_time_secs = LONG_MAX;

        list_item = singlylinkedlist_get_head_item(stats->device_methods);

        while (list_item != NULL)
        {
            DEVICE_METHOD_INFO* device_method_info = (DEVICE_METHOD_INFO*)singlylinkedlist_item_get_value(list_item);

            if (device_method_info->time_invoked != INDEFINITE_TIME)
            {
                summary->methods_invoked = summary->methods_invoked + 1;

                if (device_method_info->time_received != INDEFINITE_TIME)
                {
                    double travel_time = difftime(device_method_info->time_received, device_method_info->time_invoked);

                    if (travel_time < summary->min_travel_time_secs)
                    {
                        summary->min_travel_time_secs = travel_time;
                    }

                    if (travel_time > summary->max_travel_time_secs)
                    {
                        summary->max_travel_time_secs = travel_time;
                    }

                    summary->methods_received = summary->methods_received + 1;
                }
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }

        result = 0;
    }

    return result;
}

static bool find_device_twin_info_by_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    DEVICE_TWIN_DESIRED_INFO* match_info = (DEVICE_TWIN_DESIRED_INFO*)match_context;
    DEVICE_TWIN_DESIRED_INFO* item_info = (DEVICE_TWIN_DESIRED_INFO*)singlylinkedlist_item_get_value(list_item);

    return (item_info->update_id == match_info->update_id);
}

static bool find_device_twin_reported_info_by_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    DEVICE_TWIN_REPORTED_INFO* match_info = (DEVICE_TWIN_REPORTED_INFO*)match_context;
    DEVICE_TWIN_REPORTED_INFO* item_info = (DEVICE_TWIN_REPORTED_INFO*)singlylinkedlist_item_get_value(list_item);

    return (item_info->update_id == match_info->update_id);
}

int iothub_client_statistics_add_device_twin_desired_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_TWIN_EVENT_TYPE type, DEVICE_TWIN_DESIRED_INFO* info)
{
    int result;

    if (handle == NULL || info == NULL)
    {
        LogError("Invalid argument (handle=%p, type=%s, info=%p)", handle, MU_ENUM_TO_STRING(DEVICE_TWIN_EVENT_TYPE, type), info);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        DEVICE_TWIN_DESIRED_INFO* queued_info;
        LIST_ITEM_HANDLE list_item = singlylinkedlist_find(stats->twin_desired_properties, find_device_twin_info_by_id, info);

        if (list_item == NULL)
        {
            if (type != DEVICE_TWIN_UPDATE_SENT)
            {
                LogError("Invalid info type (update id=%lu, type=%d)", (unsigned long)info->update_id, (int)type);
                result = MU_FAILURE;
            }
            else if ((queued_info = (DEVICE_TWIN_DESIRED_INFO*)malloc(sizeof(DEVICE_TWIN_DESIRED_INFO))) == NULL)
            {
                LogError("Failed cloning the DEVICE_TWIN_DESIRED_INFO");
                result = MU_FAILURE;
            }
            else if (singlylinkedlist_add(stats->twin_desired_properties, queued_info) == NULL)
            {
                LogError("Failed adding device twin info (update id: %lu)", (unsigned long)info->update_id);
                free(queued_info);
                result = MU_FAILURE;
            }
            else
            {
                memset(queued_info, 0, sizeof(DEVICE_TWIN_DESIRED_INFO));
                queued_info->update_id = info->update_id;
                queued_info->time_updated = info->time_updated;
                queued_info->update_result = info->update_result;
                queued_info->time_received = INDEFINITE_TIME;

                result = 0;
            }
        }
        else
        {
            if ((queued_info = (DEVICE_TWIN_DESIRED_INFO*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("Failed retrieving queued device twin info (update id: %lu)", (unsigned long)info->update_id);
                result = MU_FAILURE;
            }
            else if (type == DEVICE_TWIN_UPDATE_RECEIVED)
            {
                queued_info->time_received = info->time_received;

                result = 0;
            }
            else
            {
                LogError("Device twin %lu in queue; invalid event type (%d)", (unsigned long)info->update_id, (int)type);
                result = MU_FAILURE;
            }
        }
    }

    return result;
}

bool compare_message_time_to_connection_time(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    CONNECTION_STATUS_INFO* connection_status = (CONNECTION_STATUS_INFO*)list_item;
    time_t message_time = *((time_t*)match_context);
    if ((connection_status->status == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED || connection_status->reason == IOTHUB_CLIENT_CONNECTION_NO_NETWORK) &&
        connection_status->time < message_time && 
        connection_status->time > (message_time - SPAN_3_MINUTES_IN_SECONDS))
    {
        return true;
    }
    return false;
}

int iothub_client_statistics_get_device_twin_desired_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY* summary)
{
    int result;

    if (handle == NULL || summary == NULL)
    {
        LogError("Invalid argument (handle=%p, summary=%p)", handle, summary);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        LIST_ITEM_HANDLE list_item;

        (void)memset(summary, 0, sizeof(IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY));
        summary->min_travel_time_secs = LONG_MAX;

        list_item = singlylinkedlist_get_head_item(stats->twin_desired_properties);

        while (list_item != NULL)
        {
            DEVICE_TWIN_DESIRED_INFO* device_twin_info = (DEVICE_TWIN_DESIRED_INFO*)singlylinkedlist_item_get_value(list_item);

            if (device_twin_info->time_updated != INDEFINITE_TIME)
            {
                summary->updates_sent = summary->updates_sent + 1;

                if (device_twin_info->time_received != INDEFINITE_TIME)
                {
                    double travel_time = difftime(device_twin_info->time_received, device_twin_info->time_updated);

                    if (travel_time < summary->min_travel_time_secs)
                    {
                        summary->min_travel_time_secs = travel_time;
                    }

                    if (travel_time > summary->max_travel_time_secs)
                    {
                        summary->max_travel_time_secs = travel_time;
                    }

                    summary->updates_received = summary->updates_received + 1;
                }
                else
                {
                    // check to see if the device was disconnected during this twin update
                    // we will miss the update because we reconnected to hub
                    if (singlylinkedlist_find(stats->connection_status_history, compare_message_time_to_connection_time, &device_twin_info->time_updated))
                    {
                        summary->updates_sent--;
                        LogInfo("Removing twin desired update id (%d) because of network error", (int)device_twin_info->update_id);
                    }
                }
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }

        result = 0;
    }

    return result;
}

int iothub_client_statistics_add_device_twin_reported_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_TWIN_EVENT_TYPE type, DEVICE_TWIN_REPORTED_INFO* info)
{
    int result;

    if (handle == NULL || info == NULL)
    {
        LogError("Invalid argument (handle=%p, type=%s, info=%p)", handle, MU_ENUM_TO_STRING(DEVICE_TWIN_EVENT_TYPE, type), info);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        DEVICE_TWIN_REPORTED_INFO* queued_info;
        LIST_ITEM_HANDLE list_item = singlylinkedlist_find(stats->twin_reported_properties, find_device_twin_reported_info_by_id, info);

        if (list_item == NULL)
        {
            if (type != DEVICE_TWIN_UPDATE_QUEUED)
            {
                LogError("Invalid info type (update id=%lu, type=%d)", (unsigned long)info->update_id, (int)type);
                result = MU_FAILURE;
            }
            else if ((queued_info = (DEVICE_TWIN_REPORTED_INFO*)malloc(sizeof(DEVICE_TWIN_REPORTED_INFO))) == NULL)
            {
                LogError("Failed cloning the DEVICE_TWIN_REPORTED_INFO");
                result = MU_FAILURE;
            }
            else if (singlylinkedlist_add(stats->twin_reported_properties, queued_info) == NULL)
            {
                LogError("Failed adding device twin info (update id: %lu)", (unsigned long)info->update_id);
                free(queued_info);
                result = MU_FAILURE;
            }
            else
            {
                memset(queued_info, 0, sizeof(DEVICE_TWIN_REPORTED_INFO));
                queued_info->update_id = info->update_id;
                queued_info->time_queued = info->time_queued;
                queued_info->update_result = info->update_result;
                queued_info->time_sent = INDEFINITE_TIME;
                queued_info->time_received = INDEFINITE_TIME;

                result = 0;
            }
        }
        else
        {
            if ((queued_info = (DEVICE_TWIN_REPORTED_INFO*)singlylinkedlist_item_get_value(list_item)) == NULL)
            {
                LogError("Failed retrieving queued device twin info (update id: %lu)", (unsigned long)info->update_id);
                result = MU_FAILURE;
            }
            else  if (type == DEVICE_TWIN_UPDATE_SENT)
            {
                queued_info->time_sent = info->time_sent;
                queued_info->send_status_code = info->send_status_code;

                result = 0;
            }
            else if (type == DEVICE_TWIN_UPDATE_RECEIVED)
            {
                // Due to the nature of the service client, this function might be called several times for the same
                // test and message ID, so we must ensure only the first call sets the receive time.
                if (queued_info->time_received == INDEFINITE_TIME)
                {
                    queued_info->time_received = info->time_received;
                }

                result = 0;
            }
            else
            {
                LogError("Device twin %lu in queue; invalid event type (%d)", (unsigned long)info->update_id, (int)type);
                result = MU_FAILURE;
            }
        }
    }

    return result;
}

int iothub_client_statistics_get_device_twin_reported_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY* summary)
{
    int result;

    if (handle == NULL || summary == NULL)
    {
        LogError("Invalid argument (handle=%p, summary=%p)", handle, summary);
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_CLIENT_STATISTICS_HANDLE stats = (IOTHUB_CLIENT_STATISTICS*)handle;
        LIST_ITEM_HANDLE list_item;

        (void)memset(summary, 0, sizeof(IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY));
        summary->min_travel_time_secs = LONG_MAX;

        list_item = singlylinkedlist_get_head_item(stats->twin_reported_properties);

        while (list_item != NULL)
        {
            DEVICE_TWIN_REPORTED_INFO* device_twin_info = (DEVICE_TWIN_REPORTED_INFO*)singlylinkedlist_item_get_value(list_item);

            summary->updates_sent = summary->updates_sent + 1;

            if (device_twin_info->time_sent != INDEFINITE_TIME && device_twin_info->time_received != INDEFINITE_TIME)
            {
                double travel_time = difftime(device_twin_info->time_received, device_twin_info->time_sent);

                if (travel_time < summary->min_travel_time_secs)
                {
                    summary->min_travel_time_secs = travel_time;
                }

                if (travel_time > summary->max_travel_time_secs)
                {
                    summary->max_travel_time_secs = travel_time;
                }

                summary->updates_received = summary->updates_received + 1;
            }
            else
            {
                // check to see if the device was disconnected during this twin update
                // we will miss the update because we reconnected to hub
                if (singlylinkedlist_find(stats->connection_status_history, compare_message_time_to_connection_time, &device_twin_info->time_queued))
                {
                    summary->updates_sent--;
                    LogInfo("Removing twin reported update id (%d) because of network error", (int)device_twin_info->update_id);
                }
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }

        result = 0;
    }

    return result;
}