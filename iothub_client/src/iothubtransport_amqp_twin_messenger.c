// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_uamqp_c/amqp_definitions_fields.h"
#include "azure_uamqp_c/messaging.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport_amqp_messenger.h"
#include "internal/iothubtransport_amqp_twin_messenger.h"
#include "iothub_client_options.h"
#include "internal/iothub_internal_consts.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TWIN_MESSENGER_SEND_STATUS, TWIN_MESSENGER_SEND_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TWIN_REPORT_STATE_RESULT, TWIN_REPORT_STATE_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TWIN_REPORT_STATE_REASON, TWIN_REPORT_STATE_REASON_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TWIN_MESSENGER_STATE, TWIN_MESSENGER_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TWIN_UPDATE_TYPE, TWIN_UPDATE_TYPE_VALUES);


#define RESULT_OK 0
#define INDEFINITE_TIME ((time_t)(-1))

#define UNIQUE_ID_BUFFER_SIZE                           37

#define EMPTY_TWIN_BODY_DATA                            ((const unsigned char*)" ")
#define EMPTY_TWIN_BODY_SIZE                            1

#define TWIN_MESSAGE_PROPERTY_OPERATION                 "operation"
#define TWIN_MESSAGE_PROPERTY_RESOURCE                  "resource"
#define TWIN_MESSAGE_PROPERTY_VERSION                   "version"
#define TWIN_MESSAGE_PROPERTY_STATUS                    "status"

#define TWIN_RESOURCE_DESIRED                           "/notifications/twin/properties/desired"
#define TWIN_RESOURCE_REPORTED                          "/properties/reported"

#define TWIN_CORRELATION_ID_PROPERTY_NAME               "com.microsoft:channel-correlation-id"
#define TWIN_API_VERSION_PROPERTY_NAME                  "com.microsoft:api-version"
#define TWIN_CORRELATION_ID_PROPERTY_FORMAT             "twin:%s"

#define DEFAULT_MAX_TWIN_SUBSCRIPTION_ERROR_COUNT       3
#define DEFAULT_TWIN_OPERATION_TIMEOUT_SECS             300.0

static char* DEFAULT_TWIN_SEND_LINK_SOURCE_NAME =       "twin";
static char* DEFAULT_TWIN_RECEIVE_LINK_TARGET_NAME =    "twin";

static const char* TWIN_OPERATION_PATCH =               "PATCH";
static const char* TWIN_OPERATION_GET =                 "GET";
static const char* TWIN_OPERATION_PUT =                 "PUT";
static const char* TWIN_OPERATION_DELETE =              "DELETE";


#define TWIN_OPERATION_TYPE_STRINGS \
    TWIN_OPERATION_TYPE_PATCH, \
    TWIN_OPERATION_TYPE_GET, \
    TWIN_OPERATION_TYPE_GET_ON_DEMAND, \
    TWIN_OPERATION_TYPE_PUT, \
    TWIN_OPERATION_TYPE_DELETE

MU_DEFINE_LOCAL_ENUM(TWIN_OPERATION_TYPE, TWIN_OPERATION_TYPE_STRINGS);

#define TWIN_SUBSCRIPTION_STATE_STRINGS \
    TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED, \
    TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES, \
    TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES, \
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES, \
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBING, \
    TWIN_SUBSCRIPTION_STATE_SUBSCRIBED, \
    TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE, \
    TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBING

// Suppress unused function warning for TWIN_SUBSCRIPTION_STATEstrings
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
MU_DEFINE_LOCAL_ENUM(TWIN_SUBSCRIPTION_STATE, TWIN_SUBSCRIPTION_STATE_STRINGS);
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

typedef struct TWIN_MESSENGER_INSTANCE_TAG
{
    pfTransport_GetOption_Product_Info_Callback prod_info_cb;
    void* prod_info_ctx;
    char* device_id;
    char* module_id;
    char* iothub_host_fqdn;

    TWIN_MESSENGER_STATE state;

    SINGLYLINKEDLIST_HANDLE pending_patches;
    SINGLYLINKEDLIST_HANDLE operations;

    TWIN_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
    void* on_state_changed_context;

    TWIN_SUBSCRIPTION_STATE subscription_state;
    size_t subscription_error_count;
    TWIN_STATE_UPDATE_CALLBACK on_message_received_callback;
    void* on_message_received_context;

    AMQP_MESSENGER_HANDLE amqp_msgr;
    AMQP_MESSENGER_STATE amqp_msgr_state;
    bool amqp_msgr_is_subscribed;
} TWIN_MESSENGER_INSTANCE;

typedef struct TWIN_PATCH_OPERATION_CONTEXT_TAG
{
    CONSTBUFFER_HANDLE data;
    TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK on_report_state_complete_callback;
    const void* on_report_state_complete_context;
    time_t time_enqueued;
} TWIN_PATCH_OPERATION_CONTEXT;

typedef struct TWIN_OPERATION_CONTEXT_TAG
{
    TWIN_OPERATION_TYPE type;
    TWIN_MESSENGER_INSTANCE* msgr;
    char* correlation_id;
    union {
        struct REPORTED_PROPERTIES_TAG
        {
            TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK callback;
            const void* context;
        } reported_properties;

        struct GET_TWIN_TAG
        {
            TWIN_STATE_UPDATE_CALLBACK callback;
            const void* context;
        } get_twin;
    } cb;
    time_t time_sent;
} TWIN_OPERATION_CONTEXT;




static void update_state(TWIN_MESSENGER_INSTANCE* twin_msgr, TWIN_MESSENGER_STATE new_state)
{
    if (new_state != twin_msgr->state)
    {
        TWIN_MESSENGER_STATE previous_state = twin_msgr->state;
        twin_msgr->state = new_state;

        if (twin_msgr->on_state_changed_callback != NULL)
        {
            twin_msgr->on_state_changed_callback(twin_msgr->on_state_changed_context, previous_state, new_state);
        }
    }
}

//---------- AMQP Helper Functions ----------//

static int set_message_correlation_id(MESSAGE_HANDLE message, const char* value)
{
    int result;
    PROPERTIES_HANDLE properties = NULL;

    if (message_get_properties(message, &properties) != 0)
    {
        LogError("Failed getting the AMQP message properties");
        result = MU_FAILURE;
    }
    else if (properties == NULL && (properties = properties_create()) == NULL)
    {
        LogError("Failed creating properties for AMQP message");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_VALUE amqp_value;

        if ((amqp_value = amqpvalue_create_string(value)) == NULL)
        {
            LogError("Failed creating AMQP value for correlation-id");
            result = MU_FAILURE;
        }
        else
        {
            if (properties_set_correlation_id(properties, amqp_value) != RESULT_OK)
            {
                LogError("Failed setting the correlation id");
                result = MU_FAILURE;
            }
            else if (message_set_properties(message, properties) != RESULT_OK)
            {
                LogError("Failed setting the AMQP message properties");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }

            amqpvalue_destroy(amqp_value);
        }

        properties_destroy(properties);
    }

    return result;
}

