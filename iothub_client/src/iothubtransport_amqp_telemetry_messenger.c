// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "internal/uamqp_messaging.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_version.h"
#include "internal/iothubtransport_amqp_telemetry_messenger.h"

#define RESULT_OK 0
#define INDEFINITE_TIME ((time_t)(-1))

#define IOTHUB_DEVICES_PATH_FMT                         "%s/devices/%s"
#define IOTHUB_DEVICES_MODULE_PATH_FMT                  "%s/devices/%s/modules/%s"
#define IOTHUB_EVENT_SEND_ADDRESS_FMT                   "amqps://%s/messages/events"
#define IOTHUB_MESSAGE_RECEIVE_ADDRESS_FMT              "amqps://%s/messages/devicebound"
#define MESSAGE_SENDER_LINK_NAME_PREFIX                 "link-snd"
#define MESSAGE_SENDER_MAX_LINK_SIZE                    UINT64_MAX
#define MESSAGE_RECEIVER_LINK_NAME_PREFIX               "link-rcv"
#define MESSAGE_RECEIVER_MAX_LINK_SIZE                  65536
#define DEFAULT_EVENT_SEND_RETRY_LIMIT                  10
#define DEFAULT_EVENT_SEND_TIMEOUT_SECS                 600
#define MAX_MESSAGE_SENDER_STATE_CHANGE_TIMEOUT_SECS    300
#define MAX_MESSAGE_RECEIVER_STATE_CHANGE_TIMEOUT_SECS  300
#define UNIQUE_ID_BUFFER_SIZE                           37
#define STRING_NULL_TERMINATOR                          '\0'

#define AMQP_BATCHING_FORMAT_CODE 0x80013700

typedef struct TELEMETRY_MESSENGER_INSTANCE_TAG
{
    STRING_HANDLE device_id;
    STRING_HANDLE module_id;
    pfTransport_GetOption_Product_Info_Callback prod_info_cb;
    void* prod_info_ctx;

    STRING_HANDLE iothub_host_fqdn;
    SINGLYLINKEDLIST_HANDLE waiting_to_send;   // List of MESSENGER_SEND_EVENT_CALLER_INFORMATION's
    SINGLYLINKEDLIST_HANDLE in_progress_list;  // List of MESSENGER_SEND_EVENT_TASK's
    TELEMETRY_MESSENGER_STATE state;

    ON_TELEMETRY_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
    void* on_state_changed_context;

    bool receive_messages;
    ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED on_message_received_callback;
    void* on_message_received_context;

    SESSION_HANDLE session_handle;
    LINK_HANDLE sender_link;
    MESSAGE_SENDER_HANDLE message_sender;
    MESSAGE_SENDER_STATE message_sender_current_state;
    MESSAGE_SENDER_STATE message_sender_previous_state;
    LINK_HANDLE receiver_link;
    MESSAGE_RECEIVER_HANDLE message_receiver;
    MESSAGE_RECEIVER_STATE message_receiver_current_state;
    MESSAGE_RECEIVER_STATE message_receiver_previous_state;

    size_t event_send_retry_limit;
    size_t event_send_error_count;
    size_t event_send_timeout_secs;
    time_t last_message_sender_state_change_time;
    time_t last_message_receiver_state_change_time;
} TELEMETRY_MESSENGER_INSTANCE;

// MESSENGER_SEND_EVENT_CALLER_INFORMATION corresponds to a message sent from the API, including
// the message and callback information to alert caller about what happened.
typedef struct MESSENGER_SEND_EVENT_CALLER_INFORMATION_TAG
{
    IOTHUB_MESSAGE_LIST* message;
    ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE on_event_send_complete_callback;
    void* context;
} MESSENGER_SEND_EVENT_CALLER_INFORMATION;

// MESSENGER_SEND_EVENT_TASK interfaces with underlying uAMQP layer.  It receives the callback
// from this lower layer which is used to pass the results back to the API via the callback_list.
typedef struct MESSENGER_SEND_EVENT_TASK_TAG
{
    SINGLYLINKEDLIST_HANDLE callback_list;  // List of MESSENGER_SEND_EVENT_CALLER_INFORMATION's
    time_t send_time;
    TELEMETRY_MESSENGER_INSTANCE *messenger;
    bool is_timed_out;
} MESSENGER_SEND_EVENT_TASK;


// @brief
//     Evaluates if the ammount of time since start_time is greater or lesser than timeout_in_secs.
// @param is_timed_out
//     Set to 1 if a timeout has been reached, 0 otherwise. Not set if any failure occurs.
// @returns
//     0 if no failures occur, non-zero otherwise.
static int is_timeout_reached(time_t start_time, size_t timeout_in_secs, int *is_timed_out)
{
    int result;

    if (start_time == INDEFINITE_TIME)
    {
        LogError("Failed to verify timeout (start_time is INDEFINITE)");
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
                *is_timed_out = 1;
            }
            else
            {
                *is_timed_out = 0;
            }

            result = RESULT_OK;
        }
    }

    return result;
}

static STRING_HANDLE create_devices_and_modules_path(STRING_HANDLE iothub_host_fqdn, STRING_HANDLE device_id, STRING_HANDLE module_id)
{
    STRING_HANDLE devices_and_modules_path;

    if ((devices_and_modules_path = STRING_new()) == NULL)
    {
        LogError("Failed creating devices_and_modules_path (STRING_new failed)");
    }
    else
    {
        const char* iothub_host_fqdn_char_ptr = STRING_c_str(iothub_host_fqdn);
        const char* device_id_char_ptr = STRING_c_str(device_id);
        const char* module_id_char_ptr = STRING_c_str(module_id);

        if (module_id_char_ptr != NULL)
        {
            if (STRING_sprintf(devices_and_modules_path, IOTHUB_DEVICES_MODULE_PATH_FMT, iothub_host_fqdn_char_ptr, device_id_char_ptr, module_id_char_ptr) != RESULT_OK)
            {
                STRING_delete(devices_and_modules_path);
                devices_and_modules_path = NULL;
                LogError("Failed creating devices_and_modules_path (STRING_sprintf failed)");
            }
        }
        else
        {
            if (STRING_sprintf(devices_and_modules_path, IOTHUB_DEVICES_PATH_FMT, iothub_host_fqdn_char_ptr, device_id_char_ptr) != RESULT_OK)
            {
                STRING_delete(devices_and_modules_path);
                devices_and_modules_path = NULL;
                LogError("Failed creating devices_and_modules_path (STRING_sprintf failed)");
            }
        }
    }

    return devices_and_modules_path;
}

static STRING_HANDLE create_event_send_address(STRING_HANDLE devices_and_modules_path)
{
    STRING_HANDLE event_send_address;

    if ((event_send_address = STRING_new()) == NULL)
    {
        LogError("Failed creating the event_send_address (STRING_new failed)");
    }
    else
    {
        const char* devices_and_modules_path_char_ptr = STRING_c_str(devices_and_modules_path);
        if (STRING_sprintf(event_send_address, IOTHUB_EVENT_SEND_ADDRESS_FMT, devices_and_modules_path_char_ptr) != RESULT_OK)
        {
            STRING_delete(event_send_address);
            event_send_address = NULL;
            LogError("Failed creating the event_send_address (STRING_sprintf failed)");
        }
    }

    return event_send_address;
}

static STRING_HANDLE create_event_sender_source_name(STRING_HANDLE link_name)
{
    STRING_HANDLE source_name;

    if ((source_name = STRING_new()) == NULL)
    {
        LogError("Failed creating the source_name (STRING_new failed)");
    }
    else
    {
        const char* link_name_char_ptr = STRING_c_str(link_name);
        if (STRING_sprintf(source_name, "%s-source", link_name_char_ptr) != RESULT_OK)
        {
            STRING_delete(source_name);
            source_name = NULL;
            LogError("Failed creating the source_name (STRING_sprintf failed)");
        }
    }

    return source_name;
}

