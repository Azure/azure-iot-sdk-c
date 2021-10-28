// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/amqp_definitions_application_properties.h"

#include "internal/iothubtransportamqp_methods.h"
#include "internal/iothub_internal_consts.h"

typedef enum SUBSCRIBE_STATE_TAG
{
    SUBSCRIBE_STATE_NOT_SUBSCRIBED,
    SUBSCRIBE_STATE_SUBSCRIBED
} SUBSCRIBE_STATE;

typedef struct IOTHUBTRANSPORT_AMQP_METHODS_TAG
{
    char* device_id;
    char* module_id;
    char* hostname;
    LINK_HANDLE receiver_link;
    LINK_HANDLE sender_link;
    MESSAGE_RECEIVER_HANDLE message_receiver;
    MESSAGE_SENDER_HANDLE message_sender;
    ON_METHOD_REQUEST_RECEIVED on_method_request_received;
    void* on_method_request_received_context;
    ON_METHODS_ERROR on_methods_error;
    void* on_methods_error_context;
    ON_METHODS_UNSUBSCRIBED on_methods_unsubscribed;
    void* on_methods_unsubscribed_context;
    SUBSCRIBE_STATE subscribe_state;
    IOTHUBTRANSPORT_AMQP_METHOD_HANDLE* method_request_handles;
    size_t method_request_handle_count;
    bool receiver_link_disconnected;
    bool sender_link_disconnected;
} IOTHUBTRANSPORT_AMQP_METHODS;

typedef enum MESSAGE_OUTCOME_TAG
{
    MESSAGE_OUTCOME_ACCEPTED,
    MESSAGE_OUTCOME_REJECTED,
    MESSAGE_OUTCOME_RELEASED
} MESSAGE_OUTCOME;

typedef struct IOTHUBTRANSPORT_AMQP_METHOD_TAG
{
    IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle;
    uuid correlation_id;
} IOTHUBTRANSPORT_AMQP_METHOD;

static void remove_tracked_handle(IOTHUBTRANSPORT_AMQP_METHODS* amqp_methods_handle, IOTHUBTRANSPORT_AMQP_METHOD_HANDLE method_request_handle)
{
    size_t i;

    for (i = 0; i < amqp_methods_handle->method_request_handle_count; i++)
    {
        if (amqp_methods_handle->method_request_handles[i] == method_request_handle)
        {
            if (amqp_methods_handle->method_request_handle_count - i > 1)
            {
                (void)memmove(&amqp_methods_handle->method_request_handles[i], &amqp_methods_handle->method_request_handles[i + 1],
                    (amqp_methods_handle->method_request_handle_count - i - 1) * sizeof(IOTHUBTRANSPORT_AMQP_METHOD_HANDLE));
            }

            amqp_methods_handle->method_request_handle_count--;
            i--;
        }
    }

    if (amqp_methods_handle->method_request_handle_count == 0)
    {
        free(amqp_methods_handle->method_request_handles);
        amqp_methods_handle->method_request_handles = NULL;
    }
    else
    {
        IOTHUBTRANSPORT_AMQP_METHOD_HANDLE* new_handles = (IOTHUBTRANSPORT_AMQP_METHOD_HANDLE*)realloc(amqp_methods_handle->method_request_handles, amqp_methods_handle->method_request_handle_count * sizeof(IOTHUBTRANSPORT_AMQP_METHOD_HANDLE));
        if (new_handles != NULL)
        {
            amqp_methods_handle->method_request_handles = new_handles;
        }
    }
}

IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransportamqp_methods_create(const char* hostname, const char* device_id, const char* module_id)
{
    IOTHUBTRANSPORT_AMQP_METHODS* result;

    if ((hostname == NULL) ||
        (device_id == NULL))
    {
        result = NULL;
        LogError("Bad arguments: hostname=%p, device_id=%p", hostname, device_id);
    }
    else
    {
        result = malloc(sizeof(IOTHUBTRANSPORT_AMQP_METHODS));
        if (result == NULL)
        {
            LogError("Cannot allocate memory for AMQP C2D methods handle");
        }
        else
        {
            memset(result, 0, sizeof(IOTHUBTRANSPORT_AMQP_METHODS));

            if (mallocAndStrcpy_s(&result->device_id, device_id) != 0)
            {
                LogError("Cannot copy device_id");
                free(result);
                result = NULL;
            }
            else if ((module_id != NULL) && (mallocAndStrcpy_s(&result->module_id, module_id) != 0))
            {
                LogError("Cannot copy device_id");
                free(result->device_id);
                free(result);
                result = NULL;
            }
            else
            {
                if (mallocAndStrcpy_s(&result->hostname, hostname) != 0)
                {
                    LogError("Cannot copy hostname");
                    free(result->device_id);
                    free(result->module_id);
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->subscribe_state = SUBSCRIBE_STATE_NOT_SUBSCRIBED;
                    result->method_request_handles = NULL;
                    result->method_request_handle_count = 0;
                    result->receiver_link_disconnected = false;
                    result->sender_link_disconnected = false;
                }
            }
        }
    }

    return result;
}

void iothubtransportamqp_methods_destroy(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle == NULL)
    {
        LogError("NULL handle");
    }
    else
    {
        size_t i;

        if (iothubtransport_amqp_methods_handle->subscribe_state == SUBSCRIBE_STATE_SUBSCRIBED)
        {
            iothubtransportamqp_methods_unsubscribe(iothubtransport_amqp_methods_handle);
        }

        for (i = 0; i < iothubtransport_amqp_methods_handle->method_request_handle_count; i++)
        {
            free(iothubtransport_amqp_methods_handle->method_request_handles[i]);
        }

        if (iothubtransport_amqp_methods_handle->method_request_handles != NULL)
        {
            free(iothubtransport_amqp_methods_handle->method_request_handles);
        }

        free(iothubtransport_amqp_methods_handle->hostname);
        free(iothubtransport_amqp_methods_handle->device_id);
        free(iothubtransport_amqp_methods_handle->module_id);
        free(iothubtransport_amqp_methods_handle);
    }
}

static void call_methods_unsubscribed_if_needed(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE amqp_methods_handle)
{
    if (amqp_methods_handle->receiver_link_disconnected && amqp_methods_handle->sender_link_disconnected)
    {
        amqp_methods_handle->receiver_link_disconnected = false;
        amqp_methods_handle->sender_link_disconnected = false;
        amqp_methods_handle->on_methods_unsubscribed(amqp_methods_handle->on_methods_unsubscribed_context);
    }
}

static void on_message_receiver_state_changed(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    IOTHUBTRANSPORT_AMQP_METHODS_HANDLE amqp_methods_handle = (IOTHUBTRANSPORT_AMQP_METHODS_HANDLE)context;
    if ((new_state != previous_state) &&
        (new_state == MESSAGE_RECEIVER_STATE_ERROR))
    {
        amqp_methods_handle->on_methods_error(amqp_methods_handle->on_methods_error_context);
    }
    else
    {
        if ((new_state == MESSAGE_RECEIVER_STATE_IDLE) && (previous_state == MESSAGE_RECEIVER_STATE_OPEN))
        {
            amqp_methods_handle->receiver_link_disconnected = true;
        }
        call_methods_unsubscribed_if_needed(amqp_methods_handle);
    }
}

static void on_message_sender_state_changed(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    IOTHUBTRANSPORT_AMQP_METHODS_HANDLE amqp_methods_handle = (IOTHUBTRANSPORT_AMQP_METHODS_HANDLE)context;
    if ((new_state != previous_state) &&
        (new_state == MESSAGE_SENDER_STATE_ERROR))
    {
        amqp_methods_handle->on_methods_error(amqp_methods_handle->on_methods_error_context);
    }
    else
    {
        if ((new_state == MESSAGE_SENDER_STATE_IDLE) && (previous_state == MESSAGE_SENDER_STATE_OPEN))
        {
            amqp_methods_handle->sender_link_disconnected = true;
        }
        call_methods_unsubscribed_if_needed(amqp_methods_handle);
    }
}

