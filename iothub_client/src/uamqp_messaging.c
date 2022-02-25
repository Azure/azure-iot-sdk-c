// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#else
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#endif

#include "internal/uamqp_messaging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uuid.h"
#include "azure_uamqp_c/amqp_definitions.h"
#include "azure_uamqp_c/message.h"
#include "azure_uamqp_c/amqpvalue.h"
#include "iothub_message.h"

#include "internal/iothub_internal_consts.h"

#ifndef RESULT_OK
#define RESULT_OK 0
#endif

#define MESSAGE_ID_MAX_SIZE 128

#define AMQP_DIAGNOSTIC_ID_KEY "Diagnostic-Id"
#define AMQP_DIAGNOSTIC_CONTEXT_KEY "Correlation-Context"
#define AMQP_DIAGNOSTIC_CREATION_TIME_UTC_KEY "creationtimeutc"
#define AMQP_IOTHUB_CREATION_TIME_UTC "iothub-creation-time-utc"

static int encode_callback(void* context, const unsigned char* bytes, size_t length)
{
    BINARY_DATA* message_body_binary = (BINARY_DATA*)context;
    (void)memcpy((unsigned char*)message_body_binary->bytes + message_body_binary->length, bytes, length);
    message_body_binary->length += length;
    return 0;
}