static int get_message_correlation_id(MESSAGE_HANDLE message, char** correlation_id)
{
    int result;

    PROPERTIES_HANDLE properties;
    AMQP_VALUE amqp_value;

    if (message_get_properties(message, &properties) != 0)
    {
        LogError("Failed getting AMQP message properties");
        result = MU_FAILURE;
    }
    else if (properties == NULL)
    {
        *correlation_id = NULL;
        result = RESULT_OK;
    }
    else
    {
        if (properties_get_correlation_id(properties, &amqp_value) != 0 || amqp_value == NULL)
        {
            *correlation_id = NULL;
            result = RESULT_OK;
        }
        else
        {
            const char* value;

            if (amqpvalue_get_string(amqp_value, &value) != 0)
            {
                LogError("Failed retrieving string from AMQP value");
                result = MU_FAILURE;
            }
            else if (mallocAndStrcpy_s(correlation_id, value) != 0)
            {
                LogError("Failed cloning correlation-id");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }

        properties_destroy(properties);
    }

    return result;
}

static int add_map_item(AMQP_VALUE map, const char* name, const char* value)
{
    int result;
    AMQP_VALUE amqp_value_name;

    if ((amqp_value_name = amqpvalue_create_symbol(name)) == NULL)
    {
        LogError("Failed creating AMQP_VALUE for name");
        result = MU_FAILURE;
    }
    else
    {
        AMQP_VALUE amqp_value_value = NULL;

        if (value == NULL && (amqp_value_value = amqpvalue_create_null()) == NULL)
        {
            LogError("Failed creating AMQP_VALUE for NULL value");
            result = MU_FAILURE;
        }
        else if (value != NULL && (amqp_value_value = amqpvalue_create_string(value)) == NULL)
        {
            LogError("Failed creating AMQP_VALUE for value");
            result = MU_FAILURE;
        }
        else
        {
            if (amqpvalue_set_map_value(map, amqp_value_name, amqp_value_value) != 0)
            {
                LogError("Failed adding key/value pair to map");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }

            amqpvalue_destroy(amqp_value_value);
        }

        amqpvalue_destroy(amqp_value_name);
    }

    return result;
}

static int add_amqp_message_annotation(MESSAGE_HANDLE message, AMQP_VALUE msg_annotations_map)
{
    int result;
    AMQP_VALUE msg_annotations;

    if ((msg_annotations = amqpvalue_create_message_annotations(msg_annotations_map)) == NULL)
    {
        LogError("Failed creating new AMQP message annotations");
        result = MU_FAILURE;
    }
    else
    {
        if (message_set_message_annotations(message, (annotations)msg_annotations) != 0)
        {
            LogError("Failed setting AMQP message annotations");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        annotations_destroy(msg_annotations);
    }

    return result;
}


//---------- TWIN Helpers ----------//

static char* generate_unique_id()
{
    char* result;

    if ((result = (char*)malloc(sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1)) == NULL)
    {
        LogError("Failed generating an unique tag (malloc failed)");
    }
    else
    {
        memset(result, 0, sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1);

        if (UniqueId_Generate(result, UNIQUE_ID_BUFFER_SIZE) != UNIQUEID_OK)
        {
            LogError("Failed generating an unique tag (UniqueId_Generate failed)");
            free(result);
            result = NULL;
        }
    }

    return result;
}

static char* generate_twin_correlation_id()
{
    char* result;
    char* unique_id;

    if ((unique_id = generate_unique_id()) == NULL)
    {
        LogError("Failed generating unique ID for correlation-id");
        result = NULL;
    }
    else
    {
        if ((result = (char*)malloc(strlen(TWIN_CORRELATION_ID_PROPERTY_FORMAT) + strlen(unique_id) + 1)) == NULL)
        {
            LogError("Failed allocating correlation-id");
            result = NULL;
        }
        else
        {
            (void)sprintf(result, TWIN_CORRELATION_ID_PROPERTY_FORMAT, unique_id);
        }

        free(unique_id);
    }

    return result;
}

static TWIN_OPERATION_CONTEXT* create_twin_operation_context(TWIN_MESSENGER_INSTANCE* twin_msgr, TWIN_OPERATION_TYPE type)
{
    TWIN_OPERATION_CONTEXT* result;

    if ((result = (TWIN_OPERATION_CONTEXT*)malloc(sizeof(TWIN_OPERATION_CONTEXT))) == NULL)
    {
        LogError("Failed creating context for %s (%s)", MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, type), twin_msgr->device_id);
    }
    else
    {
        memset(result, 0, sizeof(TWIN_OPERATION_CONTEXT));

        if ((result->correlation_id = generate_unique_id()) == NULL)
        {
            LogError("Failed setting context correlation-id (%s, %s)", MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, type), twin_msgr->device_id);
            free(result);
            result = NULL;
        }
        else
        {
            result->type = type;
            result->msgr = twin_msgr;
        }
    }

    return result;
}

static bool find_twin_operation_by_correlation_id(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    TWIN_OPERATION_CONTEXT* twin_op_ctx = (TWIN_OPERATION_CONTEXT*)singlylinkedlist_item_get_value(list_item);
    return (twin_op_ctx == NULL) ? false : (strcmp(twin_op_ctx->correlation_id, (const char*)match_context) == 0);
}

static bool find_twin_operation_by_type(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    TWIN_OPERATION_CONTEXT* twin_op_ctx = (TWIN_OPERATION_CONTEXT*)singlylinkedlist_item_get_value(list_item);
    return (twin_op_ctx == NULL) ? false : (twin_op_ctx->type == *(TWIN_OPERATION_TYPE*)match_context);
}

static void destroy_twin_operation_context(TWIN_OPERATION_CONTEXT* op_ctx)
{
    free(op_ctx->correlation_id);
    free(op_ctx);
}