static STRING_HANDLE create_message_receive_address(STRING_HANDLE devices_and_modules_path)
{
    STRING_HANDLE message_receive_address;

    if ((message_receive_address = STRING_new()) == NULL)
    {
        LogError("Failed creating the message_receive_address (STRING_new failed)");
    }
    else
    {
        const char* devices_and_modules_path_char_ptr = STRING_c_str(devices_and_modules_path);
        if (STRING_sprintf(message_receive_address, IOTHUB_MESSAGE_RECEIVE_ADDRESS_FMT, devices_and_modules_path_char_ptr) != RESULT_OK)
        {
            STRING_delete(message_receive_address);
            message_receive_address = NULL;
            LogError("Failed creating the message_receive_address (STRING_sprintf failed)");
        }
    }

    return message_receive_address;
}

static STRING_HANDLE create_message_receiver_target_name(STRING_HANDLE link_name)
{
    STRING_HANDLE target_name;

    if ((target_name = STRING_new()) == NULL)
    {
        LogError("Failed creating the target_name (STRING_new failed)");
    }
    else
    {
        const char* link_name_char_ptr = STRING_c_str(link_name);
        if (STRING_sprintf(target_name, "%s-target", link_name_char_ptr) != RESULT_OK)
        {
            STRING_delete(target_name);
            target_name = NULL;
            LogError("Failed creating the target_name (STRING_sprintf failed)");
        }
    }

    return target_name;
}

static STRING_HANDLE create_link_name(const char* prefix, const char* infix)
{
    char* unique_id;
    STRING_HANDLE tag = NULL;

    if ((unique_id = (char*)malloc(sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1)) == NULL)
    {
        LogError("Failed generating an unique tag (malloc failed)");
    }
    else
    {
        memset(unique_id, 0, sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1);

        if (UniqueId_Generate(unique_id, UNIQUE_ID_BUFFER_SIZE) != UNIQUEID_OK)
        {
            LogError("Failed generating an unique tag (UniqueId_Generate failed)");
        }
        else if ((tag = STRING_new()) == NULL)
        {
            LogError("Failed generating an unique tag (STRING_new failed)");
        }
        else if (STRING_sprintf(tag, "%s-%s-%s", prefix, infix, unique_id) != RESULT_OK)
        {
            STRING_delete(tag);
            tag = NULL;
            LogError("Failed generating an unique tag (STRING_sprintf failed)");
        }

        free(unique_id);
    }

    return tag;
}

static void update_messenger_state(TELEMETRY_MESSENGER_INSTANCE* instance, TELEMETRY_MESSENGER_STATE new_state)
{
    if (new_state != instance->state)
    {
        TELEMETRY_MESSENGER_STATE previous_state = instance->state;
        instance->state = new_state;

        if (instance->on_state_changed_callback != NULL)
        {
            instance->on_state_changed_callback(instance->on_state_changed_context, previous_state, new_state);
        }
    }
}

static void attach_device_client_type_to_link(LINK_HANDLE link, pfTransport_GetOption_Product_Info_Callback prod_info_cb, void* prod_info_ctx)
{
    fields attach_properties;
    AMQP_VALUE device_client_type_key_name;
    AMQP_VALUE device_client_type_value;

    if ((attach_properties = amqpvalue_create_map()) == NULL)
    {
        LogError("Failed to create the map for device client type.");
    }
    else
    {
        if ((device_client_type_key_name = amqpvalue_create_symbol("com.microsoft:client-version")) == NULL)
        {
            LogError("Failed to create the key name for the device client type.");
        }
        else
        {
            if ((device_client_type_value = amqpvalue_create_string(prod_info_cb(prod_info_ctx))) == NULL)
            {
                LogError("Failed to create the key value for the device client type.");
            }
            else
            {
                if (amqpvalue_set_map_value(attach_properties, device_client_type_key_name, device_client_type_value) != 0)
                {
                    LogError("Failed to set the property map for the device client type");
                }
                else if (link_set_attach_properties(link, attach_properties) != 0)
                {
                    LogError("Unable to attach the device client type to the link properties");
                }

                amqpvalue_destroy(device_client_type_value);
            }

            amqpvalue_destroy(device_client_type_key_name);
        }

        amqpvalue_destroy(attach_properties);
    }
}

static void destroy_event_sender(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    if (instance->message_sender != NULL)
    {
        messagesender_destroy(instance->message_sender);
        instance->message_sender = NULL;
    }

    instance->message_sender_current_state = MESSAGE_SENDER_STATE_IDLE;
    instance->message_sender_previous_state = MESSAGE_SENDER_STATE_IDLE;
    instance->last_message_sender_state_change_time = INDEFINITE_TIME;

    if (instance->sender_link != NULL)
    {
        link_destroy(instance->sender_link);
        instance->sender_link = NULL;
    }
}

static void on_event_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    if (context == NULL)
    {
        LogError("on_event_sender_state_changed_callback was invoked with a NULL context; although unexpected, this failure will be ignored");
    }
    else
    {
        if (new_state != previous_state)
        {
            TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)context;
            instance->message_sender_current_state = new_state;
            instance->message_sender_previous_state = previous_state;
            instance->last_message_sender_state_change_time = get_time(NULL);
        }
    }
}