static int set_message_id_if_needed(IOTHUB_MESSAGE_HANDLE messageHandle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* messageId;

    if (NULL != (messageId = IoTHubMessage_GetMessageId(messageHandle)))
    {
        AMQP_VALUE uamqp_message_id;

        if ((uamqp_message_id = amqpvalue_create_string(messageId)) == NULL)
        {
            LogError("Failed amqpvalue_create_string for message_id");
            result = MU_FAILURE;
        }
        else if (properties_set_message_id(uamqp_message_properties, uamqp_message_id) != 0)
        {
            LogError("Failed properties_set_message_id");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        if (NULL != uamqp_message_id)
        {
            amqpvalue_destroy(uamqp_message_id);
        }
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int set_message_correlation_id_if_needed(IOTHUB_MESSAGE_HANDLE messageHandle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* correlationId;

    if (NULL != (correlationId = IoTHubMessage_GetCorrelationId(messageHandle)))
    {
        AMQP_VALUE uamqp_correlation_id;

        if ((uamqp_correlation_id = amqpvalue_create_string(correlationId)) == NULL)
        {
            LogError("Failed amqpvalue_create_string for correlationId");
            result = MU_FAILURE;
        }
        else if (properties_set_correlation_id(uamqp_message_properties, uamqp_correlation_id) != 0)
        {
            LogError("Failed properties_set_correlation_id");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        if (uamqp_correlation_id != NULL)
        {
            amqpvalue_destroy(uamqp_correlation_id);
        }
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int set_message_content_type_if_needed(IOTHUB_MESSAGE_HANDLE messageHandle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* content_type;

    if ((content_type = IoTHubMessage_GetContentTypeSystemProperty(messageHandle)) != NULL)
    {
        if (properties_set_content_type(uamqp_message_properties, content_type) != 0)
        {
            LogError("Failed properties_set_content_type");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}

static int set_message_content_encoding_if_needed(IOTHUB_MESSAGE_HANDLE messageHandle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* content_encoding;

    if ((content_encoding = IoTHubMessage_GetContentEncodingSystemProperty(messageHandle)) != NULL)
    {
        if (properties_set_content_encoding(uamqp_message_properties, content_encoding) != 0)
        {
            LogError("Failed properties_set_content_encoding");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}


static int create_message_properties_to_encode(IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE *message_properties, size_t *message_properties_length)
{
    PROPERTIES_HANDLE uamqp_message_properties = NULL;
    int result;

    if ((uamqp_message_properties = properties_create()) == NULL)
    {
        LogError("Failed on properties_create()");
        result = MU_FAILURE;
    }
    else if (set_message_id_if_needed(messageHandle, uamqp_message_properties) != RESULT_OK)
    {
        LogError("Failed on set_message_id_if_needed()");
        result = MU_FAILURE;
    }
    else if (set_message_correlation_id_if_needed(messageHandle, uamqp_message_properties) != RESULT_OK)
    {
        LogError("Failed on set_message_correlation_id_if_needed()");
        result = MU_FAILURE;
    }
    else if (set_message_content_type_if_needed(messageHandle, uamqp_message_properties) != RESULT_OK)
    {
        LogError("Failed on set_message_content_type_if_needed()");
        result = MU_FAILURE;
    }
    else if (set_message_content_encoding_if_needed(messageHandle, uamqp_message_properties) != RESULT_OK)
    {
        LogError("Failed on set_message_content_encoding_if_needed()");
        result = MU_FAILURE;
    }
    else if ((*message_properties = amqpvalue_create_properties(uamqp_message_properties)) == NULL)
    {
        LogError("Failed on amqpvalue_create_properties()");
        result = MU_FAILURE;
    }
    else if ((amqpvalue_get_encoded_size(*message_properties, message_properties_length)) != 0)
    {
        LogError("Failed on amqpvalue_get_encoded_size()");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    if (NULL != uamqp_message_properties)
    {
        properties_destroy(uamqp_message_properties);
    }

    return result;
}

// Adds fault injection properties to an AMQP message.
static int add_fault_injection_properties(MESSAGE_HANDLE message_batch_container, const char* const* property_keys, const char* const* property_values, size_t property_count)
{
    int result;
    AMQP_VALUE uamqp_map;

    if ((uamqp_map = amqpvalue_create_map()) == NULL)
    {
        LogError("Failed to create uAMQP map for the properties.");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;

        for (size_t i = 0; result == RESULT_OK && i < property_count; i++)
        {
            AMQP_VALUE map_key_value = NULL;
            AMQP_VALUE map_value_value = NULL;

            if ((map_key_value = amqpvalue_create_string(property_keys[i])) == NULL)
            {
                LogError("Failed to create uAMQP property key name.");
                result = MU_FAILURE;
            }
            else if ((map_value_value = amqpvalue_create_string(property_values[i])) == NULL)
            {
                LogError("Failed to create uAMQP property key value.");
                result = MU_FAILURE;
            }
            else if (amqpvalue_set_map_value(uamqp_map, map_key_value, map_value_value) != 0)
            {
                LogError("Failed to set key/value into the the uAMQP property map.");
                result = MU_FAILURE;
            }

            if (map_key_value != NULL)
                amqpvalue_destroy(map_key_value);

            if (map_value_value != NULL)
                amqpvalue_destroy(map_value_value);
        }

        if (result == RESULT_OK)
        {
            if (message_set_application_properties(message_batch_container, uamqp_map) != 0)
            {
                LogError("Failed to transfer the message properties to the uAMQP message.");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        amqpvalue_destroy(uamqp_map);
    }

    return result;
}

// To test AMQP fault injection, we currently must have the error properties be specified on the batch_container
// (not one of the messages sent in this container).  As the SDK layer does not support options for configuring
// this envelope (this is AMQP/batching specific), we will instead intercept fault messages and apply to the container.
static int override_fault_injection_properties_if_needed(MESSAGE_HANDLE message_batch_container,  const char* const* property_keys, const char* const* property_values, size_t property_count, bool *override_for_fault_injection)
{
    int result;

    if ((property_count == 0) || (strcmp(property_keys[0], "AzIoTHub_FaultOperationType") != 0))
    {
        *override_for_fault_injection = false;
        result = RESULT_OK;
    }
    else
    {
        *override_for_fault_injection = true;
        result = add_fault_injection_properties(message_batch_container,  property_keys, property_values, property_count);
    }

    return result;
}

static int create_application_properties_to_encode(MESSAGE_HANDLE message_batch_container, IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE *application_properties, size_t *application_properties_length)
{
    MAP_HANDLE properties_map;
    const char* const* property_keys = NULL;
    const char* const* property_values = NULL;
    const char* message_creation_time_utc;
    size_t property_count = 0;
    AMQP_VALUE uamqp_properties_map = NULL;
    int result = RESULT_OK;

    if ((properties_map = IoTHubMessage_Properties(messageHandle)) == NULL)
    {
        LogError("Failed to get property map from IoTHub message.");
        result = MU_FAILURE;
    }

    if (RESULT_OK == result &&
        NULL != (message_creation_time_utc = IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(messageHandle)))
    {
        if (Map_AddOrUpdate(properties_map, AMQP_IOTHUB_CREATION_TIME_UTC, message_creation_time_utc) != MAP_OK)
        {
            LogError("Failed to add/update application message property map.");
            result = MU_FAILURE;
        }
    }

    if (RESULT_OK == result &&
        Map_GetInternals(properties_map, &property_keys, &property_values, &property_count) != MAP_OK)
    {
        LogError("Failed reading the incoming uAMQP message properties");
        result = MU_FAILURE;
    }
    else if (property_count > 0)
    {
        size_t i;
        if ((uamqp_properties_map = amqpvalue_create_map()) == NULL)
        {
            LogError("amqpvalue_create_map failed");
            result = MU_FAILURE;
        }
        else
        {
            bool override_for_fault_injection = false;
            result = override_fault_injection_properties_if_needed(message_batch_container, property_keys, property_values, property_count, &override_for_fault_injection);

            if (override_for_fault_injection == false)
            {
                for (i = 0; i < property_count; i++)
                {
                    AMQP_VALUE map_property_key;
                    AMQP_VALUE map_property_value;

                    if ((map_property_key = amqpvalue_create_string(property_keys[i])) == NULL)
                    {
                        LogError("Failed amqpvalue_create_string for key");
                        result = MU_FAILURE;
                        break;
                    }

                    if ((map_property_value = amqpvalue_create_string(property_values[i])) == NULL)
                    {
                        LogError("Failed amqpvalue_create_string for value");
                        amqpvalue_destroy(map_property_key);
                        result = MU_FAILURE;
                        break;
                    }

                    if (amqpvalue_set_map_value(uamqp_properties_map, map_property_key, map_property_value) != 0)
                    {
                        LogError("Failed amqpvalue_set_map_value");
                        amqpvalue_destroy(map_property_key);
                        amqpvalue_destroy(map_property_value);
                        result = MU_FAILURE;
                        break;
                    }

                    amqpvalue_destroy(map_property_key);
                    amqpvalue_destroy(map_property_value);
                }

                if (RESULT_OK == result)
                {
                    if ((*application_properties = amqpvalue_create_application_properties(uamqp_properties_map)) == NULL)
                    {
                        LogError("Failed amqpvalue_create_application_properties");
                        result = MU_FAILURE;
                    }
                    else if (amqpvalue_get_encoded_size(*application_properties, application_properties_length) != 0)
                    {
                        LogError("Failed amqpvalue_get_encoded_size");
                        result = MU_FAILURE;
                    }
                }
            }
        }
    }

    if (NULL != uamqp_properties_map)
    {
        amqpvalue_destroy(uamqp_properties_map);
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

static int create_diagnostic_message_annotations(IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE* message_annotations_map)
{
    int result = RESULT_OK;

    // Deprecated: maintained for backwards compatibility; use IoTHubMessage_GetDistributedTracingSystemProperty instead.
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData;
    bool annotation_created = false;

    if ((diagnosticData = IoTHubMessage_GetDiagnosticPropertyData(messageHandle)) != NULL &&
        diagnosticData->diagnosticId != NULL && diagnosticData->diagnosticCreationTimeUtc != NULL)
    {
        if (*message_annotations_map == NULL)
        {
            if ((*message_annotations_map = amqpvalue_create_map()) == NULL)
            {
                LogError("Failed amqpvalue_create_map for annotations");
                result = MU_FAILURE;
            }
            else
            {
                annotation_created = true;
            }
        }

        if (result == RESULT_OK)
        {
            char* diagContextBuffer = NULL;
            if (add_map_item(*message_annotations_map, AMQP_DIAGNOSTIC_ID_KEY, diagnosticData->diagnosticId) != RESULT_OK)
            {
                LogError("Failed adding diagnostic id");
                result = MU_FAILURE;
                if (annotation_created)
                {
                    amqpvalue_destroy(*message_annotations_map);
                    *message_annotations_map = NULL;
                }
            }
            else if ((diagContextBuffer = (char*)malloc(strlen(AMQP_DIAGNOSTIC_CREATION_TIME_UTC_KEY) + 1
                + strlen(diagnosticData->diagnosticCreationTimeUtc) + 1)) == NULL)
            {
                LogError("Failed malloc for diagnostic context");
                result = MU_FAILURE;
                if (annotation_created)
                {
                    amqpvalue_destroy(*message_annotations_map);
                    *message_annotations_map = NULL;
                }
            }
            else if (sprintf(diagContextBuffer, "%s=%s", AMQP_DIAGNOSTIC_CREATION_TIME_UTC_KEY, diagnosticData->diagnosticCreationTimeUtc) < 0)
            {
                LogError("Failed sprintf diagnostic context");
                result = MU_FAILURE;
                if (annotation_created)
                {
                    amqpvalue_destroy(*message_annotations_map);
                    *message_annotations_map = NULL;
                }
            }
            else if (add_map_item(*message_annotations_map, AMQP_DIAGNOSTIC_CONTEXT_KEY, diagContextBuffer) != RESULT_OK)
            {
                LogError("Failed adding diagnostic context");
                result = MU_FAILURE;
                if (annotation_created)
                {
                    amqpvalue_destroy(*message_annotations_map);
                    *message_annotations_map = NULL;
                }
            }
            free(diagContextBuffer);
        }
    }
    return result;
}

static int create_security_message_annotations(IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE* message_annotations_map)
{
    int result = RESULT_OK;
    bool annotation_created = false;
    if (IoTHubMessage_IsSecurityMessage(messageHandle))
    {
        if (*message_annotations_map == NULL)
        {
            if ((*message_annotations_map = amqpvalue_create_map()) == NULL)
            {
                LogError("Failed amqpvalue_create_map for annotations");
                result = MU_FAILURE;
            }
            else
            {
                annotation_created = true;
            }
        }

        if (result == RESULT_OK)
        {
            if (add_map_item(*message_annotations_map, SECURITY_INTERFACE_ID, SECURITY_INTERFACE_ID_VALUE) != RESULT_OK)
            {
                LogError("Failed adding Security interface id");
                result = MU_FAILURE;
                if (annotation_created)
                {
                    amqpvalue_destroy(*message_annotations_map);
                    *message_annotations_map = NULL;
                }
            }
        }
    }
    return result;
}

static int create_message_annotations_to_encode(IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE *message_annotations, size_t *message_annotations_length)
{
    AMQP_VALUE message_annotations_map = NULL;
    int result;

    if ((result = create_diagnostic_message_annotations(messageHandle, &message_annotations_map)) != RESULT_OK)
    {
        LogError("Failed creating message annotations");
        result = MU_FAILURE;
    }
    else if ((result = create_security_message_annotations(messageHandle, &message_annotations_map)) != RESULT_OK)
    {
        LogError("Failed creating message annotations");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }
    if (message_annotations_map != NULL)
    {
        if (result == RESULT_OK)
        {
            if ((*message_annotations = amqpvalue_create_message_annotations(message_annotations_map)) == NULL)
            {
                LogError("Failed creating message annotations");
                result = MU_FAILURE;
            }
            else if (amqpvalue_get_encoded_size(*message_annotations, message_annotations_length) != 0)
            {
                LogError("Failed getting size of annotations");
                result = MU_FAILURE;
            }
        }
        amqpvalue_destroy(message_annotations_map);
    }
    return result;
}

static int create_data_to_encode(IOTHUB_MESSAGE_HANDLE messageHandle, AMQP_VALUE *data_value, size_t *data_length)
{
    int result;

    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(messageHandle);
    const char* messageContent = NULL;
    size_t messageContentSize = 0;

    if ((contentType == IOTHUBMESSAGE_BYTEARRAY) &&
        IoTHubMessage_GetByteArray(messageHandle, (const unsigned char **)&messageContent, &messageContentSize) != IOTHUB_MESSAGE_OK)
    {
        LogError("Failed getting the BYTE array representation of the IOTHUB_MESSAGE_HANDLE instance.");
        result = MU_FAILURE;
    }
    else if ((contentType == IOTHUBMESSAGE_STRING) &&
        ((messageContent = IoTHubMessage_GetString(messageHandle)) == NULL))
    {
        LogError("Failed getting the STRING representation of the IOTHUB_MESSAGE_HANDLE instance.");
        result = MU_FAILURE;
    }
    else if (contentType == IOTHUBMESSAGE_UNKNOWN)
    {
        LogError("Cannot parse IOTHUB_MESSAGE_HANDLE with content type IOTHUBMESSAGE_UNKNOWN.");
        result = MU_FAILURE;
    }
    else
    {
        if (contentType == IOTHUBMESSAGE_STRING)
        {
            messageContentSize = messageContent != NULL ? strlen(messageContent) : 0;
        }

        data bin_data;
        bin_data.bytes = (const unsigned char *)messageContent;
        bin_data.length = (uint32_t)messageContentSize;

        if ((*data_value = amqpvalue_create_data(bin_data)) == NULL)
        {
            LogError("amqpvalue_create_data failed");
            result = MU_FAILURE;
        }
        else if (amqpvalue_get_encoded_size(*data_value, data_length) != 0)
        {
            LogError("amqpvalue_get_encoded_size failed");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

int message_create_uamqp_encoding_from_iothub_message(MESSAGE_HANDLE message_batch_container, IOTHUB_MESSAGE_HANDLE message_handle, BINARY_DATA* body_binary_data)
{
    int result;

    AMQP_VALUE message_properties = NULL;
    AMQP_VALUE application_properties = NULL;
    AMQP_VALUE message_annotations = NULL;
    AMQP_VALUE data_value = NULL;
    size_t message_properties_length = 0;
    size_t application_properties_length = 0;
    size_t message_annotations_length = 0;
    size_t data_length = 0;

    body_binary_data->bytes = NULL;
    body_binary_data->length = 0;

    if (create_message_properties_to_encode(message_handle, &message_properties, &message_properties_length) != RESULT_OK)
    {
        LogError("create_message_properties_to_encode() failed");
        result = MU_FAILURE;
    }
    else if (create_application_properties_to_encode(message_batch_container, message_handle, &application_properties, &application_properties_length) != RESULT_OK)
    {
        LogError("create_application_properties_to_encode() failed");
        result = MU_FAILURE;
    }
    else if (create_message_annotations_to_encode(message_handle, &message_annotations, &message_annotations_length) != RESULT_OK)
    {
        LogError("create_message_annotations_to_encode() failed");
        result = MU_FAILURE;
    }
    else if (create_data_to_encode(message_handle, &data_value, &data_length) != RESULT_OK)
    {
        LogError("create_data_to_encode() failed");
        result = MU_FAILURE;
    }
    else if ((body_binary_data->bytes = malloc(message_properties_length + application_properties_length + data_length + message_annotations_length)) == NULL)
    {
        LogError("malloc of %lu bytes failed", (unsigned long)(message_properties_length + application_properties_length + data_length + message_annotations_length));
        result = MU_FAILURE;
    }
    else if (amqpvalue_encode(message_properties, &encode_callback, body_binary_data) != RESULT_OK)
    {
        LogError("amqpvalue_encode() for message properties failed");
        result = MU_FAILURE;
    }
    else if ((application_properties_length > 0) && (amqpvalue_encode(application_properties, &encode_callback, body_binary_data)  != RESULT_OK))
    {
        LogError("amqpvalue_encode() for application properties failed");
        result = MU_FAILURE;
    }
    else if (message_annotations_length > 0 && amqpvalue_encode(message_annotations, &encode_callback, body_binary_data) != RESULT_OK)
    {
        LogError("amqpvalue_encode() for message annotations failed");
        result = MU_FAILURE;
    }
    else if (RESULT_OK != amqpvalue_encode(data_value, &encode_callback, body_binary_data))
    {
        LogError("amqpvalue_encode() for data value failed");
        result = MU_FAILURE;
    }
    else
    {
        body_binary_data->length = message_properties_length + application_properties_length + data_length + message_annotations_length;
        result = RESULT_OK;
    }

    if (NULL != data_value)
    {
        amqpvalue_destroy(data_value);
    }

    if (NULL != application_properties)
    {
        amqpvalue_destroy(application_properties);
    }

    if (NULL != message_annotations)
    {
        amqpvalue_destroy(message_annotations);
    }

    if (NULL != message_properties)
    {
        amqpvalue_destroy(message_properties);
    }

    return result;
}

static int readMessageIdFromuAQMPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    AMQP_VALUE uamqp_message_property;

    if (properties_get_message_id(uamqp_message_properties, &uamqp_message_property) != 0 || uamqp_message_property == NULL)
    {
        result = RESULT_OK;
    }
    else
    {
        AMQP_TYPE value_type = amqpvalue_get_type(uamqp_message_property);

        if (value_type != AMQP_TYPE_NULL)
        {

            char* string_value;
            char string_buffer[MESSAGE_ID_MAX_SIZE];
            bool free_string_value = false;

            memset(string_buffer, 0, MESSAGE_ID_MAX_SIZE);

            if (value_type == AMQP_TYPE_STRING)
            {
                if (amqpvalue_get_string(uamqp_message_property, (const char**)(&string_value)) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'message-id' property (string)");
                    string_value = NULL;
                }
            }
            else if (value_type == AMQP_TYPE_ULONG)
            {
                uint64_t ulong_value;

                if (amqpvalue_get_ulong(uamqp_message_property, &ulong_value) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'message-id' property (ulong)");
                    string_value = NULL;
                }
                else if (sprintf(string_buffer, "%" PRIu64, ulong_value) < 0)
                {
                    LogError("Failed converting 'message-id' (ulong) to string");
                    string_value = NULL;
                }
                else
                {
                    string_value = string_buffer;
                }
            }
            else if (value_type == AMQP_TYPE_UUID)
            {
                uuid uuid_value;

                if (amqpvalue_get_uuid(uamqp_message_property, &uuid_value) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'message-id' property (UUID)");
                    string_value = NULL;
                }
                else if ((string_value = UUID_to_string((const UUID_T*)uuid_value)) == NULL)
                {
                    LogError("Failed to get the string representation of 'message-id' UUID");
                    string_value = NULL;
                }
                else
                {
                    free_string_value = true;
                }
            }
            else
            {
                LogError("Unrecognized type for message-id (%d)", value_type);
                string_value = NULL;
            }

            if (string_value != NULL)
            {
                if (IoTHubMessage_SetMessageId(iothub_message_handle, string_value) != IOTHUB_MESSAGE_OK)
                {
                    LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'message-id' property.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = RESULT_OK;
                }

                if (free_string_value)
                {
                    // Only certain code paths allocate this.
                    free(string_value);
                }
            }
            else
            {
                LogError("Unexpected null string for message-id");
                result = MU_FAILURE;
            }
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

static int readCorrelationIdFromuAQMPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    AMQP_VALUE uamqp_message_property;

    if (properties_get_correlation_id(uamqp_message_properties, &uamqp_message_property) != 0 || uamqp_message_property == NULL)
    {
        result = RESULT_OK;
    }
    else
    {
        AMQP_TYPE value_type = amqpvalue_get_type(uamqp_message_property);

        if (value_type != AMQP_TYPE_NULL)
        {
            char* string_value;
            char string_buffer[MESSAGE_ID_MAX_SIZE];
            bool free_string_value = false;

            memset(string_buffer, 0, MESSAGE_ID_MAX_SIZE);

            if (value_type == AMQP_TYPE_STRING)
            {
                if (amqpvalue_get_string(uamqp_message_property, (const char**)(&string_value)) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'correlation-id' property (string)");
                    string_value = NULL;
                }
            }
            else if (value_type == AMQP_TYPE_ULONG)
            {
                uint64_t ulong_value;

                if (amqpvalue_get_ulong(uamqp_message_property, &ulong_value) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'correlation-id' property (ulong)");
                    string_value = NULL;
                }
                else if (sprintf(string_buffer, "%" PRIu64, ulong_value) < 0)
                {
                    LogError("Failed converting 'correlation-id' (ulong) to string");
                    string_value = NULL;
                }
                else
                {
                    string_value = string_buffer;
                }
            }
            else if (value_type == AMQP_TYPE_UUID)
            {
                uuid uuid_value;

                if (amqpvalue_get_uuid(uamqp_message_property, &uuid_value) != 0)
                {
                    LogError("Failed to get value of uAMQP message 'correlation-id' property (UUID)");
                    string_value = NULL;
                }
                else if ((string_value = UUID_to_string((const UUID_T*)uuid_value)) == NULL)
                {
                    LogError("Failed to get the string representation of 'correlation-id' UUID");
                    string_value = NULL;
                }
                else
                {
                    free_string_value = true;
                }
            }
            else
            {
                LogError("Unrecognized type for correlation-id (%d)", value_type);
                string_value = NULL;
            }

            if (string_value != NULL)
            {
                if (IoTHubMessage_SetCorrelationId(iothub_message_handle, string_value) != IOTHUB_MESSAGE_OK)
                {
                    LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'correlation-id' property.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = RESULT_OK;
                }

                if (free_string_value)
                {
                    // Only certain code paths allocate this.
                    free(string_value);
                }
            }
            else
            {
                LogError("Unexpected null string for correlation-id");
                result = MU_FAILURE;
            }
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

static int readUserIdFromuAQMPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    amqp_binary uamqp_message_property;

    if (properties_get_user_id(uamqp_message_properties, &uamqp_message_property) != 0 || uamqp_message_property.length == 0)
    {
        result = RESULT_OK;
    }
    else
    {
        char string_buffer[MESSAGE_ID_MAX_SIZE];
        memset(string_buffer, 0, MESSAGE_ID_MAX_SIZE);
        strncpy(string_buffer, uamqp_message_property.bytes, uamqp_message_property.length > (MESSAGE_ID_MAX_SIZE - 1) ? MESSAGE_ID_MAX_SIZE - 1 : uamqp_message_property.length);
        if (IoTHubMessage_SetMessageUserIdSystemProperty(iothub_message_handle, string_buffer) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'user-id' property.");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }
    }

    return result;
}

static int readPropertiesFromuAMQPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, MESSAGE_HANDLE uamqp_message)
{
    int result;
    PROPERTIES_HANDLE uamqp_message_properties;

    if (message_get_properties(uamqp_message, &uamqp_message_properties) != 0)
    {
        LogError("Failed to get property properties map from uAMQP message.");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK; // Properties 'message-id' and 'correlation-id' are optional according to the AMQP 1.0 spec.

        if (readMessageIdFromuAQMPMessage(iothub_message_handle, uamqp_message_properties) != RESULT_OK)
        {
            LogError("Failed readMessageIdFromuAQMPMessage.");
            result = MU_FAILURE;
        }
        else if (readCorrelationIdFromuAQMPMessage(iothub_message_handle, uamqp_message_properties) != RESULT_OK)
        {
            LogError("Failed readCorrelationIdFromuAQMPMessage.");
            result = MU_FAILURE;
        }
        else if (readUserIdFromuAQMPMessage(iothub_message_handle, uamqp_message_properties) != RESULT_OK)
        {
            LogError("Failed readUserIdFromuAQMPMessage.");
            result = MU_FAILURE;
        }
        else
        {
            const char* uamqp_message_property_value = NULL;

            if (properties_get_content_type(uamqp_message_properties, &uamqp_message_property_value) == 0 && uamqp_message_property_value != NULL)
            {
                if (IoTHubMessage_SetContentTypeSystemProperty(iothub_message_handle, uamqp_message_property_value) != IOTHUB_MESSAGE_OK)
                {
                    LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'content-type' property.");
                    result = MU_FAILURE;
                }
            }

            uamqp_message_property_value = NULL;

            if (properties_get_content_encoding(uamqp_message_properties, &uamqp_message_property_value) == 0 && uamqp_message_property_value != NULL)
            {
                if (IoTHubMessage_SetContentEncodingSystemProperty(iothub_message_handle, uamqp_message_property_value) != IOTHUB_MESSAGE_OK)
                {
                    LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'content-encoding' property.");
                    result = MU_FAILURE;
                }
            }
        }

        properties_destroy(uamqp_message_properties);
    }

    return result;
}

static int readApplicationPropertiesFromuAMQPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, MESSAGE_HANDLE uamqp_message)
{
    int result;
    AMQP_VALUE uamqp_app_properties = NULL;
    AMQP_VALUE uamqp_app_properties_ipdv = NULL;
    uint32_t property_count = 0;
    MAP_HANDLE iothub_message_properties_map;

    if ((iothub_message_properties_map = IoTHubMessage_Properties(iothub_message_handle)) == NULL)
    {
        LogError("Failed to get property map from IoTHub message.");
        result = MU_FAILURE;
    }
    else if ((result = message_get_application_properties(uamqp_message, &uamqp_app_properties)) != 0)
    {
        LogError("Failed reading the incoming uAMQP message properties (return code %d).", result);
        result = MU_FAILURE;
    }
    else
    {
        if (uamqp_app_properties == NULL)
        {
            result = 0;
        }
        else
        {
            if ((uamqp_app_properties_ipdv = amqpvalue_get_inplace_described_value(uamqp_app_properties)) == NULL)
            {
                LogError("Failed getting the map of uAMQP message application properties (return code %d).", result);
                result = MU_FAILURE;
            }
            else if ((result = amqpvalue_get_map_pair_count(uamqp_app_properties_ipdv, &property_count)) != 0)
            {
                LogError("Failed reading the number of values in the uAMQP property map (return code %d).", result);
                result = MU_FAILURE;
            }
            else
            {
                uint32_t i;
                for (i = 0; result == RESULT_OK && i < property_count; i++)
                {
                    AMQP_VALUE map_key_name = NULL;
                    AMQP_VALUE map_key_value = NULL;
                    const char *key_name;
                    const char* key_value;

                    if ((result = amqpvalue_get_map_key_value_pair(uamqp_app_properties_ipdv, i, &map_key_name, &map_key_value)) != 0)
                    {
                        LogError("Failed reading the key/value pair from the uAMQP property map (return code %d).", result);
                        result = MU_FAILURE;
                    }

                    else if ((result = amqpvalue_get_string(map_key_name, &key_name)) != 0)
                    {
                        LogError("Failed parsing the uAMQP property name (return code %d).", result);
                        result = MU_FAILURE;
                    }
                    else if ((result = amqpvalue_get_string(map_key_value, &key_value)) != 0)
                    {
                        LogError("Failed parsing the uAMQP property value (return code %d).", result);
                        result = MU_FAILURE;
                    }
                    else if (Map_AddOrUpdate(iothub_message_properties_map, key_name, key_value) != MAP_OK)
                    {
                        LogError("Failed to add/update IoTHub message property map.");
                        result = MU_FAILURE;
                    }

                    if (map_key_name != NULL)
                    {
                        amqpvalue_destroy(map_key_name);
                    }

                    if (map_key_value != NULL)
                    {
                        amqpvalue_destroy(map_key_value);
                    }
                }
            }

            amqpvalue_destroy(uamqp_app_properties);
        }
    }

    return result;
}

int message_create_IoTHubMessage_from_uamqp_message(MESSAGE_HANDLE uamqp_message, IOTHUB_MESSAGE_HANDLE* iothubclient_message)
{
    int result = MU_FAILURE;

    IOTHUB_MESSAGE_HANDLE iothub_message = NULL;
    MESSAGE_BODY_TYPE body_type;

    if (message_get_body_type(uamqp_message, &body_type) != 0)
    {
        LogError("Failed to get the type of the uamqp message.");
        result = MU_FAILURE;
    }
    else
    {
        if (body_type == MESSAGE_BODY_TYPE_DATA)
        {
            BINARY_DATA binary_data;
            if (message_get_body_amqp_data_in_place(uamqp_message, 0, &binary_data) != 0)
            {
                LogError("Failed to get the body of the uamqp message.");
                result = MU_FAILURE;
            }
            else if ((iothub_message = IoTHubMessage_CreateFromByteArray(binary_data.bytes, binary_data.length)) == NULL)
            {
                LogError("Failed creating the IOTHUB_MESSAGE_HANDLE instance (IoTHubMessage_CreateFromByteArray failed).");
                result = MU_FAILURE;
            }
        }
    }

    if (iothub_message != NULL)
    {
        if (readPropertiesFromuAMQPMessage(iothub_message, uamqp_message) != RESULT_OK)
        {
            LogError("Failed reading properties of the uamqp message.");
            IoTHubMessage_Destroy(iothub_message);
            result = MU_FAILURE;
        }
        else if (readApplicationPropertiesFromuAMQPMessage(iothub_message, uamqp_message) != RESULT_OK)
        {
            LogError("Failed reading application properties of the uamqp message.");
            IoTHubMessage_Destroy(iothub_message);
            result = MU_FAILURE;
        }
        else
        {
            *iothubclient_message = iothub_message;
            result = RESULT_OK;
        }
    }

    return result;
}