static int add_twin_operation_context_to_queue(TWIN_OPERATION_CONTEXT* twin_op_ctx)
{
    int result;

    if (singlylinkedlist_add(twin_op_ctx->msgr->operations, (const void*)twin_op_ctx) == NULL)
    {
        LogError("Failed adding TWIN operation context to queue (%s, %s)", MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, twin_op_ctx->type), twin_op_ctx->correlation_id);
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int remove_twin_operation_context_from_queue(TWIN_OPERATION_CONTEXT* twin_op_ctx)
{
    int result;
    LIST_ITEM_HANDLE list_item;

    if ((list_item = singlylinkedlist_find(twin_op_ctx->msgr->operations, find_twin_operation_by_correlation_id, (const void*)twin_op_ctx->correlation_id)) == NULL)
    {
        result = RESULT_OK;
    }
    else if (singlylinkedlist_remove(twin_op_ctx->msgr->operations, list_item) != 0)
    {
        LogError("Failed removing TWIN operation context from queue (%s, %s, %s)",
            twin_op_ctx->msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, twin_op_ctx->type), twin_op_ctx->correlation_id);
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}


//---------- TWIN <-> AMQP Translation Functions ----------//

static int parse_incoming_twin_message(MESSAGE_HANDLE message,
    char** correlation_id,
    bool* has_version, int64_t* version,
    bool* has_status_code, int* status_code,
    bool* has_twin_report, BINARY_DATA* twin_report)
{
    int result;

    if (get_message_correlation_id(message, correlation_id) != 0)
    {
        LogError("Failed retrieving correlation ID from received TWIN message.");
        result = MU_FAILURE;
    }
    else
    {
        annotations message_annotations;

        if (message_get_message_annotations(message, &message_annotations) != 0)
        {
            LogError("Failed getting TWIN message annotations");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;

            if (message_annotations == NULL)
            {
                *has_version = false;
                *has_status_code = false;
            }
            else
            {
                uint32_t pair_count;
                if (amqpvalue_get_map_pair_count((AMQP_VALUE)message_annotations, &pair_count) != 0)
                {
                    LogError("Failed getting TWIN message annotations count");
                    result = MU_FAILURE;
                }
                else
                {
                    uint32_t i;

                    *has_status_code = false;
                    *has_version = false;

                    for (i = 0; i < pair_count; i++)
                    {
                        AMQP_VALUE amqp_map_key;
                        AMQP_VALUE amqp_map_value;

                        if (amqpvalue_get_map_key_value_pair((AMQP_VALUE)message_annotations, i, &amqp_map_key, &amqp_map_value) != 0)
                        {
                            LogError("Failed getting AMQP map key/value pair (%d)", i);
                            result = MU_FAILURE;
                        }
                        else
                        {
                            const char* map_key_name;

                            if (amqpvalue_get_symbol(amqp_map_key, &map_key_name) != 0)
                            {
                                LogError("Failed getting AMQP value symbol");
                                result = MU_FAILURE;
                                break;
                            }
                            else
                            {
                                if (strcmp(TWIN_MESSAGE_PROPERTY_STATUS, map_key_name) == 0)
                                {
                                    if (amqpvalue_get_type(amqp_map_value) != AMQP_TYPE_INT)
                                    {
                                        LogError("TWIN message status property expected to be INT");
                                        result = MU_FAILURE;
                                        break;
                                    }
                                    else if (amqpvalue_get_int(amqp_map_value, status_code) != 0)
                                    {
                                        LogError("Failed getting TWIN message status code value");
                                        result = MU_FAILURE;
                                        break;
                                    }
                                    else
                                    {
                                        *has_status_code = true;
                                    }
                                }
                                else if (strcmp(TWIN_MESSAGE_PROPERTY_VERSION, map_key_name) == 0)
                                {
                                    if (amqpvalue_get_type(amqp_map_value) != AMQP_TYPE_LONG)
                                    {
                                        LogError("TWIN message version property expected to be LONG");
                                        result = MU_FAILURE;
                                        break;
                                    }
                                    else if (amqpvalue_get_long(amqp_map_value, version) != 0)
                                    {
                                        LogError("Failed getting TWIN message version value");
                                        result = MU_FAILURE;
                                        break;
                                    }
                                    else
                                    {
                                        *has_version = true;
                                    }
                                }

                                amqpvalue_destroy(amqp_map_value);
                            }

                            amqpvalue_destroy(amqp_map_key);
                        }
                    }
                }

                amqpvalue_destroy(message_annotations);
            }

            if (result == RESULT_OK)
            {
                MESSAGE_BODY_TYPE body_type;

                if (message_get_body_type(message, &body_type) != 0)
                {
                    LogError("Failed getting TWIN message body type");
                    result = MU_FAILURE;
                }
                else if (body_type == MESSAGE_BODY_TYPE_DATA)
                {
                    size_t body_count;

                    if (message_get_body_amqp_data_count(message, &body_count) != 0)
                    {
                        LogError("Failed getting TWIN message body count");
                        result = MU_FAILURE;
                    }
                    else if (body_count != 1)
                    {
                        LogError("Unexpected number of TWIN message bodies (%lu)", (unsigned long)body_count);
                        result = MU_FAILURE;
                    }
                    else if (message_get_body_amqp_data_in_place(message, 0, twin_report) != 0)
                    {
                        LogError("Failed getting TWIN message body");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        *has_twin_report = true;
                    }
                }
                else if (body_type != MESSAGE_BODY_TYPE_NONE)
                {
                    LogError("Unexpected TWIN message body %d", body_type);
                    result = MU_FAILURE;
                }
                else
                {
                    *has_twin_report = false;
                }
            }
        }

        if (result != RESULT_OK)
        {
            free(*correlation_id);
            *correlation_id = NULL;
        }
    }

    return result;
}

static void destroy_link_attach_properties(MAP_HANDLE properties)
{
    Map_Destroy(properties);
}

static MAP_HANDLE create_link_attach_properties(TWIN_MESSENGER_INSTANCE* twin_msgr)
{
    MAP_HANDLE result;

    if ((result = Map_Create(NULL)) == NULL)
    {
        LogError("Failed creating map for AMQP link properties (%s)", twin_msgr->device_id);
    }
    else
    {
        char* correlation_id;

        if ((correlation_id = generate_twin_correlation_id()) == NULL)
        {
            LogError("Failed adding AMQP link property ");
            destroy_link_attach_properties(result);
            result = NULL;
        }
        else
        {
            if (Map_Add(result, CLIENT_VERSION_PROPERTY_NAME, twin_msgr->prod_info_cb(twin_msgr->prod_info_ctx) ) != MAP_OK)
            {
                LogError("Failed adding AMQP link property 'client version' (%s)", twin_msgr->device_id);
                destroy_link_attach_properties(result);
                result = NULL;
            }
            else if (Map_Add(result, TWIN_CORRELATION_ID_PROPERTY_NAME, correlation_id) != MAP_OK)
            {
                LogError("Failed adding AMQP link property 'correlation-id' (%s)", twin_msgr->device_id);
                destroy_link_attach_properties(result);
                result = NULL;
            }
            else if (Map_Add(result, TWIN_API_VERSION_PROPERTY_NAME, IOTHUB_API_VERSION) != MAP_OK)
            {
                LogError("Failed adding AMQP link property 'api-version' (%s)", twin_msgr->device_id);
                destroy_link_attach_properties(result);
                result = NULL;
            }

            free(correlation_id);
        }
    }

    return result;
}

static const char* get_twin_operation_name(TWIN_OPERATION_TYPE op_type)
{
    const char* result;

    switch (op_type)
    {
        default:
            LogError("Unrecognized TWIN operation (%s)", MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_type));
            result = NULL;
            break;
        case TWIN_OPERATION_TYPE_PATCH:
            result = TWIN_OPERATION_PATCH;
            break;
        case TWIN_OPERATION_TYPE_GET:
        case TWIN_OPERATION_TYPE_GET_ON_DEMAND:
            result = TWIN_OPERATION_GET;
            break;
        case TWIN_OPERATION_TYPE_PUT:
            result = TWIN_OPERATION_PUT;
            break;
        case TWIN_OPERATION_TYPE_DELETE:
            result = TWIN_OPERATION_DELETE;
            break;
    }

    return result;
}

static MESSAGE_HANDLE create_amqp_message_for_twin_operation(TWIN_OPERATION_TYPE op_type, char* correlation_id, CONSTBUFFER_HANDLE data)
{
    MESSAGE_HANDLE result;
    const char* twin_op_name;

    if ((twin_op_name = get_twin_operation_name(op_type))== NULL)
    {
        result = NULL;
    }
    else if ((result = message_create()) == NULL)
    {
        LogError("Failed creating AMQP message (%s)", twin_op_name);
    }
    else
    {
        AMQP_VALUE msg_annotations_map;

        if ((msg_annotations_map = amqpvalue_create_map()) == NULL)
        {
            LogError("Failed creating map for message annotations");
            message_destroy(result);
            result = NULL;
        }
        else
        {
            if (add_map_item(msg_annotations_map, TWIN_MESSAGE_PROPERTY_OPERATION, twin_op_name) != RESULT_OK)
            {
                LogError("Failed adding operation to AMQP message annotations (%s)", twin_op_name);
                message_destroy(result);
                result = NULL;
            }
            else if ((op_type == TWIN_OPERATION_TYPE_PATCH) &&
                add_map_item(msg_annotations_map, TWIN_MESSAGE_PROPERTY_RESOURCE, TWIN_RESOURCE_REPORTED) != RESULT_OK)
            {
                LogError("Failed adding resource to AMQP message annotations (%s)", twin_op_name);
                message_destroy(result);
                result = NULL;
            }
            else if ((op_type == TWIN_OPERATION_TYPE_PUT || op_type == TWIN_OPERATION_TYPE_DELETE) &&
                add_map_item(msg_annotations_map, TWIN_MESSAGE_PROPERTY_RESOURCE, TWIN_RESOURCE_DESIRED) != RESULT_OK)
            {
                LogError("Failed adding resource to AMQP message annotations (%s)", twin_op_name);
                message_destroy(result);
                result = NULL;
            }
            else if (add_amqp_message_annotation(result, msg_annotations_map) != RESULT_OK)
            {
                LogError("Failed adding annotations to AMQP message (%s)", twin_op_name);
                message_destroy(result);
                result = NULL;
            }
            else if (set_message_correlation_id(result, correlation_id) != RESULT_OK)
            {
                LogError("Failed AMQP message correlation-id (%s)", twin_op_name);
                message_destroy(result);
                result = NULL;
            }
            else
            {
                BINARY_DATA binary_data;

                if (data != NULL)
                {
                    const CONSTBUFFER* data_buffer;
                    data_buffer = CONSTBUFFER_GetContent(data);
                    binary_data.bytes = data_buffer->buffer;
                    binary_data.length = data_buffer->size;
                }
                else
                {
                    binary_data.bytes = EMPTY_TWIN_BODY_DATA;
                    binary_data.length = EMPTY_TWIN_BODY_SIZE;
                }

                if (message_add_body_amqp_data(result, binary_data) != 0)
                {
                    LogError("Failed adding twin patch data to AMQP message body");
                    message_destroy(result);
                    result = NULL;
                }
            }

            amqpvalue_destroy(msg_annotations_map);
        }
    }

    return result;
}

static TWIN_REPORT_STATE_RESULT get_twin_messenger_result_from(AMQP_MESSENGER_SEND_RESULT amqp_send_result)
{
    TWIN_REPORT_STATE_RESULT result;

    switch (amqp_send_result)
    {
        case AMQP_MESSENGER_SEND_RESULT_SUCCESS:
            result = TWIN_REPORT_STATE_RESULT_SUCCESS;
            break;
        case AMQP_MESSENGER_SEND_RESULT_CANCELLED:
            result = TWIN_REPORT_STATE_RESULT_CANCELLED;
            break;
        case AMQP_MESSENGER_SEND_RESULT_ERROR:
            result = TWIN_REPORT_STATE_RESULT_ERROR;
            break;
        default:
            LogError("Unrecognized enum value %s", MU_ENUM_TO_STRING(AMQP_MESSENGER_SEND_RESULT, amqp_send_result));
            result = TWIN_REPORT_STATE_RESULT_ERROR;
            break;
    };

    return result;
}