static void on_message_send_complete(void* context, MESSAGE_SEND_RESULT send_result, AMQP_VALUE delivery_state)
{
    (void)delivery_state;
    if (send_result == MESSAGE_SEND_ERROR)
    {
        IOTHUBTRANSPORT_AMQP_METHODS_HANDLE amqp_methods_handle = (IOTHUBTRANSPORT_AMQP_METHODS_HANDLE)context;
        amqp_methods_handle->on_methods_error(amqp_methods_handle->on_methods_error_context);
    }
}

static AMQP_VALUE on_message_received(const void* context, MESSAGE_HANDLE message)
{
    PROPERTIES_HANDLE properties;
    /* VS believes this is not initialized, so have to set it to the worse case here */
    AMQP_VALUE result = NULL;
    IOTHUBTRANSPORT_AMQP_METHODS_HANDLE amqp_methods_handle = (IOTHUBTRANSPORT_AMQP_METHODS_HANDLE)context;
    MESSAGE_OUTCOME message_outcome;

    if (message == NULL)
    {
        LogError("NULL message");
        message_outcome = MESSAGE_OUTCOME_RELEASED;
    }
    else
    {
        if (message_get_properties(message, &properties) != 0)
        {
            LogError("Cannot retrieve message properties");
            message_outcome = MESSAGE_OUTCOME_REJECTED;
            result = messaging_delivery_rejected("amqp:decode-error", "Cannot retrieve message properties");
        }
        else
        {
            AMQP_VALUE correlation_id;

            if (properties_get_correlation_id(properties, &correlation_id) != 0)
            {
                LogError("Cannot retrieve correlation id");
                message_outcome = MESSAGE_OUTCOME_REJECTED;
                result = messaging_delivery_rejected("amqp:decode-error", "Cannot retrieve correlation id");
            }
            else
            {
                IOTHUBTRANSPORT_AMQP_METHOD* method_handle = (IOTHUBTRANSPORT_AMQP_METHOD*)malloc(sizeof(IOTHUBTRANSPORT_AMQP_METHOD));
                if (method_handle == NULL)
                {
                    LogError("Cannot allocate method handle");
                    message_outcome = MESSAGE_OUTCOME_RELEASED;
                }
                else
                {
                    IOTHUBTRANSPORT_AMQP_METHOD_HANDLE* new_handles;
                    new_handles = (IOTHUBTRANSPORT_AMQP_METHOD_HANDLE*)realloc(amqp_methods_handle->method_request_handles, (amqp_methods_handle->method_request_handle_count + 1) * sizeof(IOTHUBTRANSPORT_AMQP_METHOD_HANDLE));
                    if (new_handles == NULL)
                    {
                        free(method_handle);
                        LogError("Cannot grow method handles array");
                        message_outcome = MESSAGE_OUTCOME_RELEASED;
                    }
                    else
                    {
                        amqp_methods_handle->method_request_handles = new_handles;

                        if (amqpvalue_get_uuid(correlation_id, &method_handle->correlation_id) != 0)
                        {
                            free(method_handle);
                            LogError("Cannot get uuid value for correlation-id");
                            message_outcome = MESSAGE_OUTCOME_REJECTED;
                            result = messaging_delivery_rejected("amqp:decode-error", "Cannot get uuid value for correlation-id");
                        }
                        else
                        {
                            BINARY_DATA binary_data;

                            if (message_get_body_amqp_data_in_place(message, 0, &binary_data) != 0)
                            {
                                free(method_handle);
                                LogError("Cannot get method request message payload");
                                message_outcome = MESSAGE_OUTCOME_REJECTED;
                                result = messaging_delivery_rejected("amqp:decode-error", "Cannot get method request message payload");
                            }
                            else
                            {
                                AMQP_VALUE application_properties;

                                if (message_get_application_properties(message, &application_properties) != 0)
                                {
                                    LogError("Cannot get application properties");
                                    free(method_handle);
                                    message_outcome = MESSAGE_OUTCOME_REJECTED;
                                    result = messaging_delivery_rejected("amqp:decode-error", "Cannot get application properties");
                                }
                                else
                                {
                                    AMQP_VALUE amqp_properties_map = amqpvalue_get_inplace_described_value(application_properties);
                                    if (amqp_properties_map == NULL)
                                    {
                                        LogError("Cannot get application properties map");
                                        free(method_handle);
                                        message_outcome = MESSAGE_OUTCOME_RELEASED;
                                    }
                                    else
                                    {
                                        AMQP_VALUE property_key = amqpvalue_create_string("IoThub-methodname");
                                        if (property_key == NULL)
                                        {
                                            LogError("Cannot create the property key for method name");
                                            free(method_handle);
                                            message_outcome = MESSAGE_OUTCOME_RELEASED;
                                        }
                                        else
                                        {
                                            AMQP_VALUE property_value = amqpvalue_get_map_value(amqp_properties_map, property_key);
                                            if (property_value == NULL)
                                            {
                                                LogError("Cannot find the IoThub-methodname property in the properties map");
                                                free(method_handle);
                                                message_outcome = MESSAGE_OUTCOME_REJECTED;
                                                result = messaging_delivery_rejected("amqp:decode-error", "Cannot find the IoThub-methodname property in the properties map");
                                            }
                                            else
                                            {
                                                const char* method_name;

                                                if (amqpvalue_get_string(property_value, &method_name) != 0)
                                                {
                                                    LogError("Cannot read the method name from the property value");
                                                    free(method_handle);
                                                    message_outcome = MESSAGE_OUTCOME_REJECTED;
                                                    result = messaging_delivery_rejected("amqp:decode-error", "Cannot read the method name from the property value");
                                                }
                                                else
                                                {
                                                    result = messaging_delivery_accepted();
                                                    if (result == NULL)
                                                    {
                                                        LogError("Cannot allocate memory for delivery state");
                                                        free(method_handle);
                                                        message_outcome = MESSAGE_OUTCOME_RELEASED;
                                                    }
                                                    else
                                                    {
                                                        method_handle->iothubtransport_amqp_methods_handle = amqp_methods_handle;

                                                        /* set the method request handle in the handle array */
                                                        amqp_methods_handle->method_request_handles[amqp_methods_handle->method_request_handle_count] = method_handle;
                                                        amqp_methods_handle->method_request_handle_count++;

                                                        if (amqp_methods_handle->on_method_request_received(amqp_methods_handle->on_method_request_received_context, method_name, binary_data.bytes, binary_data.length, method_handle) != 0)
                                                        {
                                                            LogError("Cannot execute the callback with the given data");
                                                            amqpvalue_destroy(result);
                                                            free(method_handle);
                                                            amqp_methods_handle->method_request_handle_count--;
                                                            message_outcome = MESSAGE_OUTCOME_REJECTED;
                                                            result = messaging_delivery_rejected("amqp:internal-error", "Cannot execute the callback with the given data");
                                                        }
                                                        else
                                                        {
                                                            message_outcome = MESSAGE_OUTCOME_ACCEPTED;
                                                        }
                                                    }
                                                }

                                                amqpvalue_destroy(property_value);
                                            }

                                            amqpvalue_destroy(property_key);
                                        }
                                    }

                                    application_properties_destroy(application_properties);
                                }
                            }
                        }
                    }
                }
            }

            properties_destroy(properties);
        }
    }

    switch (message_outcome)
    {
    default:
        break;

    case MESSAGE_OUTCOME_RELEASED:
        result = messaging_delivery_released();

        amqp_methods_handle->on_methods_error(amqp_methods_handle->on_methods_error_context);
        break;

    case MESSAGE_OUTCOME_REJECTED:
    case MESSAGE_OUTCOME_ACCEPTED:
        /* all is well */
        break;
    }

    return result;
}