static int create_event_sender(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    int result;

    STRING_HANDLE link_name = NULL;
    STRING_HANDLE source_name = NULL;
    AMQP_VALUE source = NULL;
    AMQP_VALUE target = NULL;
    STRING_HANDLE devices_and_modules_path = NULL;
    STRING_HANDLE event_send_address = NULL;

    if ((devices_and_modules_path = create_devices_and_modules_path(instance->iothub_host_fqdn, instance->device_id, instance->module_id)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (failed creating the 'devices_and_modules_path')");
    }
    else if ((event_send_address = create_event_send_address(devices_and_modules_path)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (failed creating the 'event_send_address')");
    }
    else if ((link_name = create_link_name(MESSAGE_SENDER_LINK_NAME_PREFIX, STRING_c_str(instance->device_id))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (failed creating an unique link name)");
    }
    else if ((source_name = create_event_sender_source_name(link_name)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (failed creating an unique source name)");
    }
    else if ((source = messaging_create_source(STRING_c_str(source_name))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (messaging_create_source failed)");
    }
    else if ((target = messaging_create_target(STRING_c_str(event_send_address))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (messaging_create_target failed)");
    }
    else if ((instance->sender_link = link_create(instance->session_handle, STRING_c_str(link_name), role_sender, source, target)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message sender (link_create failed)");
    }
    else
    {
        if (link_set_max_message_size(instance->sender_link, MESSAGE_SENDER_MAX_LINK_SIZE) != RESULT_OK)
        {
            LogError("Failed setting message sender link max message size.");
        }

        attach_device_client_type_to_link(instance->sender_link, instance->prod_info_cb, instance->prod_info_ctx);

        if ((instance->message_sender = messagesender_create(instance->sender_link, on_event_sender_state_changed_callback, (void*)instance)) == NULL)
        {
            LogError("Failed creating the message sender (messagesender_create failed)");
            destroy_event_sender(instance);
            result = MU_FAILURE;
        }
        else
        {
            if (messagesender_open(instance->message_sender) != RESULT_OK)
            {
                LogError("Failed opening the AMQP message sender.");
                destroy_event_sender(instance);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    if (link_name != NULL)
        STRING_delete(link_name);
    if (source_name != NULL)
        STRING_delete(source_name);
    if (source != NULL)
        amqpvalue_destroy(source);
    if (target != NULL)
        amqpvalue_destroy(target);
    if (devices_and_modules_path != NULL)
        STRING_delete(devices_and_modules_path);
    if (event_send_address != NULL)
        STRING_delete(event_send_address);

    return result;
}

static void destroy_message_receiver(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    if (instance->message_receiver != NULL)
    {
        if (messagereceiver_close(instance->message_receiver) != RESULT_OK)
        {
            LogError("Failed closing the AMQP message receiver (this failure will be ignored).");
        }

        messagereceiver_destroy(instance->message_receiver);

        instance->message_receiver = NULL;
    }

    instance->message_receiver_current_state = MESSAGE_RECEIVER_STATE_IDLE;
    instance->message_receiver_previous_state = MESSAGE_RECEIVER_STATE_IDLE;
    instance->last_message_receiver_state_change_time = INDEFINITE_TIME;

    if (instance->receiver_link != NULL)
    {
        link_destroy(instance->receiver_link);
        instance->receiver_link = NULL;
    }
}

static void on_message_receiver_state_changed_callback(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    if (context == NULL)
    {
        LogError("on_message_receiver_state_changed_callback was invoked with a NULL context; although unexpected, this failure will be ignored");
    }
    else
    {
        if (new_state != previous_state)
        {
            TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)context;
            instance->message_receiver_current_state = new_state;
            instance->message_receiver_previous_state = previous_state;
            instance->last_message_receiver_state_change_time = get_time(NULL);
        }
    }
}

static TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* create_message_disposition_info(TELEMETRY_MESSENGER_INSTANCE* messenger)
{
    TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* result;

    if ((result = (TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO*)malloc(sizeof(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO))) == NULL)
    {
        LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO container (malloc failed)");
        result = NULL;
    }
    else
    {
        delivery_number message_id;

        if (messagereceiver_get_received_message_id(messenger->message_receiver, &message_id) != RESULT_OK)
        {
            LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO container (messagereceiver_get_received_message_id failed)");
            free(result);
            result = NULL;
        }
        else
        {
            const char* link_name;

            if (messagereceiver_get_link_name(messenger->message_receiver, &link_name) != RESULT_OK)
            {
                LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO container (messagereceiver_get_link_name failed)");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->source, link_name) != RESULT_OK)
            {
                LogError("Failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO container (failed copying link name)");
                free(result);
                result = NULL;
            }
            else
            {
                result->message_id = message_id;
            }
        }
    }

    return result;
}

static void destroy_message_disposition_info(TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info)
{
    free(disposition_info->source);
    free(disposition_info);
}

static AMQP_VALUE create_uamqp_disposition_result_from(TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    AMQP_VALUE uamqp_disposition_result;

    if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_NONE)
    {
        uamqp_disposition_result = NULL; // intentionally not sending an answer.
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_ACCEPTED)
    {
        uamqp_disposition_result = messaging_delivery_accepted();
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_RELEASED)
    {
        uamqp_disposition_result = messaging_delivery_released();
    }
    else if (disposition_result == TELEMETRY_MESSENGER_DISPOSITION_RESULT_REJECTED)
    {
        uamqp_disposition_result = messaging_delivery_rejected("Rejected by application", "Rejected by application");
    }
    else
    {
        LogError("Failed creating a disposition result for messagereceiver (result %d is not supported)", disposition_result);
        uamqp_disposition_result = NULL;
    }

    return uamqp_disposition_result;
}

static AMQP_VALUE on_message_received_internal_callback(const void* context, MESSAGE_HANDLE message)
{
    AMQP_VALUE result;
    IOTHUB_MESSAGE_HANDLE iothub_message;

    if (message_create_IoTHubMessage_from_uamqp_message(message, &iothub_message) != RESULT_OK)
    {
        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading AMQP message");

        LogError("on_message_received_internal_callback failed (message_create_IoTHubMessage_from_uamqp_message).");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)context;
        TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* message_disposition_info;

        if ((message_disposition_info = create_message_disposition_info(instance)) == NULL)
        {
            LogError("on_message_received_internal_callback failed (failed creating TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO).");
            result = messaging_delivery_released();
        }
        else
        {
            TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result = instance->on_message_received_callback(iothub_message, message_disposition_info, instance->on_message_received_context);

            destroy_message_disposition_info(message_disposition_info);

            result = create_uamqp_disposition_result_from(disposition_result);
        }
    }

    return result;
}

static int create_message_receiver(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    int result;

    STRING_HANDLE devices_and_modules_path = NULL;
    STRING_HANDLE message_receive_address = NULL;
    STRING_HANDLE link_name = NULL;
    STRING_HANDLE target_name = NULL;
    AMQP_VALUE source = NULL;
    AMQP_VALUE target = NULL;

    if ((devices_and_modules_path = create_devices_and_modules_path(instance->iothub_host_fqdn, instance->device_id, instance->module_id)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (failed creating the 'devices_and_modules_path')");
    }
    else if ((message_receive_address = create_message_receive_address(devices_and_modules_path)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (failed creating the 'message_receive_address')");
    }
    else if ((link_name = create_link_name(MESSAGE_RECEIVER_LINK_NAME_PREFIX, STRING_c_str(instance->device_id))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (failed creating an unique link name)");
    }
    else if ((target_name = create_message_receiver_target_name(link_name)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (failed creating an unique target name)");
    }
    else if ((target = messaging_create_target(STRING_c_str(target_name))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (messaging_create_target failed)");
    }
    else if ((source = messaging_create_source(STRING_c_str(message_receive_address))) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (messaging_create_source failed)");
    }
    else if ((instance->receiver_link = link_create(instance->session_handle, STRING_c_str(link_name), role_receiver, source, target)) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (link_create failed)");
    }
    else if (link_set_rcv_settle_mode(instance->receiver_link, receiver_settle_mode_first) != RESULT_OK)
    {
        result = MU_FAILURE;
        LogError("Failed creating the message receiver (link_set_rcv_settle_mode failed)");
    }
    else
    {
        if (link_set_max_message_size(instance->receiver_link, MESSAGE_RECEIVER_MAX_LINK_SIZE) != RESULT_OK)
        {
            LogError("Failed setting message receiver link max message size.");
        }

        attach_device_client_type_to_link(instance->receiver_link, instance->prod_info_cb, instance->prod_info_ctx);

        if ((instance->message_receiver = messagereceiver_create(instance->receiver_link, on_message_receiver_state_changed_callback, (void*)instance)) == NULL)
        {
            LogError("Failed creating the message receiver (messagereceiver_create failed)");
            destroy_message_receiver(instance);
            result = MU_FAILURE;
        }
        else
        {
            if (messagereceiver_open(instance->message_receiver, on_message_received_internal_callback, (void*)instance) != RESULT_OK)
            {
                LogError("Failed opening the AMQP message receiver.");
                destroy_message_receiver(instance);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    if (devices_and_modules_path != NULL)
        STRING_delete(devices_and_modules_path);
    if (message_receive_address != NULL)
        STRING_delete(message_receive_address);
    if (link_name != NULL)
        STRING_delete(link_name);
    if (target_name != NULL)
        STRING_delete(target_name);
    if (source != NULL)
        amqpvalue_destroy(source);
    if (target != NULL)
        amqpvalue_destroy(target);

    return result;
}

static int move_event_to_in_progress_list(MESSENGER_SEND_EVENT_TASK* task)
{
    int result;

    if (singlylinkedlist_add(task->messenger->in_progress_list, (void*)task) == NULL)
    {
        result = MU_FAILURE;
        LogError("Failed moving event to in_progress list (singlylinkedlist_add failed)");
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static bool find_MESSENGER_SEND_EVENT_TASK_on_list(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    return (list_item != NULL && singlylinkedlist_item_get_value(list_item) == match_context);
}

static void remove_event_from_in_progress_list(MESSENGER_SEND_EVENT_TASK *task)
{
    LIST_ITEM_HANDLE list_item = singlylinkedlist_find(task->messenger->in_progress_list, find_MESSENGER_SEND_EVENT_TASK_on_list, (void*)task);

    if (list_item != NULL)
    {
        if (singlylinkedlist_remove(task->messenger->in_progress_list, list_item) != RESULT_OK)
        {
            LogError("Failed removing event from in_progress list (singlylinkedlist_remove failed)");
        }
    }
}

static void free_task(MESSENGER_SEND_EVENT_TASK* task)
{
    LIST_ITEM_HANDLE list_node;

    if (NULL != task && NULL != task->callback_list)
    {
        while ((list_node = singlylinkedlist_get_head_item(task->callback_list)) != NULL)
        {
            MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_info = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)singlylinkedlist_item_get_value(list_node);
            (void)singlylinkedlist_remove(task->callback_list, list_node);
            free(caller_info);
        }
        singlylinkedlist_destroy(task->callback_list);
    }

    free(task);
}

static int copy_events_to_list(SINGLYLINKEDLIST_HANDLE from_list, SINGLYLINKEDLIST_HANDLE to_list)
{
    int result;
    LIST_ITEM_HANDLE list_item;

    result = RESULT_OK;
    list_item = singlylinkedlist_get_head_item(from_list);

    while (list_item != NULL)
    {
        MESSENGER_SEND_EVENT_TASK *task = (MESSENGER_SEND_EVENT_TASK*)singlylinkedlist_item_get_value(list_item);

        if (singlylinkedlist_add(to_list, task) == NULL)
        {
            LogError("Failed copying event to destination list (singlylinkedlist_add failed)");
            result = MU_FAILURE;
            break;
        }
        else
        {
            list_item = singlylinkedlist_get_next_item(list_item);
        }
    }

    return result;
}

static int copy_events_from_in_progress_to_waiting_list(TELEMETRY_MESSENGER_INSTANCE* instance, SINGLYLINKEDLIST_HANDLE to_list)
{
    int result;
    LIST_ITEM_HANDLE list_task_item;
    LIST_ITEM_HANDLE list_task_item_next = NULL;

    result = RESULT_OK;
    list_task_item = singlylinkedlist_get_head_item(instance->in_progress_list);

    while (list_task_item != NULL)
    {
        MESSENGER_SEND_EVENT_TASK* task = (MESSENGER_SEND_EVENT_TASK*)singlylinkedlist_item_get_value(list_task_item);
        if (task != NULL)
        {
            LIST_ITEM_HANDLE list_caller_item;

            list_caller_item = singlylinkedlist_get_head_item(task->callback_list);

            while (list_caller_item != NULL)
            {
                MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_information = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)singlylinkedlist_item_get_value(list_caller_item);

                if (singlylinkedlist_add(to_list, caller_information) == NULL)
                {
                    LogError("Failed copying event to destination list (singlylinkedlist_add failed)");
                    result = MU_FAILURE;
                    break;
                }

                list_caller_item = singlylinkedlist_get_next_item(list_caller_item);
            }

            list_task_item_next = singlylinkedlist_get_next_item(list_task_item);

            singlylinkedlist_destroy(task->callback_list);
            task->callback_list = NULL;

            free_task(task);
        }

        singlylinkedlist_remove(instance->in_progress_list, list_task_item);
        list_task_item = list_task_item_next;
    }

    return result;
}


static int move_events_to_wait_to_send_list(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    int result;
    LIST_ITEM_HANDLE list_item;

    if ((list_item = singlylinkedlist_get_head_item(instance->in_progress_list)) == NULL)
    {
        result = RESULT_OK;
    }
    else
    {
        SINGLYLINKEDLIST_HANDLE new_wait_to_send_list;

        if ((new_wait_to_send_list = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed moving events back to wait_to_send list (singlylinkedlist_create failed to create new wait_to_send_list)");
            result = MU_FAILURE;
        }
        else
        {
            SINGLYLINKEDLIST_HANDLE new_in_progress_list;

            if (copy_events_from_in_progress_to_waiting_list(instance, new_wait_to_send_list) != RESULT_OK)
            {
                LogError("Failed moving events back to wait_to_send list (failed adding in_progress_list items to new_wait_to_send_list)");
                singlylinkedlist_destroy(new_wait_to_send_list);
                result = MU_FAILURE;
            }
            else if (copy_events_to_list(instance->waiting_to_send, new_wait_to_send_list) != RESULT_OK)
            {
                LogError("Failed moving events back to wait_to_send list (failed adding wait_to_send items to new_wait_to_send_list)");
                singlylinkedlist_destroy(new_wait_to_send_list);
                result = MU_FAILURE;
            }
            else if ((new_in_progress_list = singlylinkedlist_create()) == NULL)
            {
                LogError("Failed moving events back to wait_to_send list (singlylinkedlist_create failed to create new in_progress_list)");
                singlylinkedlist_destroy(new_wait_to_send_list);
                result = MU_FAILURE;
            }
            else
            {
                singlylinkedlist_destroy(instance->waiting_to_send);
                singlylinkedlist_destroy(instance->in_progress_list);
                instance->waiting_to_send = new_wait_to_send_list;
                instance->in_progress_list = new_in_progress_list;
                result = RESULT_OK;
            }
        }
    }

    return result;
}

static MESSENGER_SEND_EVENT_TASK* create_task(TELEMETRY_MESSENGER_INSTANCE *messenger)
{
    MESSENGER_SEND_EVENT_TASK* task = NULL;

    if (NULL == (task = (MESSENGER_SEND_EVENT_TASK *)malloc(sizeof(MESSENGER_SEND_EVENT_TASK))))
    {
        LogError("malloc of MESSENGER_SEND_EVENT_TASK failed");
    }
    else
    {
        memset(task, 0, sizeof(*task ));
        task->messenger = messenger;
        task->send_time = INDEFINITE_TIME;
        if (NULL == (task->callback_list = singlylinkedlist_create()))
        {
            LogError("singlylinkedlist_create failed to create callback_list");
            free_task(task);
            task = NULL;
        }
        else if (RESULT_OK != move_event_to_in_progress_list(task))
        {
            LogError("move_event_to_in_progress_list failed");
            free_task(task);
            task = NULL;
        }
    }
    return task;
}



static void invoke_callback(const void* item, const void* action_context, bool* continue_processing)
{
    MESSENGER_SEND_EVENT_CALLER_INFORMATION *caller_info = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)item;

    if (NULL != caller_info->on_event_send_complete_callback)
    {
        TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT* messenger_send_result = (TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT*)action_context;
        caller_info->on_event_send_complete_callback(caller_info->message, *messenger_send_result, caller_info->context);
    }
    *continue_processing = true;
}

static void internal_on_event_send_complete_callback(void* context, MESSAGE_SEND_RESULT send_result, AMQP_VALUE delivery_state)
{
    (void)delivery_state;
    if (context != NULL)
    {
        MESSENGER_SEND_EVENT_TASK* task = (MESSENGER_SEND_EVENT_TASK*)context;

        if (task->messenger->message_sender_current_state != MESSAGE_SENDER_STATE_ERROR)
        {
            if (task->is_timed_out == false)
            {
                TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT messenger_send_result;

                if (send_result == MESSAGE_SEND_OK)
                {
                    messenger_send_result = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_OK;
                }
                else
                {
                    messenger_send_result = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
                }

                // Initially typecast to a size_t to avoid 64 bit compiler warnings on casting of void* to larger type.
                singlylinkedlist_foreach(task->callback_list, invoke_callback, (void*)&messenger_send_result);
            }
            else
            {
                LogInfo("messenger on_event_send_complete_callback invoked for timed out event %p; not firing upper layer callback.", task);
            }

            remove_event_from_in_progress_list(task);

            free_task(task);
        }
    }
}

static MESSENGER_SEND_EVENT_CALLER_INFORMATION* get_next_caller_message_to_send(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_info;
    LIST_ITEM_HANDLE list_item;

    if ((list_item = singlylinkedlist_get_head_item(instance->waiting_to_send)) == NULL)
    {
        caller_info = NULL;
    }
    else
    {
        caller_info = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)singlylinkedlist_item_get_value(list_item);

        if (singlylinkedlist_remove(instance->waiting_to_send, list_item) != RESULT_OK)
        {
            LogError("Failed removing item from waiting_to_send list (singlylinkedlist_remove failed)");
        }
    }

    return caller_info;
}

typedef struct SEND_PENDING_EVENTS_STATE_TAG
{
    MESSENGER_SEND_EVENT_TASK* task;
    MESSAGE_HANDLE message_batch_container;
    uint64_t bytes_pending;
} SEND_PENDING_EVENTS_STATE;


static int create_send_pending_events_state(TELEMETRY_MESSENGER_INSTANCE* instance, SEND_PENDING_EVENTS_STATE *send_pending_events_state)
{
    int result;
    memset(send_pending_events_state, 0, sizeof(*send_pending_events_state));

    if ((send_pending_events_state->message_batch_container = message_create()) == NULL)
    {
        LogError("messageBatchContainer = message_create() failed");
        result = MU_FAILURE;
    }
    else if (message_set_message_format(send_pending_events_state->message_batch_container, AMQP_BATCHING_FORMAT_CODE) != 0)
    {
         LogError("Failed setting the message format to batching format");
         result = MU_FAILURE;
    }
    else if ((send_pending_events_state->task = create_task(instance)) == NULL)
    {
        LogError("create_task() failed");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static void invoke_callback_on_error(MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT messenger_event_send_complete_result)
{
    caller_info->on_event_send_complete_callback(caller_info->message, messenger_event_send_complete_result, (void*)caller_info->context);
}

static int send_batched_message_and_reset_state(TELEMETRY_MESSENGER_INSTANCE* instance, SEND_PENDING_EVENTS_STATE *send_pending_events_state)
{
    int result;

    if (messagesender_send_async(instance->message_sender, send_pending_events_state->message_batch_container, internal_on_event_send_complete_callback, send_pending_events_state->task, 0) == NULL)
    {
        LogError("messagesender_send failed");
        result = MU_FAILURE;
    }
    else
    {
        send_pending_events_state->task->send_time = get_time(NULL);
        result = RESULT_OK;
    }

    message_destroy(send_pending_events_state->message_batch_container);
    memset(send_pending_events_state, 0, sizeof(*send_pending_events_state));

    return result;
}

static int get_max_message_size_for_batching(TELEMETRY_MESSENGER_INSTANCE* instance, uint64_t* max_messagesize)
{
    int result;

    if (link_get_peer_max_message_size(instance->sender_link, max_messagesize) != 0)
    {
        LogError("link_get_peer_max_message_size failed");
        result = MU_FAILURE;
    }
    // Reserve AMQP_BATCHING_RESERVE_SIZE bytes for AMQP overhead of the "main" message itself.
    else if (*max_messagesize <= AMQP_BATCHING_RESERVE_SIZE)
    {
        LogError("link_get_peer_max_message_size (%" PRIu64 ") is less than the reserve size (%lu)", *max_messagesize, (unsigned long)AMQP_BATCHING_RESERVE_SIZE);
        result = MU_FAILURE;
    }
    else
    {
        *max_messagesize -= AMQP_BATCHING_RESERVE_SIZE;
        result = 0;
    }

    return result;
}

static int send_pending_events(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    int result = RESULT_OK;

    MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_info;
    BINARY_DATA body_binary_data;

    SEND_PENDING_EVENTS_STATE send_pending_events_state;
    memset(&send_pending_events_state, 0, sizeof(send_pending_events_state));
    memset(&body_binary_data, 0, sizeof(body_binary_data));

    uint64_t max_messagesize = 0;

    while ((caller_info = get_next_caller_message_to_send(instance)) != NULL)
    {
        if (body_binary_data.bytes != NULL)
        {
            // Free here as this may have been set during previous run through loop.
            free((unsigned char*)body_binary_data.bytes);
        }
        memset(&body_binary_data, 0, sizeof(body_binary_data));

        if ((0 == max_messagesize) && (get_max_message_size_for_batching(instance, &max_messagesize)) != 0)
        {
            LogError("get_max_message_size_for_batching failed");
            invoke_callback_on_error(caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING);
            free(caller_info);
            result = MU_FAILURE;
            break;
        }
        else if ((send_pending_events_state.task == 0) && (create_send_pending_events_state(instance, &send_pending_events_state) != 0))
        {
            LogError("create_send_pending_events_state failed");
            invoke_callback_on_error(caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING);
            free(caller_info);
            result = MU_FAILURE;
            break;
        }
        else if (message_create_uamqp_encoding_from_iothub_message(send_pending_events_state.message_batch_container, caller_info->message->messageHandle, &body_binary_data) != RESULT_OK)
        {
            LogError("message_create_uamqp_encoding_from_iothub_message() failed.  Will continue to try to process messages, result");
            invoke_callback_on_error(caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_CANNOT_PARSE);
            free(caller_info);
            continue;
        }
        else if (body_binary_data.length > (int)max_messagesize)
        {
            LogError("a single message will encode to be %lu bytes, larger than max we will send the link %" PRIu64 ".  Will continue to try to process messages", (unsigned long)body_binary_data.length, max_messagesize);
            invoke_callback_on_error(caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING);
            free(caller_info);
            continue;
        }
        else if (send_pending_events_state.task == NULL ||
                 singlylinkedlist_add(send_pending_events_state.task->callback_list, (void*)caller_info) == NULL)
        {
            LogError("singlylinkedlist_add failed");
            invoke_callback_on_error(caller_info, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING);
            free(caller_info);
            result = MU_FAILURE;
            break;
        }

        // Once we've added the caller_info to the callback_list, don't directly 'invoke_callback_on_error' anymore directly.
        // The task is responsible for running through its callers for callbacks, even for errors in this function.
        // Similarly, responsibility for freeing this memory falls on the 'task' cleanup also.

        if (body_binary_data.length + send_pending_events_state.bytes_pending > max_messagesize)
        {
            // If we tried to add the current message, we would overflow.  Send what we've queued immediately.
            if (send_batched_message_and_reset_state(instance, &send_pending_events_state) != RESULT_OK)
            {
                LogError("send_batched_message_and_reset_state failed");
                result = MU_FAILURE;
                break;
            }

            // Now that we've given off the task to uAMQP layer, allocate a new task.
            if (create_send_pending_events_state(instance, &send_pending_events_state) != 0)
            {
                LogError("create_send_pending_events_state failed, result");
                result = MU_FAILURE;
                break;
            }
        }

        if (message_add_body_amqp_data(send_pending_events_state.message_batch_container, body_binary_data) != 0)
        {
            LogError("message_add_body_amqp_data failed");
            result = MU_FAILURE;
            break;
        }

        send_pending_events_state.bytes_pending += body_binary_data.length;
    }

    if ((result == 0) && (send_pending_events_state.bytes_pending != 0))
    {
        if (send_batched_message_and_reset_state(instance, &send_pending_events_state) != RESULT_OK)
        {
            LogError("send_batched_message_and_reset_state failed");
            result = MU_FAILURE;
        }
    }

    if (body_binary_data.bytes != NULL)
    {
        free((unsigned char*)body_binary_data.bytes);
    }

    // A non-NULL task indicates error, since otherwise send_batched_message_and_reset_state would've sent off messages and reset send_pending_events_state
    if (send_pending_events_state.task != NULL)
    {
        TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT send_result = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_FAIL_SENDING;
        singlylinkedlist_foreach(send_pending_events_state.task->callback_list, invoke_callback, (void*)&send_result);
        remove_event_from_in_progress_list(send_pending_events_state.task);
        free_task(send_pending_events_state.task);
    }

    if (send_pending_events_state.message_batch_container != NULL)
    {
        message_destroy(send_pending_events_state.message_batch_container);
    }

    return result;
}

// @brief
//     Goes through each task in in_progress_list and checks if the events timed out to be sent.
// @remarks
//     If an event is timed out, it is marked as such but not removed, and the upper layer callback is invoked.
// @returns
//     0 if no failures occur, non-zero otherwise.
static int process_event_send_timeouts(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    int result = RESULT_OK;

    if (instance->event_send_timeout_secs > 0)
    {
        LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(instance->in_progress_list);

        while (list_item != NULL)
        {
            MESSENGER_SEND_EVENT_TASK* task = (MESSENGER_SEND_EVENT_TASK*)singlylinkedlist_item_get_value(list_item);

            if (task != NULL && task->is_timed_out == false)
            {
                int is_timed_out;

                if (is_timeout_reached(task->send_time, instance->event_send_timeout_secs, &is_timed_out) == RESULT_OK)
                {
                    if (is_timed_out)
                    {
                        task->is_timed_out = true;
                        TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT send_result = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
                        singlylinkedlist_foreach(task->callback_list, invoke_callback, (void*)&send_result);
                    }
                }
                else
                {
                    LogError("messenger failed to evaluate event send timeout of event %p", task);
                    result = MU_FAILURE;
                }
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }
    }

    return result;
}

// @brief
//     Removes all the timed out events from the in_progress_list, without invoking callbacks or detroying the messages.
static void remove_timed_out_events(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(instance->in_progress_list);

    while (list_item != NULL)
    {
        MESSENGER_SEND_EVENT_TASK* task = (MESSENGER_SEND_EVENT_TASK*)singlylinkedlist_item_get_value(list_item);

        if (task != NULL && task->is_timed_out == true)
        {
            remove_event_from_in_progress_list(task);

            free_task(task);
        }

        list_item = singlylinkedlist_get_next_item(list_item);
    }
}


// ---------- Set/Retrieve Options Helpers ----------//

static void* telemetry_messenger_clone_option(const char* name, const void* value)
{
    void* result;

    if (name == NULL)
    {
        LogError("Failed to clone messenger option (name is NULL)");
        result = NULL;
    }
    else if (value == NULL)
    {
        LogError("Failed to clone messenger option (value is NULL)");
        result = NULL;
    }
    else
    {
        if (strcmp(TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, name) == 0 ||
            strcmp(TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS, name) == 0)
        {
            result = (void*)value;
        }
        else
        {
            LogError("Failed to clone messenger option (option with name '%s' is not suppported)", name);
            result = NULL;
        }
    }

    return result;
}

static void telemetry_messenger_destroy_option(const char* name, const void* value)
{
    if (name == NULL)
    {
        LogError("Failed to destroy messenger option (name is NULL)");
    }
    else if (value == NULL)
    {
        LogError("Failed to destroy messenger option (value is NULL)");
    }
    else
    {
        // Nothing to be done for the supported options.
    }
}


// Public API:

int telemetry_messenger_subscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle, ON_TELEMETRY_MESSENGER_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
    int result;

    if (messenger_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("telemetry_messenger_subscribe_for_messages failed (messenger_handle is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (instance->receive_messages)
        {
            result = MU_FAILURE;
            LogError("telemetry_messenger_subscribe_for_messages failed (messenger already subscribed)");
        }
        else if (on_message_received_callback == NULL)
        {
            result = MU_FAILURE;
            LogError("telemetry_messenger_subscribe_for_messages failed (on_message_received_callback is NULL)");
        }
        else
        {
            instance->on_message_received_callback = on_message_received_callback;

            instance->on_message_received_context = context;

            instance->receive_messages = true;

            result = RESULT_OK;
        }
    }

    return result;
}

int telemetry_messenger_unsubscribe_for_messages(TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    int result;

    if (messenger_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("telemetry_messenger_unsubscribe_for_messages failed (messenger_handle is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (instance->receive_messages == false)
        {
            result = MU_FAILURE;
            LogError("telemetry_messenger_unsubscribe_for_messages failed (messenger is not subscribed)");
        }
        else
        {
            instance->receive_messages = false;

            instance->on_message_received_callback = NULL;

            instance->on_message_received_context = NULL;

            result = RESULT_OK;
        }
    }

    return result;
}

int telemetry_messenger_send_message_disposition(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, TELEMETRY_MESSENGER_DISPOSITION_RESULT disposition_result)
{
    int result;

    if (messenger_handle == NULL || disposition_info == NULL)
    {
        LogError("Failed sending message disposition (either messenger_handle (%p) or disposition_info (%p) are NULL)", messenger_handle, disposition_info);
        result = MU_FAILURE;
    }
    else if (disposition_info->source == NULL)
    {
        LogError("Failed sending message disposition (disposition_info->source is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* messenger = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (messenger->message_receiver == NULL)
        {
            LogError("Failed sending message disposition (message_receiver is not created; check if it is subscribed)");
            result = MU_FAILURE;
        }
        else
        {
            AMQP_VALUE uamqp_disposition_result;

            if ((uamqp_disposition_result = create_uamqp_disposition_result_from(disposition_result)) == NULL)
            {
                LogError("Failed sending message disposition (disposition result %d is not supported)", disposition_result);
                result = MU_FAILURE;
            }
            else
            {
                if (messagereceiver_send_message_disposition(messenger->message_receiver, disposition_info->source, disposition_info->message_id, uamqp_disposition_result) != RESULT_OK)
                {
                    LogError("Failed sending message disposition (messagereceiver_send_message_disposition failed)");
                    result = MU_FAILURE;
                }
                else
                {
                    result = RESULT_OK;
                }

                amqpvalue_destroy(uamqp_disposition_result);
            }
        }
    }

    return result;
}

int telemetry_messenger_send_async(TELEMETRY_MESSENGER_HANDLE messenger_handle, IOTHUB_MESSAGE_LIST* message, ON_TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE on_messenger_event_send_complete_callback, void* context)
{
    int result;

    if (messenger_handle == NULL)
    {
        LogError("Failed sending event (messenger_handle is NULL)");
        result = MU_FAILURE;
    }
    else if (message == NULL)
    {
        LogError("Failed sending event (message is NULL)");
        result = MU_FAILURE;
    }
    else if (on_messenger_event_send_complete_callback == NULL)
    {
        LogError("Failed sending event (on_event_send_complete_callback is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        MESSENGER_SEND_EVENT_CALLER_INFORMATION *caller_info;
        TELEMETRY_MESSENGER_INSTANCE *instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if ((caller_info = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)malloc(sizeof(MESSENGER_SEND_EVENT_CALLER_INFORMATION))) == NULL)
        {
            LogError("Failed sending event (failed to create struct for task; malloc failed)");
            result = MU_FAILURE;
        }
        else if (singlylinkedlist_add(instance->waiting_to_send, caller_info) == NULL)
        {
            LogError("Failed sending event (singlylinkedlist_add failed)");

            result = MU_FAILURE;

            free(caller_info);
        }
        else
        {
            memset(caller_info, 0, sizeof(MESSENGER_SEND_EVENT_CALLER_INFORMATION));
            caller_info->message = message;
            caller_info->on_event_send_complete_callback = on_messenger_event_send_complete_callback;
            caller_info->context = context;

            result = RESULT_OK;
        }
    }

    return result;
}

int telemetry_messenger_get_send_status(TELEMETRY_MESSENGER_HANDLE messenger_handle, TELEMETRY_MESSENGER_SEND_STATUS* send_status)
{
    int result;

    if (messenger_handle == NULL)
    {
        LogError("telemetry_messenger_get_send_status failed (messenger_handle is NULL)");
        result = MU_FAILURE;
    }
    else if (send_status == NULL)
    {
        LogError("telemetry_messenger_get_send_status failed (send_status is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;
        LIST_ITEM_HANDLE wts_list_head = singlylinkedlist_get_head_item(instance->waiting_to_send);
        LIST_ITEM_HANDLE ip_list_head = singlylinkedlist_get_head_item(instance->in_progress_list);

        if (wts_list_head == NULL && ip_list_head == NULL)
        {
            *send_status = TELEMETRY_MESSENGER_SEND_STATUS_IDLE;
        }
        else
        {
            *send_status = TELEMETRY_MESSENGER_SEND_STATUS_BUSY;
        }

        result = RESULT_OK;
    }

    return result;
}

int telemetry_messenger_start(TELEMETRY_MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle)
{
    int result;

    if (messenger_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("telemetry_messenger_start failed (messenger_handle is NULL)");
    }
    else if (session_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("telemetry_messenger_start failed (session_handle is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (instance->state != TELEMETRY_MESSENGER_STATE_STOPPED)
        {
            result = MU_FAILURE;
            LogError("telemetry_messenger_start failed (current state is %d; expected TELEMETRY_MESSENGER_STATE_STOPPED)", instance->state);
        }
        else
        {
            instance->session_handle = session_handle;

            update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_STARTING);

            result = RESULT_OK;
        }
    }

    return result;
}

int telemetry_messenger_stop(TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    int result;

    if (messenger_handle == NULL)
    {
        result = MU_FAILURE;
        LogError("telemetry_messenger_stop failed (messenger_handle is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (instance->state == TELEMETRY_MESSENGER_STATE_STOPPED)
        {
            result = MU_FAILURE;
            LogError("telemetry_messenger_stop failed (messenger is already stopped)");
        }
        else
        {
            update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_STOPPING);

            destroy_event_sender(instance);
            destroy_message_receiver(instance);

            remove_timed_out_events(instance);

            if (move_events_to_wait_to_send_list(instance) != RESULT_OK)
            {
                LogError("Messenger failed to move events in progress back to wait_to_send list");

                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                result = MU_FAILURE;
            }
            else
            {
                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_STOPPED);
                result = RESULT_OK;
            }
        }
    }

    return result;
}

// @brief
//     Sets the messenger module state based on the state changes from messagesender and messagereceiver
static void process_state_changes(TELEMETRY_MESSENGER_INSTANCE* instance)
{
    // Note: messagesender and messagereceiver are still not created or already destroyed
    //       when state is TELEMETRY_MESSENGER_STATE_STOPPED, so no checking is needed there.

    if (instance->state == TELEMETRY_MESSENGER_STATE_STARTED)
    {
        if (instance->message_sender_current_state != MESSAGE_SENDER_STATE_OPEN)
        {
            LogError("messagesender reported unexpected state %d while messenger was started", instance->message_sender_current_state);
            update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
        }
        else if (instance->message_receiver != NULL && instance->message_receiver_current_state != MESSAGE_RECEIVER_STATE_OPEN)
        {
            if (instance->message_receiver_current_state == MESSAGE_RECEIVER_STATE_OPENING)
            {
                int is_timed_out;
                if (is_timeout_reached(instance->last_message_receiver_state_change_time, MAX_MESSAGE_RECEIVER_STATE_CHANGE_TIMEOUT_SECS, &is_timed_out) != RESULT_OK)
                {
                    LogError("messenger got an error (failed to verify messagereceiver start timeout)");
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
                else if (is_timed_out == 1)
                {
                    LogError("messenger got an error (messagereceiver failed to start within expected timeout (%d secs))", MAX_MESSAGE_RECEIVER_STATE_CHANGE_TIMEOUT_SECS);
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
            }
            else if (instance->message_receiver_current_state == MESSAGE_RECEIVER_STATE_ERROR ||
                instance->message_receiver_current_state == MESSAGE_RECEIVER_STATE_IDLE)
            {
                LogError("messagereceiver reported unexpected state %d while messenger is starting", instance->message_receiver_current_state);
                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
            }
        }
    }
    else
    {
        if (instance->state == TELEMETRY_MESSENGER_STATE_STARTING)
        {
            if (instance->message_sender_current_state == MESSAGE_SENDER_STATE_OPEN)
            {
                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_STARTED);
            }
            else if (instance->message_sender_current_state == MESSAGE_SENDER_STATE_OPENING)
            {
                int is_timed_out;
                if (is_timeout_reached(instance->last_message_sender_state_change_time, MAX_MESSAGE_SENDER_STATE_CHANGE_TIMEOUT_SECS, &is_timed_out) != RESULT_OK)
                {
                    LogError("messenger failed to start (failed to verify messagesender start timeout)");
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
                else if (is_timed_out == 1)
                {
                    LogError("messenger failed to start (messagesender failed to start within expected timeout (%d secs))", MAX_MESSAGE_SENDER_STATE_CHANGE_TIMEOUT_SECS);
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
            }
            // For this module, the only valid scenario where messagesender state is IDLE is if
            // the messagesender hasn't been created yet or already destroyed.
            else if ((instance->message_sender_current_state == MESSAGE_SENDER_STATE_ERROR) ||
                (instance->message_sender_current_state == MESSAGE_SENDER_STATE_CLOSING) ||
                (instance->message_sender_current_state == MESSAGE_SENDER_STATE_IDLE && instance->message_sender != NULL))
            {
                LogError("messagesender reported unexpected state %d while messenger is starting", instance->message_sender_current_state);
                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
            }
        }
        // message sender and receiver are stopped/destroyed synchronously, so no need for state control.
    }
}

void telemetry_messenger_do_work(TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    if (messenger_handle == NULL)
    {
        LogError("telemetry_messenger_do_work failed (messenger_handle is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        process_state_changes(instance);

        if (instance->state == TELEMETRY_MESSENGER_STATE_STARTING)
        {
            if (instance->message_sender == NULL)
            {
                if (create_event_sender(instance) != RESULT_OK)
                {
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
            }
        }
        else if (instance->state == TELEMETRY_MESSENGER_STATE_STARTED)
        {
            if (instance->receive_messages == true &&
                instance->message_receiver == NULL &&
                create_message_receiver(instance) != RESULT_OK)
            {
                LogError("telemetry_messenger_do_work warning (failed creating the message receiver [%s])", STRING_c_str(instance->device_id));
            }
            else if (instance->receive_messages == false && instance->message_receiver != NULL)
            {
                destroy_message_receiver(instance);
            }

            if (process_event_send_timeouts(instance) != RESULT_OK)
            {
                update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
            }
            else if (send_pending_events(instance) != RESULT_OK && instance->event_send_retry_limit > 0)
            {
                instance->event_send_error_count++;

                if (instance->event_send_error_count >= instance->event_send_retry_limit)
                {
                    LogError("telemetry_messenger_do_work failed (failed sending events; reached max number of consecutive attempts)");
                    update_messenger_state(instance, TELEMETRY_MESSENGER_STATE_ERROR);
                }
            }
            else
            {
                instance->event_send_error_count = 0;
            }
        }
    }
}

void telemetry_messenger_destroy(TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    if (messenger_handle == NULL)
    {
        LogError("telemetry_messenger_destroy failed (messenger_handle is NULL)");
    }
    else
    {
        LIST_ITEM_HANDLE list_node;
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (instance->state != TELEMETRY_MESSENGER_STATE_STOPPED)
        {
            (void)telemetry_messenger_stop(messenger_handle);
        }


        // Note: yes telemetry_messenger_stop() tried to move all events from in_progress_list to wait_to_send_list,
        //       but we need to iterate through in case any events failed to be moved.
        while ((list_node = singlylinkedlist_get_head_item(instance->in_progress_list)) != NULL)
        {
            MESSENGER_SEND_EVENT_TASK* task = (MESSENGER_SEND_EVENT_TASK*)singlylinkedlist_item_get_value(list_node);

            (void)singlylinkedlist_remove(instance->in_progress_list, list_node);

            if (task != NULL)
            {
                TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT send_result = TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_ERROR_TIMEOUT;
                singlylinkedlist_foreach(task->callback_list, invoke_callback, (void*)&send_result);
                free_task(task);
            }
        }

        while ((list_node = singlylinkedlist_get_head_item(instance->waiting_to_send)) != NULL)
        {
            MESSENGER_SEND_EVENT_CALLER_INFORMATION* caller_info = (MESSENGER_SEND_EVENT_CALLER_INFORMATION*)singlylinkedlist_item_get_value(list_node);

            (void)singlylinkedlist_remove(instance->waiting_to_send, list_node);

            if (caller_info != NULL)
            {
                caller_info->on_event_send_complete_callback(caller_info->message, TELEMETRY_MESSENGER_EVENT_SEND_COMPLETE_RESULT_MESSENGER_DESTROYED, (void*)caller_info->context);
                free(caller_info);
            }
        }

        singlylinkedlist_destroy(instance->waiting_to_send);
        singlylinkedlist_destroy(instance->in_progress_list);

        STRING_delete(instance->iothub_host_fqdn);

        STRING_delete(instance->device_id);

        STRING_delete(instance->module_id);

        (void)free(instance);
    }
}

TELEMETRY_MESSENGER_HANDLE telemetry_messenger_create(const TELEMETRY_MESSENGER_CONFIG* messenger_config, pfTransport_GetOption_Product_Info_Callback prod_info_cb, void* prod_info_ctx)
{
    TELEMETRY_MESSENGER_HANDLE handle;

    if (messenger_config == NULL)
    {
        handle = NULL;
        LogError("telemetry_messenger_create failed (messenger_config is NULL)");
    }
    else if (messenger_config->device_id == NULL)
    {
        handle = NULL;
        LogError("telemetry_messenger_create failed (messenger_config->device_id is NULL)");
    }
    else if (messenger_config->iothub_host_fqdn == NULL)
    {
        handle = NULL;
        LogError("telemetry_messenger_create failed (messenger_config->iothub_host_fqdn is NULL)");
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance;

        if ((instance = (TELEMETRY_MESSENGER_INSTANCE*)malloc(sizeof(TELEMETRY_MESSENGER_INSTANCE))) == NULL)
        {
            handle = NULL;
            LogError("telemetry_messenger_create failed (messenger_config->wait_to_send_list is NULL)");
        }
        else
        {
            memset(instance, 0, sizeof(TELEMETRY_MESSENGER_INSTANCE));
            instance->state = TELEMETRY_MESSENGER_STATE_STOPPED;
            instance->message_sender_current_state = MESSAGE_SENDER_STATE_IDLE;
            instance->message_sender_previous_state = MESSAGE_SENDER_STATE_IDLE;
            instance->message_receiver_current_state = MESSAGE_RECEIVER_STATE_IDLE;
            instance->message_receiver_previous_state = MESSAGE_RECEIVER_STATE_IDLE;
            instance->event_send_retry_limit = DEFAULT_EVENT_SEND_RETRY_LIMIT;
            instance->event_send_timeout_secs = DEFAULT_EVENT_SEND_TIMEOUT_SECS;
            instance->last_message_sender_state_change_time = INDEFINITE_TIME;
            instance->last_message_receiver_state_change_time = INDEFINITE_TIME;

            if ((instance->device_id = STRING_construct(messenger_config->device_id)) == NULL)
            {
                handle = NULL;
                LogError("telemetry_messenger_create failed (device_id could not be copied; STRING_construct failed)");
            }
            else if ((messenger_config->module_id != NULL) && ((instance->module_id = STRING_construct(messenger_config->module_id)) == NULL))
            {
                handle = NULL;
                LogError("telemetry_messenger_create failed (module_id could not be copied; STRING_construct failed)");
            }
            else if ((instance->iothub_host_fqdn = STRING_construct(messenger_config->iothub_host_fqdn)) == NULL)
            {
                handle = NULL;
                LogError("telemetry_messenger_create failed (iothub_host_fqdn could not be copied; STRING_construct failed)");
            }
            else if ((instance->waiting_to_send = singlylinkedlist_create()) == NULL)
            {
                handle = NULL;
                LogError("telemetry_messenger_create failed (singlylinkedlist_create failed to create wait_to_send_list)");
            }
            else if ((instance->in_progress_list = singlylinkedlist_create()) == NULL)
            {
                handle = NULL;
                LogError("telemetry_messenger_create failed (singlylinkedlist_create failed to create in_progress_list)");
            }
            else
            {
                instance->on_state_changed_callback = messenger_config->on_state_changed_callback;

                instance->on_state_changed_context = messenger_config->on_state_changed_context;

                instance->prod_info_cb = prod_info_cb;
                instance->prod_info_ctx = prod_info_ctx;

                handle = (TELEMETRY_MESSENGER_HANDLE)instance;
            }
        }

        if (handle == NULL)
        {
            telemetry_messenger_destroy((TELEMETRY_MESSENGER_HANDLE)instance);
        }
    }

    return handle;
}

int telemetry_messenger_set_option(TELEMETRY_MESSENGER_HANDLE messenger_handle, const char* name, void* value)
{
    int result;

    if (messenger_handle == NULL || name == NULL || value == NULL)
    {
        LogError("telemetry_messenger_set_option failed (one of the followin are NULL: messenger_handle=%p, name=%p, value=%p)",
            messenger_handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

        if (strcmp(TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, name) == 0)
        {
            instance->event_send_timeout_secs = *((size_t*)value);
            result = RESULT_OK;
        }
        else if (strcmp(TELEMETRY_MESSENGER_OPTION_SAVED_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, messenger_handle) != OPTIONHANDLER_OK)
            {
                LogError("telemetry_messenger_set_option failed (OptionHandler_FeedOptions failed)");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            LogError("telemetry_messenger_set_option failed (option with name '%s' is not suppported)", name);
            result = MU_FAILURE;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE telemetry_messenger_retrieve_options(TELEMETRY_MESSENGER_HANDLE messenger_handle)
{
    OPTIONHANDLER_HANDLE result;

    if (messenger_handle == NULL)
    {
        LogError("Failed to retrieve options from messenger instance (messenger_handle is NULL)");
        result = NULL;
    }
    else
    {
        OPTIONHANDLER_HANDLE options = OptionHandler_Create(telemetry_messenger_clone_option, telemetry_messenger_destroy_option, (pfSetOption)telemetry_messenger_set_option);

        if (options == NULL)
        {
            LogError("Failed to retrieve options from messenger instance (OptionHandler_Create failed)");
            result = NULL;
        }
        else
        {
            TELEMETRY_MESSENGER_INSTANCE* instance = (TELEMETRY_MESSENGER_INSTANCE*)messenger_handle;

            if (OptionHandler_AddOption(options, TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS, (void*)&instance->event_send_timeout_secs) != OPTIONHANDLER_OK)
            {
                LogError("Failed to retrieve options from messenger instance (OptionHandler_Create failed for option '%s')", TELEMETRY_MESSENGER_OPTION_EVENT_SEND_TIMEOUT_SECS);
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