static TWIN_REPORT_STATE_REASON get_twin_messenger_reason_from(AMQP_MESSENGER_REASON amqp_reason)
{
    TWIN_REPORT_STATE_REASON result;

    switch (amqp_reason)
    {
        case AMQP_MESSENGER_REASON_NONE:
            result = TWIN_REPORT_STATE_REASON_NONE;
            break;
        case AMQP_MESSENGER_REASON_FAIL_SENDING:
            result = TWIN_REPORT_STATE_REASON_FAIL_SENDING;
            break;
        case AMQP_MESSENGER_REASON_TIMEOUT:
            result = TWIN_REPORT_STATE_REASON_TIMEOUT;
            break;
        case AMQP_MESSENGER_REASON_MESSENGER_DESTROYED:
            result = TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED;
            break;
        case AMQP_MESSENGER_REASON_CANNOT_PARSE:
            result = TWIN_REPORT_STATE_REASON_NONE;
            break;
        default:
            LogError("Unrecognized enum value %s (%d)", MU_ENUM_TO_STRING(AMQP_MESSENGER_REASON, amqp_reason), amqp_reason);
            result = TWIN_REPORT_STATE_REASON_NONE;
            break;
    };

    return result;
}

static void on_amqp_send_complete_callback(AMQP_MESSENGER_SEND_RESULT result, AMQP_MESSENGER_REASON reason, void* context)
{
    // Applicable to TWIN requests for reported state PATCH, GET, PUT, DELETE.

    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        TWIN_OPERATION_CONTEXT* twin_op_ctx = (TWIN_OPERATION_CONTEXT*)context;

        if (result != AMQP_MESSENGER_SEND_RESULT_SUCCESS)
        {
            if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PATCH)
            {
                if (twin_op_ctx->cb.reported_properties.callback != NULL)
                {
                    TWIN_REPORT_STATE_RESULT callback_result;
                    TWIN_REPORT_STATE_REASON callback_reason;

                    callback_result = get_twin_messenger_result_from(result);
                    callback_reason = get_twin_messenger_reason_from(reason);

                    twin_op_ctx->cb.reported_properties.callback(callback_result, callback_reason, 0, (void*)twin_op_ctx->cb.reported_properties.context);
                }
            }
            else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET_ON_DEMAND)
            {
                if (twin_op_ctx->cb.get_twin.callback != NULL)
                {
                    twin_op_ctx->cb.get_twin.callback(TWIN_UPDATE_TYPE_COMPLETE, NULL, 0, (void*)twin_op_ctx->cb.get_twin.context);
                }
            }
            else if (reason != AMQP_MESSENGER_REASON_MESSENGER_DESTROYED)
            {
                LogError("Failed sending TWIN operation request (%s, %s, %s, %s, %s)",
                    twin_op_ctx->msgr->device_id,
                    MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, twin_op_ctx->type),
                    twin_op_ctx->correlation_id,
                    MU_ENUM_TO_STRING(AMQP_MESSENGER_SEND_RESULT, result), MU_ENUM_TO_STRING(AMQP_MESSENGER_REASON, reason));

                if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET &&
                    twin_op_ctx->msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES)
                {
                    twin_op_ctx->msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
                    twin_op_ctx->msgr->subscription_error_count++;
                }
                else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PUT &&
                    twin_op_ctx->msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES)
                {
                    twin_op_ctx->msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
                    twin_op_ctx->msgr->subscription_error_count++;
                }
            }

            if (remove_twin_operation_context_from_queue(twin_op_ctx) != RESULT_OK)
            {
                update_state(twin_op_ctx->msgr, TWIN_MESSENGER_STATE_ERROR);
            }
            else
            {
                destroy_twin_operation_context(twin_op_ctx);
            }
        }
    }
}

static int send_twin_operation_request(TWIN_MESSENGER_INSTANCE* twin_msgr, TWIN_OPERATION_CONTEXT* op_ctx, CONSTBUFFER_HANDLE data)
{
    int result;
    MESSAGE_HANDLE amqp_message;

    if ((amqp_message = create_amqp_message_for_twin_operation(op_ctx->type, op_ctx->correlation_id, data)) == NULL)
    {
        LogError("Failed creating request message (%s, %s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_ctx->type), op_ctx->correlation_id);
        result = MU_FAILURE;
    }
    else
    {
        if ((op_ctx->time_sent = get_time(NULL)) == INDEFINITE_TIME)
        {
            LogError("Failed setting TWIN operation sent time (%s, %s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_ctx->type), op_ctx->correlation_id);
            result = MU_FAILURE;
        }
        else if (amqp_messenger_send_async(twin_msgr->amqp_msgr, amqp_message, on_amqp_send_complete_callback, (void*)op_ctx) != 0)
        {
            LogError("Failed sending request message for (%s, %s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_ctx->type), op_ctx->correlation_id);
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        message_destroy(amqp_message);
    }

    return result;
}


//---------- Internal Helpers----------//

static bool remove_expired_twin_patch_request(const void* item, const void* match_context, bool* continue_processing)
{
    bool remove_item;

    if (item == NULL || match_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid argument (item=%p, match_context=%p, continue_processing=%p)", item, match_context, continue_processing);
        remove_item = false;
    }
    else
    {

        time_t current_time = *(time_t*)match_context;
        TWIN_PATCH_OPERATION_CONTEXT* twin_patch_ctx = (TWIN_PATCH_OPERATION_CONTEXT*)item;

        if (get_difftime(current_time, twin_patch_ctx->time_enqueued) >= DEFAULT_TWIN_OPERATION_TIMEOUT_SECS)
        {
            remove_item = true;
            *continue_processing = true;

            if (twin_patch_ctx->on_report_state_complete_callback != NULL)
            {
                twin_patch_ctx->on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_TIMEOUT, 0, twin_patch_ctx->on_report_state_complete_context);
            }

            CONSTBUFFER_DecRef(twin_patch_ctx->data);
            free(twin_patch_ctx);
        }
        else
        {
            remove_item = false;
            *continue_processing = false;
        }
    }

    return remove_item;
}

static bool remove_expired_twin_operation_request(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;

    if (item == NULL || match_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid argument (item=%p, match_context=%p, continue_processing=%p)", item, match_context, continue_processing);
        result = false;
    }
    else
    {
        TWIN_OPERATION_CONTEXT* twin_op_ctx = (TWIN_OPERATION_CONTEXT*)item;
        TWIN_MESSENGER_INSTANCE* twin_msgr = twin_op_ctx->msgr;
        time_t current_time = *(time_t*)match_context;

        if (get_difftime(current_time, twin_op_ctx->time_sent) < DEFAULT_TWIN_OPERATION_TIMEOUT_SECS)
        {
            result = false;
            // All next elements in the list have a later time_sent, so they won't be expired, and don't need to be removed.
            *continue_processing = false;
        }
        else
        {
            LogError("Twin operation timed out (%s, %s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, twin_op_ctx->type), twin_op_ctx->correlation_id);
            result = true;
            *continue_processing = true;

            if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PATCH)
            {
                if (twin_op_ctx->cb.reported_properties.callback != NULL)
                {
                    twin_op_ctx->cb.reported_properties.callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_TIMEOUT, 0, twin_op_ctx->cb.reported_properties.context);
                }
            }
            else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET)
            {
                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES)
                {
                    twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
                    twin_msgr->subscription_error_count++;
                }
            }
            else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PUT)
            {
                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_SUBSCRIBING)
                {
                    twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES;
                    twin_msgr->subscription_error_count++;
                }
            }
            else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_DELETE)
            {
                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBING)
                {
                    twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE;
                    twin_msgr->subscription_error_count++;
                }
            }
            else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET_ON_DEMAND)
            {
                twin_op_ctx->cb.get_twin.callback(TWIN_UPDATE_TYPE_COMPLETE, NULL, 0, twin_op_ctx->cb.get_twin.context);
            }

            destroy_twin_operation_context(twin_op_ctx);
        }
    }

    return result;
}