STRING_HANDLE create_correlation_id(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle->module_id != NULL)
    {
        return STRING_construct_sprintf("%s/%s", iothubtransport_amqp_methods_handle->device_id, iothubtransport_amqp_methods_handle->module_id);
    }
    else
    {
        return STRING_construct(iothubtransport_amqp_methods_handle->device_id);
    }
}

STRING_HANDLE create_peer_endpoint_name(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle->module_id != NULL)
    {
        return STRING_construct_sprintf("amqps://%s/devices/%s/modules/%s/methods/devicebound", iothubtransport_amqp_methods_handle->hostname, iothubtransport_amqp_methods_handle->device_id, iothubtransport_amqp_methods_handle->module_id);
    }
    else
    {
        return STRING_construct_sprintf("amqps://%s/devices/%s/methods/devicebound", iothubtransport_amqp_methods_handle->hostname, iothubtransport_amqp_methods_handle->device_id);
    }
}

STRING_HANDLE create_requests_link_name(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle->module_id != NULL)
    {
        return STRING_construct_sprintf("methods_requests_link-%s/%s", iothubtransport_amqp_methods_handle->device_id, iothubtransport_amqp_methods_handle->module_id);
    }
    else
    {
        return STRING_construct_sprintf("methods_requests_link-%s", iothubtransport_amqp_methods_handle->device_id);
    }
}