static void process_timeouts(TWIN_MESSENGER_INSTANCE* twin_msgr)
{
    time_t current_time;

    if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failed obtaining current time (%s)", twin_msgr->device_id);
        update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
    }
    else
    {
        (void)singlylinkedlist_remove_if(twin_msgr->pending_patches, remove_expired_twin_patch_request, (const void*)&current_time);
        (void)singlylinkedlist_remove_if(twin_msgr->operations, remove_expired_twin_operation_request, (const void*)&current_time);
    }
}

static bool send_pending_twin_patch(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;

    if (item == NULL || match_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid argument (item=%p, match_context=%p, continue_processing=%p)", item, match_context, continue_processing);
        result = false;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)match_context;
        TWIN_PATCH_OPERATION_CONTEXT* twin_patch_ctx = (TWIN_PATCH_OPERATION_CONTEXT*)item;
        TWIN_OPERATION_CONTEXT* twin_op_ctx;

        if ((twin_op_ctx = create_twin_operation_context(twin_msgr, TWIN_OPERATION_TYPE_PATCH)) == NULL)
        {
            LogError("Failed creating context for sending reported state (%s)", twin_msgr->device_id);

            if (twin_patch_ctx->on_report_state_complete_callback != NULL)
            {
                twin_patch_ctx->on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_INTERNAL_ERROR, 0, twin_patch_ctx->on_report_state_complete_context);
            }
        }
        else if (add_twin_operation_context_to_queue(twin_op_ctx) != RESULT_OK)
        {
            LogError("Failed adding TWIN operation context to queue (%s)", twin_msgr->device_id);

            if (twin_patch_ctx->on_report_state_complete_callback != NULL)
            {
                twin_patch_ctx->on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_INTERNAL_ERROR, 0, twin_patch_ctx->on_report_state_complete_context);
            }

            destroy_twin_operation_context(twin_op_ctx);
        }
        else
        {
            twin_op_ctx->cb.reported_properties.callback = twin_patch_ctx->on_report_state_complete_callback;
            twin_op_ctx->cb.reported_properties.context = twin_patch_ctx->on_report_state_complete_context;

            if (send_twin_operation_request(twin_msgr, twin_op_ctx, twin_patch_ctx->data) != RESULT_OK)
            {
                LogError("Failed sending reported state (%s)", twin_msgr->device_id);

                if (twin_patch_ctx->on_report_state_complete_callback != NULL)
                {
                    twin_patch_ctx->on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_FAIL_SENDING, 0, twin_patch_ctx->on_report_state_complete_context);
                }

                (void)remove_twin_operation_context_from_queue(twin_op_ctx);
                destroy_twin_operation_context(twin_op_ctx);
            }
        }

        CONSTBUFFER_DecRef(twin_patch_ctx->data);
        free(twin_patch_ctx);

        *continue_processing = true;
        result = true;
    }

    return result;
}

static void process_twin_subscription(TWIN_MESSENGER_INSTANCE* twin_msgr)
{
    if (twin_msgr->subscription_error_count >= DEFAULT_MAX_TWIN_SUBSCRIPTION_ERROR_COUNT)
    {
        LogError("Maximum number of TWIN subscription-related failures reached (%s, %lu)", twin_msgr->device_id, (unsigned long)twin_msgr->subscription_error_count);
        update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
    }
    else
    {
        TWIN_OPERATION_TYPE op_type = TWIN_OPERATION_TYPE_PATCH;
        TWIN_SUBSCRIPTION_STATE next_subscription_state = twin_msgr->subscription_state;

        if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES)
        {
            op_type = TWIN_OPERATION_TYPE_GET;
            next_subscription_state = TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES;
        }
        else if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES)
        {
            op_type = TWIN_OPERATION_TYPE_PUT;
            next_subscription_state = TWIN_SUBSCRIPTION_STATE_SUBSCRIBING;
        }
        else if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE)
        {
            op_type = TWIN_OPERATION_TYPE_PUT;
            next_subscription_state = TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBING;
        }

        if (next_subscription_state != twin_msgr->subscription_state)
        {
            TWIN_OPERATION_CONTEXT* twin_op_ctx;

            if ((twin_op_ctx = create_twin_operation_context(twin_msgr, op_type)) == NULL)
            {
                LogError("Failed creating a context for TWIN request (%s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_type));
                update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
            }
            else
            {
                if (add_twin_operation_context_to_queue(twin_op_ctx) != RESULT_OK)
                {
                    LogError("Failed queueing TWIN request context (%s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_type));
                    destroy_twin_operation_context(twin_op_ctx);
                    update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
                }
                else if (send_twin_operation_request(twin_msgr, twin_op_ctx, NULL) != RESULT_OK)
                {
                    LogError("Failed sending TWIN request (%s, %s)", twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_OPERATION_TYPE, op_type));

                    (void)remove_twin_operation_context_from_queue(twin_op_ctx);
                    destroy_twin_operation_context(twin_op_ctx);
                    update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
                }
                else
                {
                    twin_msgr->subscription_state = next_subscription_state;
                }
            }
        }
    }
}

static bool cancel_all_pending_twin_operations(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;

    if (item == NULL || continue_processing == NULL)
    {
        LogError("Invalid argument (item=%p, continue_processing=%p)", item, continue_processing);
        result = false;
    }
    else
    {
        TWIN_OPERATION_CONTEXT* twin_op_ctx = (TWIN_OPERATION_CONTEXT*)item;
        (void)match_context;

        if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PATCH)
        {
            if (twin_op_ctx->cb.reported_properties.callback != NULL)
            {
                twin_op_ctx->cb.reported_properties.callback(TWIN_REPORT_STATE_RESULT_CANCELLED, TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED, 0, twin_op_ctx->cb.reported_properties.context);
            }
        }

        destroy_twin_operation_context(twin_op_ctx);

        *continue_processing = true;
        result = true;
    }

    return result;
}

static bool cancel_pending_twin_patch_operation(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;

    if (item == NULL)
    {
        LogError("Invalid argument (item is NULL)");
        *continue_processing = false;
        result = false;
    }
    else
    {
        TWIN_PATCH_OPERATION_CONTEXT* twin_patch_ctx = (TWIN_PATCH_OPERATION_CONTEXT*)item;
        (void)match_context;

        if (twin_patch_ctx->on_report_state_complete_callback != NULL)
        {
            twin_patch_ctx->on_report_state_complete_callback(TWIN_REPORT_STATE_RESULT_CANCELLED, TWIN_REPORT_STATE_REASON_MESSENGER_DESTROYED, 0, twin_patch_ctx->on_report_state_complete_context);
        }

        CONSTBUFFER_DecRef(twin_patch_ctx->data);
        free(twin_patch_ctx);

        *continue_processing = true;
        result = true;
    }

    return result;
}

static void internal_twin_messenger_destroy(TWIN_MESSENGER_INSTANCE* twin_msgr)
{
    if (twin_msgr->amqp_msgr != NULL)
    {
        amqp_messenger_destroy(twin_msgr->amqp_msgr);
    }

    if (twin_msgr->pending_patches != NULL)
    {
        if (singlylinkedlist_remove_if(twin_msgr->pending_patches, cancel_pending_twin_patch_operation, twin_msgr) != 0)
        {
            LogError("Failed removing pending desired properties PATCH operation (%s)", twin_msgr->device_id);
        }

        singlylinkedlist_destroy(twin_msgr->pending_patches);
    }

    if (twin_msgr->operations != NULL)
    {
        singlylinkedlist_remove_if(twin_msgr->operations, cancel_all_pending_twin_operations, (const void*)twin_msgr);
        singlylinkedlist_destroy(twin_msgr->operations);
    }

    if (twin_msgr->device_id != NULL)
    {
        free(twin_msgr->device_id);
    }

    if (twin_msgr->module_id != NULL)
    {
        free(twin_msgr->module_id);
    }

    if (twin_msgr->iothub_host_fqdn != NULL)
    {
        free(twin_msgr->iothub_host_fqdn);
    }

    free(twin_msgr);
}


//---------- Internal Callbacks ----------//

static AMQP_MESSENGER_DISPOSITION_RESULT on_amqp_message_received_callback(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{

    AMQP_MESSENGER_DISPOSITION_RESULT disposition_result;

    if (message == NULL || context == NULL)
    {
        LogError("Invalid argument (message=%p, context=%p)", message, context);
        disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)context;

        char* correlation_id;

        bool has_status_code;
        int status_code;

        bool has_version;
        int64_t version;

        bool has_twin_report;
        BINARY_DATA twin_report;

        amqp_messenger_destroy_disposition_info(disposition_info);
        disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED;

        if (parse_incoming_twin_message(message, &correlation_id, &has_version, &version, &has_status_code, &status_code, &has_twin_report, &twin_report) != 0)
        {
            LogError("Failed parsing incoming TWIN message (%s)", twin_msgr->device_id);
        }
        else
        {
            if (correlation_id != NULL)
            {
                // It is supposed to be a request sent previously (reported properties PATCH, GET, PUT or DELETE).

                LIST_ITEM_HANDLE list_item;
                if ((list_item = singlylinkedlist_find(twin_msgr->operations, find_twin_operation_by_correlation_id, (const void*)correlation_id)) == NULL)
                {
                    LogError("Could not find context of TWIN incoming message (%s, %s)", twin_msgr->device_id, correlation_id);
                }
                else
                {
                    TWIN_OPERATION_CONTEXT* twin_op_ctx;

                    if ((twin_op_ctx = (TWIN_OPERATION_CONTEXT*)singlylinkedlist_item_get_value(list_item)) == NULL)
                    {
                        LogError("Could not get context for incoming TWIN message (%s, %s)", twin_msgr->device_id, correlation_id);
                    }
                    else
                    {
                        if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PATCH)
                        {
                            if (!has_status_code)
                            {
                                LogError("Received an incoming TWIN message for a PATCH operation, but with no status code (%s, %s)", twin_msgr->device_id, correlation_id);

                                disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;

                                if (twin_op_ctx->cb.reported_properties.callback != NULL)
                                {
                                    twin_op_ctx->cb.reported_properties.callback(TWIN_REPORT_STATE_RESULT_ERROR, TWIN_REPORT_STATE_REASON_INVALID_RESPONSE, 0, twin_op_ctx->cb.reported_properties.context);
                                }
                            }
                            else
                            {
                                if (twin_op_ctx->cb.reported_properties.callback != NULL)
                                {
                                    twin_op_ctx->cb.reported_properties.callback(TWIN_REPORT_STATE_RESULT_SUCCESS, TWIN_REPORT_STATE_REASON_NONE, status_code, twin_op_ctx->cb.reported_properties.context);
                                }
                            }
                        }
                        else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET)
                        {
                            if (!has_twin_report)
                            {
                                LogError("Received an incoming TWIN message for a GET operation, but with no report (%s, %s)", twin_msgr->device_id, correlation_id);

                                disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;

                                if (twin_op_ctx->msgr->on_message_received_callback != NULL)
                                {
                                    twin_op_ctx->msgr->on_message_received_callback(TWIN_UPDATE_TYPE_COMPLETE, NULL, 0, twin_op_ctx->msgr->on_message_received_context);
                                }

                                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES)
                                {
                                    twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
                                    twin_msgr->subscription_error_count++;
                                }
                            }
                            else
                            {
                                if (twin_op_ctx->msgr->on_message_received_callback != NULL)
                                {
                                    twin_op_ctx->msgr->on_message_received_callback(TWIN_UPDATE_TYPE_COMPLETE, (const char*)twin_report.bytes, twin_report.length, twin_op_ctx->msgr->on_message_received_context);
                                }

                                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_GETTING_COMPLETE_PROPERTIES)
                                {
                                    twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES;
                                    twin_msgr->subscription_error_count = 0;
                                }
                            }
                        }
                        else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_GET_ON_DEMAND)
                        {
                            if (!has_twin_report)
                            {
                                LogError("Received an incoming TWIN message for a GET operation, but with no report (%s, %s)", twin_msgr->device_id, correlation_id);

                                disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;

                                twin_op_ctx->cb.get_twin.callback(TWIN_UPDATE_TYPE_COMPLETE, NULL, 0, twin_op_ctx->cb.get_twin.context);
                            }
                            else
                            {
                                twin_op_ctx->cb.get_twin.callback(TWIN_UPDATE_TYPE_COMPLETE, (const char*)twin_report.bytes, twin_report.length, twin_op_ctx->cb.get_twin.context);
                            }
                        }
                        else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_PUT)
                        {
                            if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_SUBSCRIBED)
                            {
                                bool subscription_succeeded = true;

                                if (!has_status_code)
                                {
                                    LogError("Received an incoming TWIN message for a PUT operation, but with no status code (%s, %s)", twin_msgr->device_id, correlation_id);

                                    subscription_succeeded = false;
                                }
                                else if (status_code < 200 || status_code >= 300)
                                {
                                    LogError("Received status code %d for TWIN subscription request (%s, %s)", status_code, twin_msgr->device_id, correlation_id);

                                    subscription_succeeded = false;
                                }

                                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_SUBSCRIBING)
                                {
                                    if (subscription_succeeded)
                                    {
                                        twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_SUBSCRIBED;
                                        twin_msgr->subscription_error_count = 0;
                                    }
                                    else
                                    {
                                        twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_SUBSCRIBE_FOR_UPDATES;
                                        twin_msgr->subscription_error_count++;
                                    }
                                }
                            }
                        }
                        else if (twin_op_ctx->type == TWIN_OPERATION_TYPE_DELETE)
                        {
                            if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED)
                            {
                                bool unsubscription_succeeded = true;

                                if (!has_status_code)
                                {
                                    LogError("Received an incoming TWIN message for a DELETE operation, but with no status code (%s, %s)", twin_msgr->device_id, correlation_id);

                                    unsubscription_succeeded = false;
                                }
                                else if (status_code < 200 || status_code >= 300)
                                {
                                    LogError("Received status code %d for TWIN unsubscription request (%s, %s)", status_code, twin_msgr->device_id, correlation_id);

                                    unsubscription_succeeded = false;
                                }

                                if (twin_msgr->subscription_state == TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBING)
                                {
                                    if (unsubscription_succeeded)
                                    {
                                        twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED;
                                        twin_msgr->subscription_error_count = 0;
                                    }
                                    else
                                    {
                                        twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE;
                                        twin_msgr->subscription_error_count++;
                                    }
                                }
                            }
                        }

                        destroy_twin_operation_context(twin_op_ctx);
                    }

                    if (singlylinkedlist_remove(twin_msgr->operations, list_item) != 0)
                    {
                        LogError("Failed removing context for incoming TWIN message (%s, %s)",
                            twin_msgr->device_id, correlation_id);

                        update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
                    }
                }

                free(correlation_id);
            }
            else if (has_twin_report)
            {
                // It is supposed to be a desired properties delta update.

                if (twin_msgr->on_message_received_callback != NULL)
                {
                    twin_msgr->on_message_received_callback(TWIN_UPDATE_TYPE_PARTIAL, (const char*)twin_report.bytes, twin_report.length, twin_msgr->on_message_received_context);
                }
            }
            else
            {
                LogError("Received TWIN message with no correlation-id and no report (%s)", twin_msgr->device_id);
            }
        }
    }

    return disposition_result;
}

static void on_amqp_messenger_state_changed_callback(void* context, AMQP_MESSENGER_STATE previous_state, AMQP_MESSENGER_STATE new_state)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else if (new_state != previous_state)
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)context;

        if (twin_msgr->state == TWIN_MESSENGER_STATE_STARTING && new_state == AMQP_MESSENGER_STATE_STARTED)
        {
            if (twin_msgr->amqp_msgr_is_subscribed)
            {
                update_state(twin_msgr, TWIN_MESSENGER_STATE_STARTED);
            }
            // Else, it shall wait for the moment the AMQP msgr is subscribed.
        }
        else if (twin_msgr->state == TWIN_MESSENGER_STATE_STOPPING && new_state == AMQP_MESSENGER_STATE_STOPPED)
        {
            if (!twin_msgr->amqp_msgr_is_subscribed)
            {
                update_state(twin_msgr, TWIN_MESSENGER_STATE_STOPPED);
            }
            // Else, it shall wait for the moment the AMQP msgr is unsubscribed.
        }
        else if ((twin_msgr->state == TWIN_MESSENGER_STATE_STARTING && new_state == AMQP_MESSENGER_STATE_STARTING) ||
                (twin_msgr->state == TWIN_MESSENGER_STATE_STOPPING && new_state == AMQP_MESSENGER_STATE_STOPPING))
        {
            // Do nothing, this is expected.
        }
        else
        {
            LogError("Unexpected AMQP messenger state (%s, %s, %s)",
                twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_MESSENGER_STATE, twin_msgr->state), MU_ENUM_TO_STRING(AMQP_MESSENGER_STATE, new_state));

            update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
        }

        twin_msgr->amqp_msgr_state = new_state;
    }
}