STRING_HANDLE create_responses_link_name(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle->module_id != NULL)
    {
        return STRING_construct_sprintf("methods_responses_link-%s/%s", iothubtransport_amqp_methods_handle->device_id, iothubtransport_amqp_methods_handle->module_id);
    }
    else
    {
        return STRING_construct_sprintf("methods_responses_link-%s", iothubtransport_amqp_methods_handle->device_id);
    }
}



static int set_link_attach_properties(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    int result = 0;
    fields link_attach_properties;

    link_attach_properties = amqpvalue_create_map();
    if (link_attach_properties == NULL)
    {
        LogError("Cannot create the map for link attach properties");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_VALUE channel_correlation_id_property_key = amqpvalue_create_symbol("com.microsoft:channel-correlation-id");
        if (channel_correlation_id_property_key == NULL)
        {
            LogError("Cannot create the channel correlation id property key for the link attach properties");
            result = MU_FAILURE;
        }
        else
        {
            STRING_HANDLE correlation_id = NULL;
            AMQP_VALUE channel_correlation_id_property_value = NULL;

            if ((correlation_id = create_correlation_id(iothubtransport_amqp_methods_handle)) == NULL)
            {
                LogError("Cannot create the channel correlation id string for the link attach properties");
                result = MU_FAILURE;
            }
            else if ((channel_correlation_id_property_value = amqpvalue_create_string(STRING_c_str(correlation_id))) == NULL)
            {
                LogError("Cannot create the channel correlation id property key for the link attach properties");
                result = MU_FAILURE;
            }
            else
            {
                if (amqpvalue_set_map_value(link_attach_properties, channel_correlation_id_property_key, channel_correlation_id_property_value) != 0)
                {
                    LogError("Cannot set the property for channel correlation on the link attach properties");
                    result = MU_FAILURE;
                }
                else
                {
                    AMQP_VALUE api_version_property_key = amqpvalue_create_symbol("com.microsoft:api-version");
                    if (api_version_property_key == NULL)
                    {
                        LogError("Cannot create the API version property key for the link attach properties");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        AMQP_VALUE api_version_property_value = amqpvalue_create_string(IOTHUB_API_VERSION);
                        if (api_version_property_value == NULL)
                        {
                            LogError("Cannot create the API version property value for the link attach properties");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            if (amqpvalue_set_map_value(link_attach_properties, api_version_property_key, api_version_property_value) != 0)
                            {
                                LogError("Cannot set the property for API version on the link attach properties");
                                result = MU_FAILURE;
                            }
                            else if (link_set_attach_properties(iothubtransport_amqp_methods_handle->sender_link, link_attach_properties) != 0)
                            {
                                LogError("Cannot set the link attach properties on the sender link");
                                result = MU_FAILURE;
                            }
                            else if (link_set_attach_properties(iothubtransport_amqp_methods_handle->receiver_link, link_attach_properties) != 0)
                            {
                                LogError("Cannot set the link attach properties on the receiver link");
                                result = MU_FAILURE;
                            }
                            else
                            {
                                result = 0;
                            }

                            amqpvalue_destroy(api_version_property_value);
                        }

                        amqpvalue_destroy(api_version_property_key);
                    }
                }

                amqpvalue_destroy(channel_correlation_id_property_value);
            }
            STRING_delete(correlation_id);

            amqpvalue_destroy(channel_correlation_id_property_key);
        }

        amqpvalue_destroy(link_attach_properties);
    }

    return result;
}

int iothubtransportamqp_methods_subscribe(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle,
    SESSION_HANDLE session_handle, ON_METHODS_ERROR on_methods_error, void* on_methods_error_context,
    ON_METHOD_REQUEST_RECEIVED on_method_request_received, void* on_method_request_received_context,
    ON_METHODS_UNSUBSCRIBED on_methods_unsubscribed, void* on_methods_unsubscribed_context)
{
    int result;

    if ((iothubtransport_amqp_methods_handle == NULL) ||
        (session_handle == NULL) ||
        (on_methods_error == NULL) ||
        (on_method_request_received == NULL) ||
        (on_methods_unsubscribed == NULL))
    {
        LogError("Invalid arguments: iothubtransport_amqp_methods_handle=%p, session_handle=%p, on_methods_error=%p, on_method_request_received=%p, on_methods_unsubscribed=%p",
            iothubtransport_amqp_methods_handle, session_handle, on_methods_error, on_method_request_received, on_methods_unsubscribed);
        result = MU_FAILURE;
    }
    else if (iothubtransport_amqp_methods_handle->subscribe_state != SUBSCRIBE_STATE_NOT_SUBSCRIBED)
    {
        LogError("Already subscribed");
        result = MU_FAILURE;
    }
    else
    {
        STRING_HANDLE peer_endpoint_string = create_peer_endpoint_name(iothubtransport_amqp_methods_handle);
        if (peer_endpoint_string == NULL)
        {
            result = MU_FAILURE;
        }
        else
        {
            iothubtransport_amqp_methods_handle->on_method_request_received = on_method_request_received;
            iothubtransport_amqp_methods_handle->on_method_request_received_context = on_method_request_received_context;
            iothubtransport_amqp_methods_handle->on_methods_error = on_methods_error;
            iothubtransport_amqp_methods_handle->on_methods_error_context = on_methods_error_context;
            iothubtransport_amqp_methods_handle->on_methods_unsubscribed = on_methods_unsubscribed;
            iothubtransport_amqp_methods_handle->on_methods_unsubscribed_context = on_methods_unsubscribed_context;

            AMQP_VALUE receiver_source = messaging_create_source(STRING_c_str(peer_endpoint_string));
            if (receiver_source == NULL)
            {
                LogError("Cannot create receiver source");
                result = MU_FAILURE;
            }
            else
            {
                AMQP_VALUE receiver_target = messaging_create_target("requests");
                if (receiver_target == NULL)
                {
                    LogError("Cannot create receiver target");
                    result = MU_FAILURE;
                }
                else
                {
                    STRING_HANDLE requests_link_name = create_requests_link_name(iothubtransport_amqp_methods_handle);
                    if (requests_link_name == NULL)
                    {
                        LogError("Cannot create methods requests link name.");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        iothubtransport_amqp_methods_handle->receiver_link = link_create(session_handle, STRING_c_str(requests_link_name), role_receiver, receiver_source, receiver_target);
                        if (iothubtransport_amqp_methods_handle->receiver_link == NULL)
                        {
                            LogError("Cannot create receiver link");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            AMQP_VALUE sender_source = messaging_create_source("responses");
                            if (sender_source == NULL)
                            {
                                LogError("Cannot create sender source");
                                result = MU_FAILURE;
                            }
                            else
                            {
                                AMQP_VALUE sender_target = messaging_create_target(STRING_c_str(peer_endpoint_string));
                                if (sender_target == NULL)
                                {
                                    LogError("Cannot create sender target");
                                    result = MU_FAILURE;
                                }
                                else
                                {
                                    STRING_HANDLE responses_link_name = create_responses_link_name(iothubtransport_amqp_methods_handle);
                                    if (responses_link_name == NULL)
                                    {
                                        LogError("Cannot create methods responses link name.");
                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        iothubtransport_amqp_methods_handle->sender_link = link_create(session_handle, STRING_c_str(responses_link_name), role_sender, sender_source, sender_target);
                                        if (iothubtransport_amqp_methods_handle->sender_link == NULL)
                                        {
                                            LogError("Cannot create sender link");
                                            result = MU_FAILURE;
                                        }
                                        else
                                        {
                                            if (set_link_attach_properties(iothubtransport_amqp_methods_handle) != 0)
                                            {
                                                result = MU_FAILURE;
                                            }
                                            else
                                            {
                                                iothubtransport_amqp_methods_handle->message_receiver = messagereceiver_create(iothubtransport_amqp_methods_handle->receiver_link, on_message_receiver_state_changed, iothubtransport_amqp_methods_handle);
                                                if (iothubtransport_amqp_methods_handle->message_receiver == NULL)
                                                {
                                                    LogError("Cannot create message receiver");
                                                    result = MU_FAILURE;
                                                }
                                                else
                                                {
                                                    iothubtransport_amqp_methods_handle->message_sender = messagesender_create(iothubtransport_amqp_methods_handle->sender_link, on_message_sender_state_changed, iothubtransport_amqp_methods_handle);
                                                    if (iothubtransport_amqp_methods_handle->message_sender == NULL)
                                                    {
                                                        LogError("Cannot create message sender");
                                                        result = MU_FAILURE;
                                                    }
                                                    else
                                                    {
                                                        if (messagesender_open(iothubtransport_amqp_methods_handle->message_sender) != 0)
                                                        {
                                                            LogError("Cannot open the message sender");
                                                            result = MU_FAILURE;
                                                        }
                                                        else
                                                        {
                                                            if (messagereceiver_open(iothubtransport_amqp_methods_handle->message_receiver, on_message_received, iothubtransport_amqp_methods_handle) != 0)
                                                            {
                                                                LogError("Cannot open the message receiver");
                                                                result = MU_FAILURE;
                                                            }
                                                            else
                                                            {
                                                                iothubtransport_amqp_methods_handle->subscribe_state = SUBSCRIBE_STATE_SUBSCRIBED;

                                                                result = 0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        STRING_delete(responses_link_name);
                                    }

                                    amqpvalue_destroy(sender_target);
                                }

                                amqpvalue_destroy(sender_source);
                            }
                        }

                        STRING_delete(requests_link_name);
                    }

                    amqpvalue_destroy(receiver_target);
                }

                amqpvalue_destroy(receiver_source);
            }

            STRING_delete(peer_endpoint_string);
        }
    }

    return result;
}

void iothubtransportamqp_methods_unsubscribe(IOTHUBTRANSPORT_AMQP_METHODS_HANDLE iothubtransport_amqp_methods_handle)
{
    if (iothubtransport_amqp_methods_handle == NULL)
    {
        LogError("NULL handle");
    }
    else
    {
        if (iothubtransport_amqp_methods_handle->subscribe_state != SUBSCRIBE_STATE_SUBSCRIBED)
        {
            LogError("unsubscribe called while not subscribed");
        }
        else
        {
            messagereceiver_destroy(iothubtransport_amqp_methods_handle->message_receiver);
            iothubtransport_amqp_methods_handle->message_receiver = NULL;
            messagesender_destroy(iothubtransport_amqp_methods_handle->message_sender);
            iothubtransport_amqp_methods_handle->message_sender = NULL;
            link_destroy(iothubtransport_amqp_methods_handle->sender_link);
            iothubtransport_amqp_methods_handle->sender_link = NULL;
            link_destroy(iothubtransport_amqp_methods_handle->receiver_link);
            iothubtransport_amqp_methods_handle->receiver_link = NULL;

            iothubtransport_amqp_methods_handle->subscribe_state = SUBSCRIBE_STATE_NOT_SUBSCRIBED;
        }
    }
}

int iothubtransportamqp_methods_respond(IOTHUBTRANSPORT_AMQP_METHOD_HANDLE method_handle, const unsigned char* response, size_t response_size, int status_code)
{
    int result;

    if (method_handle == NULL)
    {
        LogError("NULL method handle");
        result = MU_FAILURE;
    }
    else if ((response == NULL) && (response_size > 0))
    {
        LogError("NULL response buffer with > 0 response payload size");
        result = MU_FAILURE;
    }
    else
    {
        MESSAGE_HANDLE message = message_create();
        if (message == NULL)
        {
            LogError("Cannot create message");
            result = MU_FAILURE;
        }
        else
        {
            PROPERTIES_HANDLE properties = properties_create();
            if (properties == NULL)
            {
                LogError("Cannot create properties");
                result = MU_FAILURE;
            }
            else
            {
                AMQP_VALUE correlation_id = amqpvalue_create_uuid(method_handle->correlation_id);
                if (correlation_id == NULL)
                {
                    LogError("Cannot create correlation_id value");
                    result = MU_FAILURE;
                }
                else
                {
                    if (properties_set_correlation_id(properties, correlation_id) != 0)
                    {
                        LogError("Cannot set correlation_id on the properties");
                        result = MU_FAILURE;
                    }
                    else if (message_set_properties(message, properties) != 0)
                    {
                        LogError("Cannot set properties on the response message");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        AMQP_VALUE application_properties_map = amqpvalue_create_map();
                        if (application_properties_map == NULL)
                        {
                            LogError("Cannot create map for application properties");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            AMQP_VALUE property_key_status = amqpvalue_create_string("IoThub-status");
                            if (property_key_status == NULL)
                            {
                                LogError("Cannot create the property key for the status property");
                                result = MU_FAILURE;
                            }
                            else
                            {
                                AMQP_VALUE property_value_status = amqpvalue_create_int(status_code);
                                if (property_value_status == NULL)
                                {
                                    LogError("Cannot create the status code property value for the application properties map");
                                    result = MU_FAILURE;
                                }
                                else
                                {
                                    if (amqpvalue_set_map_value(application_properties_map, property_key_status, property_value_status) != 0)
                                    {
                                        LogError("Cannot add the status property to the application properties");
                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        if (message_set_application_properties(message, application_properties_map) != 0)
                                        {
                                            LogError("Cannot set the application properties on the message");
                                            result = MU_FAILURE;
                                        }
                                        else
                                        {
                                            BINARY_DATA binary_data;

                                            binary_data.bytes = response;
                                            binary_data.length = response_size;

                                            if (message_add_body_amqp_data(message, binary_data) != 0)
                                            {
                                                LogError("Cannot set the response payload on the reponse message");
                                                result = MU_FAILURE;
                                            }
                                            else
                                            {
                                                if (messagesender_send_async(method_handle->iothubtransport_amqp_methods_handle->message_sender, message, on_message_send_complete, method_handle->iothubtransport_amqp_methods_handle, 0) == NULL)
                                                {
                                                    LogError("Cannot send response message");
                                                    result = MU_FAILURE;
                                                }
                                                else
                                                {
                                                    remove_tracked_handle(method_handle->iothubtransport_amqp_methods_handle, method_handle);

                                                    free(method_handle);

                                                    result = 0;
                                                }
                                            }
                                        }
                                    }

                                    amqpvalue_destroy(property_value_status);
                                }

                                amqpvalue_destroy(property_key_status);
                            }

                            amqpvalue_destroy(application_properties_map);
                        }
                    }

                    amqpvalue_destroy(correlation_id);
                }

                properties_destroy(properties);
            }

            message_destroy(message);
        }
    }

    return result;
}