static void on_amqp_messenger_subscription_changed_callback(void* context, bool is_subscribed)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)context;

        if (twin_msgr->state == TWIN_MESSENGER_STATE_STARTING && is_subscribed)
        {
            if (twin_msgr->amqp_msgr_state == AMQP_MESSENGER_STATE_STARTED)
            {
                update_state(twin_msgr, TWIN_MESSENGER_STATE_STARTED);
            }
            // Else, it shall wait for the moment the AMQP msgr is STARTED.
        }
        else if (twin_msgr->state == TWIN_MESSENGER_STATE_STOPPING && !is_subscribed)
        {
            if (twin_msgr->amqp_msgr_state == AMQP_MESSENGER_STATE_STOPPED)
            {
                update_state(twin_msgr, TWIN_MESSENGER_STATE_STOPPED);
            }
            // Else, it shall wait for the moment the AMQP msgr is STOPPED.
        }
        else
        {
            LogError("Unexpected AMQP messenger state (%s, %s, %d)",
                twin_msgr->device_id, MU_ENUM_TO_STRING(TWIN_MESSENGER_STATE, twin_msgr->state), is_subscribed);

            update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
        }

        twin_msgr->amqp_msgr_is_subscribed = is_subscribed;
    }
}


//---------- Public APIs ----------//

TWIN_MESSENGER_HANDLE twin_messenger_create(const TWIN_MESSENGER_CONFIG* messenger_config)
{
    TWIN_MESSENGER_INSTANCE* twin_msgr;

    if (messenger_config == NULL)
    {
        LogError("Invalid argument (messenger_config is NULL)");
        twin_msgr = NULL;
    }
    else if (messenger_config->device_id == NULL || messenger_config->iothub_host_fqdn == NULL || messenger_config->prod_info_cb == NULL)
    {
        LogError("Invalid argument (device_id=%p, iothub_host_fqdn=%p, client_version=%p)",
            messenger_config->device_id, messenger_config->iothub_host_fqdn, messenger_config->prod_info_cb);
        twin_msgr = NULL;
    }
    else
    {
        if ((twin_msgr = (TWIN_MESSENGER_INSTANCE*)malloc(sizeof(TWIN_MESSENGER_INSTANCE))) == NULL)
        {
            LogError("Failed allocating TWIN_MESSENGER_INSTANCE (%s)", messenger_config->device_id);
        }
        else
        {
            MAP_HANDLE link_attach_properties;

            memset(twin_msgr, 0, sizeof(TWIN_MESSENGER_INSTANCE));
            twin_msgr->state = TWIN_MESSENGER_STATE_STOPPED;
            twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED;
            twin_msgr->amqp_msgr_state = AMQP_MESSENGER_STATE_STOPPED;

            twin_msgr->prod_info_cb = messenger_config->prod_info_cb;
            twin_msgr->prod_info_ctx = messenger_config->prod_info_ctx;

            if (mallocAndStrcpy_s(&twin_msgr->device_id, messenger_config->device_id) != 0)
            {
                LogError("Failed copying device_id (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else if ((messenger_config->module_id != NULL) && (mallocAndStrcpy_s(&twin_msgr->module_id, messenger_config->module_id) != 0))
            {
                LogError("Failed copying module_id (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else if (mallocAndStrcpy_s(&twin_msgr->iothub_host_fqdn, messenger_config->iothub_host_fqdn) != 0)
            {
                LogError("Failed copying iothub_host_fqdn (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else if ((twin_msgr->pending_patches = singlylinkedlist_create()) == NULL)
            {
                LogError("Failed creating list for queueing patches (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else if ((twin_msgr->operations = singlylinkedlist_create()) == NULL)
            {
                LogError("Failed creating list for operations (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else if ((link_attach_properties = create_link_attach_properties(twin_msgr)) == NULL)
            {
                LogError("Failed creating link attach properties (%s)", messenger_config->device_id);
                internal_twin_messenger_destroy(twin_msgr);
                twin_msgr = NULL;
            }
            else
            {
                twin_msgr->prod_info_cb = messenger_config->prod_info_cb;
                twin_msgr->prod_info_ctx = messenger_config->prod_info_ctx;

                AMQP_MESSENGER_CONFIG amqp_msgr_config;
                amqp_msgr_config.prod_info_cb = twin_msgr->prod_info_cb;
                amqp_msgr_config.prod_info_ctx = twin_msgr->prod_info_ctx;
                amqp_msgr_config.device_id = twin_msgr->device_id;
                amqp_msgr_config.module_id = twin_msgr->module_id;
                amqp_msgr_config.iothub_host_fqdn = twin_msgr->iothub_host_fqdn;
                amqp_msgr_config.send_link.target_suffix = DEFAULT_TWIN_SEND_LINK_SOURCE_NAME;
                amqp_msgr_config.send_link.attach_properties = link_attach_properties;
                amqp_msgr_config.send_link.snd_settle_mode = sender_settle_mode_settled;
                amqp_msgr_config.send_link.rcv_settle_mode = receiver_settle_mode_first;
                amqp_msgr_config.receive_link.source_suffix = DEFAULT_TWIN_RECEIVE_LINK_TARGET_NAME;
                amqp_msgr_config.receive_link.attach_properties = link_attach_properties;
                amqp_msgr_config.receive_link.snd_settle_mode = sender_settle_mode_settled;
                amqp_msgr_config.receive_link.rcv_settle_mode = receiver_settle_mode_first;
                amqp_msgr_config.on_state_changed_callback = on_amqp_messenger_state_changed_callback;
                amqp_msgr_config.on_state_changed_context = (void*)twin_msgr;
                amqp_msgr_config.on_subscription_changed_callback = on_amqp_messenger_subscription_changed_callback;
                amqp_msgr_config.on_subscription_changed_context = (void*)twin_msgr;

                if ((twin_msgr->amqp_msgr = amqp_messenger_create(&amqp_msgr_config)) == NULL)
                {
                    LogError("Failed creating the AMQP messenger (%s)", messenger_config->device_id);
                    internal_twin_messenger_destroy(twin_msgr);
                    twin_msgr = NULL;
                }
                else if (amqp_messenger_subscribe_for_messages(twin_msgr->amqp_msgr, on_amqp_message_received_callback, (void*)twin_msgr))
                {
                    LogError("Failed subscribing for AMQP messages (%s)", messenger_config->device_id);
                    internal_twin_messenger_destroy(twin_msgr);
                    twin_msgr = NULL;
                }
                else
                {
                    twin_msgr->on_state_changed_callback = messenger_config->on_state_changed_callback;

                    twin_msgr->on_state_changed_context = messenger_config->on_state_changed_context;
                }

                destroy_link_attach_properties(link_attach_properties);
            }
        }
    }

    return (TWIN_MESSENGER_HANDLE)twin_msgr;
}

int twin_messenger_report_state_async(TWIN_MESSENGER_HANDLE twin_msgr_handle, CONSTBUFFER_HANDLE data, TWIN_MESSENGER_REPORT_STATE_COMPLETE_CALLBACK on_report_state_complete_callback, const void* context)
{
    int result;

    if (twin_msgr_handle == NULL || data == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, data=%p)", twin_msgr_handle, data);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;
        TWIN_PATCH_OPERATION_CONTEXT* twin_patch_ctx;

        if ((twin_patch_ctx = (TWIN_PATCH_OPERATION_CONTEXT*)malloc(sizeof(TWIN_PATCH_OPERATION_CONTEXT))) == NULL)
        {
            LogError("Failed creating context for sending reported state (%s)", twin_msgr->device_id);
            result = MU_FAILURE;
        }
        else
        {
            CONSTBUFFER_IncRef(data);
            twin_patch_ctx->data = data;

            if ((twin_patch_ctx->time_enqueued = get_time(NULL)) == INDEFINITE_TIME)
            {
                LogError("Failed setting reported state enqueue time (%s)", twin_msgr->device_id);
                CONSTBUFFER_DecRef(twin_patch_ctx->data);
                free(twin_patch_ctx);
                result = MU_FAILURE;
            }
            else
            {
                twin_patch_ctx->on_report_state_complete_callback = on_report_state_complete_callback;
                twin_patch_ctx->on_report_state_complete_context = context;

                if (singlylinkedlist_add(twin_msgr->pending_patches, twin_patch_ctx) == NULL)
                {
                    LogError("Failed adding TWIN patch request to queue (%s)", twin_msgr->device_id);
                    CONSTBUFFER_DecRef(twin_patch_ctx->data);
                    free(twin_patch_ctx);
                    result = MU_FAILURE;
                }
                else
                {
                    result = RESULT_OK;
                }
            }
        }
    }

    return result;
}

int twin_messenger_get_twin_async(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_STATE_UPDATE_CALLBACK on_get_twin_completed_callback, void* context)
{
    int result;

    if (twin_msgr_handle == NULL || on_get_twin_completed_callback == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, on_get_twin_completed_callback=%p)", twin_msgr_handle, on_get_twin_completed_callback);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;
        TWIN_OPERATION_CONTEXT* twin_op_ctx;

        if ((twin_op_ctx = create_twin_operation_context(twin_msgr, TWIN_OPERATION_TYPE_GET_ON_DEMAND)) == NULL)
        {
            LogError("Failed creating a context for TWIN request (%s, TWIN_OPERATION_TYPE_GET_ON_DEMAND)", twin_msgr->device_id);
            result = MU_FAILURE;
        }
        else
        {
            twin_op_ctx->cb.get_twin.callback = on_get_twin_completed_callback;
            twin_op_ctx->cb.get_twin.context = context;

            if (add_twin_operation_context_to_queue(twin_op_ctx) != RESULT_OK)
            {
                LogError("Failed queueing TWIN request context (%s, TWIN_OPERATION_TYPE_GET_ON_DEMAND)", twin_msgr->device_id);

                destroy_twin_operation_context(twin_op_ctx);
                result = MU_FAILURE;
            }
            else if (send_twin_operation_request(twin_msgr, twin_op_ctx, NULL) != RESULT_OK)
            {
                LogError("Failed sending TWIN request (%s, TWIN_OPERATION_TYPE_GET_ON_DEMAND)", twin_msgr->device_id);

                (void)remove_twin_operation_context_from_queue(twin_op_ctx);
                destroy_twin_operation_context(twin_op_ctx);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
    }

    return result;
}


int twin_messenger_subscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_STATE_UPDATE_CALLBACK on_twin_state_update_callback, void* context)
{
    int result;

    if (twin_msgr_handle == NULL || on_twin_state_update_callback == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, on_twin_state_update_callback=%p)", twin_msgr_handle, on_twin_state_update_callback);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        if (twin_msgr->subscription_state != TWIN_SUBSCRIPTION_STATE_NOT_SUBSCRIBED)
        {
            result = RESULT_OK;
        }
        else
        {
            twin_msgr->on_message_received_callback = on_twin_state_update_callback;
            twin_msgr->on_message_received_context = context;
            twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
            result = RESULT_OK;
        }
    }

    return result;
}

int twin_messenger_unsubscribe(TWIN_MESSENGER_HANDLE twin_msgr_handle)
{
    int result;

    if (twin_msgr_handle == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;
        twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE;
        twin_msgr->on_message_received_callback = NULL;
        twin_msgr->on_message_received_context = NULL;
        result = RESULT_OK;
    }

    return result;
}

int twin_messenger_get_send_status(TWIN_MESSENGER_HANDLE twin_msgr_handle, TWIN_MESSENGER_SEND_STATUS* send_status)
{
    int result;

    if (twin_msgr_handle == NULL || send_status == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, send_status=%p)", twin_msgr_handle, send_status);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;
        TWIN_OPERATION_TYPE twin_op_type = TWIN_OPERATION_TYPE_PATCH;

        if (singlylinkedlist_get_head_item(twin_msgr->pending_patches) != NULL ||
            singlylinkedlist_find(twin_msgr->operations, find_twin_operation_by_type, &twin_op_type))
        {
            *send_status = TWIN_MESSENGER_SEND_STATUS_BUSY;
        }
        else
        {
            *send_status = TWIN_MESSENGER_SEND_STATUS_IDLE;
        }

        result = RESULT_OK;
    }

    return result;
}

int twin_messenger_start(TWIN_MESSENGER_HANDLE twin_msgr_handle, SESSION_HANDLE session_handle)
{
    int result;

    if (twin_msgr_handle == NULL || session_handle == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, session_handle=%p)", twin_msgr_handle, session_handle);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        update_state(twin_msgr, TWIN_MESSENGER_STATE_STARTING);

        if(amqp_messenger_set_option(twin_msgr->amqp_msgr, (const char *)OPTION_PRODUCT_INFO, (void *)twin_msgr->prod_info_cb(twin_msgr->prod_info_ctx)) == 0)
        {
            if (amqp_messenger_start(twin_msgr->amqp_msgr, session_handle) != 0)
            {
                LogError("Failed starting the AMQP messenger (%s)", twin_msgr->device_id);
                update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else
        {
            LogError("Failed to set the product info option (%s)", twin_msgr->device_id);
            update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
            result = MU_FAILURE;
        }
    }

    return result;
}

int twin_messenger_stop(TWIN_MESSENGER_HANDLE twin_msgr_handle)
{
    int result;

    if (twin_msgr_handle == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        update_state(twin_msgr, TWIN_MESSENGER_STATE_STOPPING);

        if (amqp_messenger_stop(twin_msgr->amqp_msgr) != 0)
        {
            LogError("Failed stopping the AMQP messenger (%s)", twin_msgr->device_id);
            update_state(twin_msgr, TWIN_MESSENGER_STATE_ERROR);
            result = MU_FAILURE;
        }
        else
        {
            if (twin_msgr->subscription_state != TWIN_SUBSCRIPTION_STATE_UNSUBSCRIBE)
            {
                twin_msgr->subscription_state = TWIN_SUBSCRIPTION_STATE_GET_COMPLETE_PROPERTIES;
            }

            result = RESULT_OK;
        }
    }

    return result;
}

void twin_messenger_do_work(TWIN_MESSENGER_HANDLE twin_msgr_handle)
{
    if (twin_msgr_handle != NULL)
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        if (twin_msgr->state == TWIN_MESSENGER_STATE_STARTED)
        {
            (void)singlylinkedlist_remove_if(twin_msgr->pending_patches, send_pending_twin_patch, (const void*)twin_msgr);

            process_twin_subscription(twin_msgr);
        }

        process_timeouts(twin_msgr);

        amqp_messenger_do_work(twin_msgr->amqp_msgr);
    }
}

void twin_messenger_destroy(TWIN_MESSENGER_HANDLE twin_msgr_handle)
{
    if (twin_msgr_handle == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle is NULL)");
    }
    else
    {
        internal_twin_messenger_destroy((TWIN_MESSENGER_INSTANCE*)twin_msgr_handle);
    }
}

int twin_messenger_set_option(TWIN_MESSENGER_HANDLE twin_msgr_handle, const char* name, void* value)
{
    int result;

    if (twin_msgr_handle == NULL || name == NULL || value == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle=%p, name=%p, value=%p)", twin_msgr_handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        if (amqp_messenger_set_option(twin_msgr->amqp_msgr, name, value) != RESULT_OK)
        {
            LogError("Failed setting TWIN messenger option (%s, %s)", twin_msgr->device_id, name);
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE twin_messenger_retrieve_options(TWIN_MESSENGER_HANDLE twin_msgr_handle)
{
    OPTIONHANDLER_HANDLE result;

    if (twin_msgr_handle == NULL)
    {
        LogError("Invalid argument (twin_msgr_handle is NULL)");
        result = NULL;
    }
    else
    {
        TWIN_MESSENGER_INSTANCE* twin_msgr = (TWIN_MESSENGER_INSTANCE*)twin_msgr_handle;

        if ((result = amqp_messenger_retrieve_options(twin_msgr->amqp_msgr)) == NULL)
        {
            LogError("Failed TWIN messenger options (%s)", twin_msgr->device_id);
        }
    }

    return result;
}
