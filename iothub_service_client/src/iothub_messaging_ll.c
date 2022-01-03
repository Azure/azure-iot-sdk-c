// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/cbs.h"

#include "parson.h"

#include "iothub_messaging_ll.h"
#include "iothub_sc_version.h"

#define SIZE_OF_PERCENT_S_IN_FMT_STRING 2

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_FEEDBACK_STATUS_CODE, IOTHUB_FEEDBACK_STATUS_CODE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_MESSAGE_SEND_STATE, IOTHUB_MESSAGE_SEND_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_MESSAGING_RESULT, IOTHUB_MESSAGING_RESULT_VALUES);

typedef struct CALLBACK_DATA_TAG
{
    IOTHUB_OPEN_COMPLETE_CALLBACK openCompleteCompleteCallback;
    IOTHUB_SEND_COMPLETE_CALLBACK sendCompleteCallback;
    IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK feedbackMessageCallback;
    void* openUserContext;
    void* sendUserContext;
    void* feedbackUserContext;
} CALLBACK_DATA;

typedef struct IOTHUB_MESSAGING_TAG
{
    int isOpened;
    char* hostname;
    char* iothubName;
    char* iothubSuffix;
    char* sharedAccessKey;
    char* keyName;
    char* trusted_cert;

    MESSAGE_SENDER_HANDLE message_sender;
    MESSAGE_RECEIVER_HANDLE message_receiver;
    CONNECTION_HANDLE connection;
    SESSION_HANDLE session;
    LINK_HANDLE sender_link;
    LINK_HANDLE receiver_link;
    SASL_MECHANISM_HANDLE sasl_mechanism_handle;
    SASL_PLAIN_CONFIG sasl_plain_config;
    XIO_HANDLE tls_io;
    XIO_HANDLE sasl_io;

    MESSAGE_SENDER_STATE message_sender_state;
    MESSAGE_RECEIVER_STATE message_receiver_state;

    CALLBACK_DATA* callback_data;

} IOTHUB_MESSAGING;


static const char* const FEEDBACK_RECORD_KEY_DEVICE_ID = "deviceId";
static const char* const FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID = "deviceGenerationId";
static const char* const FEEDBACK_RECORD_KEY_DESCRIPTION = "description";
static const char* const FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC = "enqueuedTimeUtc";
static const char* const FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID = "originalMessageId";
static const char* const AMQP_ADDRESS_PATH_FMT = "/devices/%s/messages/deviceBound";
static const char* const AMQP_ADDRESS_PATH_MODULE_FMT = "/devices/%s/modules/%s/messages/deviceBound";

static int setMessageId(IOTHUB_MESSAGE_HANDLE iothub_message_handle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* messageId;

    if ((messageId = IoTHubMessage_GetMessageId(iothub_message_handle)) != NULL)
    {
        AMQP_VALUE uamqp_message_id;
        if ((uamqp_message_id = amqpvalue_create_string(messageId)) == NULL)
        {
            LogError("Failed to create an AMQP_VALUE for the messageId property value.");
            result = MU_FAILURE;
        }
        else
        {
            int api_call_result;

            if ((api_call_result = properties_set_message_id(uamqp_message_properties, uamqp_message_id)) != 0)
            {
                LogInfo("Failed to set value of uAMQP message 'message-id' property (%d).", api_call_result);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            amqpvalue_destroy(uamqp_message_id);
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

static int setCorrelationId(IOTHUB_MESSAGE_HANDLE iothub_message_handle, PROPERTIES_HANDLE uamqp_message_properties)
{
    int result;
    const char* correlationId;

    if ((correlationId = IoTHubMessage_GetCorrelationId(iothub_message_handle)) != NULL)
    {
        AMQP_VALUE uamqp_correlation_id;
        if ((uamqp_correlation_id = amqpvalue_create_string(correlationId)) == NULL)
        {
            LogError("Failed to create an AMQP_VALUE for the messageId property value.");
            result = MU_FAILURE;
        }
        else
        {
            int api_call_result;

            if ((api_call_result = properties_set_correlation_id(uamqp_message_properties, uamqp_correlation_id)) != 0)
            {
                LogInfo("Failed to set value of uAMQP message 'message-id' property (%d).", api_call_result);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            amqpvalue_destroy(uamqp_correlation_id);
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

static int addPropertiesToAMQPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, MESSAGE_HANDLE uamqp_message, AMQP_VALUE to_amqp_value)
{
    int result;
    PROPERTIES_HANDLE uamqp_message_properties = NULL; /* This initialization is forced by Valgrind */
    int api_call_result;

    if ((api_call_result = message_get_properties(uamqp_message, &uamqp_message_properties)) != 0)
    {
        LogError("Failed to get properties map from uAMQP message (error code %d).", api_call_result);
        result = MU_FAILURE;
    }
    else if (uamqp_message_properties == NULL &&
        (uamqp_message_properties = properties_create()) == NULL)
    {
        LogError("Failed to create properties map for uAMQP message (error code %d).", api_call_result);
        result = MU_FAILURE;
    }
    else
    {
        if (setMessageId(iothub_message_handle, uamqp_message_properties) != 0)
        {
            LogError("Failed to set uampq messageId.");
            result = MU_FAILURE;
        }
        else if (setCorrelationId(iothub_message_handle, uamqp_message_properties) != 0)
        {
            LogError("Failed to set uampq correlationId.");
            result = MU_FAILURE;
        }
        else
        {
            if ((properties_set_to(uamqp_message_properties, to_amqp_value)) != 0)
            {
                LogError("Could not create properties for message - properties_set_to failed");
                result = MU_FAILURE;
            }
            else if ((api_call_result = message_set_properties(uamqp_message, uamqp_message_properties)) != 0)
            {
                LogError("Failed to set properties map on uAMQP message (error code %d).", api_call_result);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        properties_destroy(uamqp_message_properties);
    }
    return result;
}

static int addApplicationPropertiesToAMQPMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, MESSAGE_HANDLE uamqp_message)
{
    int result;
    MAP_HANDLE properties_map;
    const char* const* propertyKeys;
    const char* const* propertyValues;
    size_t propertyCount = 0;

    properties_map = IoTHubMessage_Properties(iothub_message_handle);

    if (Map_GetInternals(properties_map, &propertyKeys, &propertyValues, &propertyCount) != MAP_OK)
    {
        LogError("Failed to get the internals of the property map.");
        result = MU_FAILURE;
    }
    else
    {
        if (propertyCount != 0)
        {
            AMQP_VALUE uamqp_map;

            if ((uamqp_map = amqpvalue_create_map()) == NULL)
            {
                LogError("Failed to create uAMQP map for the properties.");
                result = MU_FAILURE;
            }
            else
            {
                size_t i = 0;
                for (i = 0; i < propertyCount; i++)
                {
                    AMQP_VALUE map_key_value = NULL;
                    AMQP_VALUE map_value_value = NULL;

                    if ((map_key_value = amqpvalue_create_string(propertyKeys[i])) == NULL)
                    {
                        LogError("Failed to create uAMQP property key name.");
                        break;
                    }
                    else
                    {
                        if ((map_value_value = amqpvalue_create_string(propertyValues[i])) == NULL)
                        {
                            LogError("Failed to create uAMQP property key value.");
                            break;
                        }
                        else
                        {
                            if (amqpvalue_set_map_value(uamqp_map, map_key_value, map_value_value) != 0)
                            {
                                LogError("Failed to set key/value into the the uAMQP property map.");
                                break;
                            }
                            amqpvalue_destroy(map_value_value);
                        }
                        amqpvalue_destroy(map_key_value);
                    }
                }

                if (i == propertyCount)
                {
                    if (message_set_application_properties(uamqp_message, uamqp_map) != 0)
                    {
                        LogError("Failed to transfer the message properties to the uAMQP message.");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else
                {
                    LogError("Failed to set application property into the the uAMQP property map.");
                    result = MU_FAILURE;
                }
                amqpvalue_destroy(uamqp_map);
            }
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int getMessageContentAndSize(IOTHUB_MESSAGE_HANDLE message, unsigned const char** messageContent, size_t* messageContentSize)
{
    int result;
    unsigned const char* contentByteArr;
    const char* contentStr;
    size_t contentSize;

    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message);

    switch (contentType)
    {
    case IOTHUBMESSAGE_BYTEARRAY:
        if (IoTHubMessage_GetByteArray(message, &contentByteArr, &contentSize) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failed getting the BYTE array representation of the IOTHUB_MESSAGE_HANDLE instance.");
            result = MU_FAILURE;
        }
        else
        {
            *messageContent = contentByteArr;
            *messageContentSize = contentSize;
            result = 0;
        }
        break;
    case IOTHUBMESSAGE_STRING:
        if ((contentStr = IoTHubMessage_GetString(message)) == NULL)
        {
            LogError("Failed getting the STRING representation of the IOTHUB_MESSAGE_HANDLE instance.");
            result = MU_FAILURE;
        }
        else
        {
            contentSize = strlen(contentStr);
            *messageContent = (unsigned const char*)contentStr;
            *messageContentSize = contentSize;
            result = 0;
        }
        break;
    default:
        LogError("Cannot parse IOTHUB_MESSAGE_HANDLE with content type IOTHUBMESSAGE_UNKNOWN.");
        result = MU_FAILURE;
        break;
    }
    return result;
}

static char* createSasToken(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    char* result;

    char* buffer = NULL;
    if (messagingHandle->sharedAccessKey == NULL)
    {
        LogError("createSasPlainConfig failed - sharedAccessKey cannot be NULL");
        result = NULL;
    }
    else if (messagingHandle->hostname == NULL)
    {
        LogError("createSasPlainConfig failed - hostname cannot be NULL");
        result = NULL;
    }
    else if (messagingHandle->keyName == NULL)
    {
        LogError("createSasPlainConfig failed - keyName cannot be NULL");
        result = NULL;
    }
    else
    {
        STRING_HANDLE hostName = NULL;
        STRING_HANDLE sharedAccessKey = NULL;
        STRING_HANDLE keyName = NULL;

        if ((hostName = STRING_construct(messagingHandle->hostname)) == NULL)
        {
            LogError("STRING_construct failed for hostName");
            result = NULL;
        }
        else if ((sharedAccessKey = STRING_construct(messagingHandle->sharedAccessKey)) == NULL)
        {
            LogError("STRING_construct failed for sharedAccessKey");
            result = NULL;
        }
        else if ((keyName = STRING_construct(messagingHandle->keyName)) == NULL)
        {
            LogError("STRING_construct failed for keyName");
            result = NULL;
        }
        else
        {
            time_t currentTime = time(NULL);
            size_t expiry_time = (size_t)(currentTime + (365 * 24 * 60 * 60));
            const char* c_buffer = NULL;

            STRING_HANDLE sasHandle = SASToken_Create(sharedAccessKey, hostName, keyName, expiry_time);
            if (sasHandle == NULL)
            {
                LogError("SASToken_Create failed");
                result = NULL;
            }
            else if ((c_buffer = (const char*)STRING_c_str(sasHandle)) == NULL)
            {
                LogError("STRING_c_str returned NULL");
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&buffer, c_buffer) != 0)
            {
                LogError("mallocAndStrcpy_s failed for sharedAccessToken");
                result = NULL;
            }
            else
            {
                result = buffer;
            }
            STRING_delete(sasHandle);
        }
        STRING_delete(keyName);
        STRING_delete(sharedAccessKey);
        STRING_delete(hostName);
    }
    return result;
}

static char* createAuthCid(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    char* result;

    char* buffer = NULL;
    if (messagingHandle->iothubName == NULL)
    {
        LogError("createSasPlainConfig failed - iothubName cannot be NULL");
        result = NULL;
    }
    else
    {
        const char AMQP_SEND_AUTHCID_FMT[] = "%s@sas.root.%s";
        const int AMQP_SEND_AUTHCID_FMT_LENGTH = sizeof(AMQP_SEND_AUTHCID_FMT) - 2 * SIZE_OF_PERCENT_S_IN_FMT_STRING;

        size_t authCidLen = strlen(messagingHandle->keyName) + AMQP_SEND_AUTHCID_FMT_LENGTH + strlen(messagingHandle->iothubName);

        if ((buffer = (char*)malloc(authCidLen + 1)) == NULL)
        {
            LogError("Malloc failed for authCid.");
            result = NULL;
        }
        else if ((snprintf(buffer, authCidLen + 1, AMQP_SEND_AUTHCID_FMT, messagingHandle->keyName, messagingHandle->iothubName)) < 0)
        {
            LogError("sprintf_s failed for authCid.");
            free(buffer);
            result = NULL;
        }
        else
        {
            result = buffer;
        }
    }
    return result;
}

static char* createReceiveTargetAddress(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    char* result;

    char* buffer = NULL;
    if (messagingHandle->hostname == NULL)
    {
        LogError("createSendTargetAddress failed - hostname cannot be NULL");
        result = NULL;
    }
    else
    {
        const char* AMQP_SEND_TARGET_ADDRESS_FMT = "amqps://%s/messages/servicebound/feedback";
        size_t addressLen = strlen(AMQP_SEND_TARGET_ADDRESS_FMT) + strlen(messagingHandle->hostname);

        if ((buffer = (char*)malloc(addressLen + 1)) == NULL)
        {
            LogError("Malloc failed for receiveTargetAddress");
            result = NULL;
        }
        else if ((snprintf(buffer, addressLen + 1, AMQP_SEND_TARGET_ADDRESS_FMT, messagingHandle->hostname)) < 0)
        {
            LogError("sprintf_s failed for receiveTargetAddress.");
            free((char*)buffer);
            result = NULL;
        }
        else
        {
            result = buffer;
        }
    }
    return result;
}

static char* createSendTargetAddress(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    char* result;

    char* buffer = NULL;
    if (messagingHandle->hostname == NULL)
    {
        LogError("createSendTargetAddress failed - hostname cannot be NULL");
        result = NULL;
    }
    else
    {
        const char* AMQP_SEND_TARGET_ADDRESS_FMT = "amqps://%s/messages/deviceBound";
        size_t addressLen = strlen(AMQP_SEND_TARGET_ADDRESS_FMT) + strlen(messagingHandle->hostname);

        if ((buffer = (char*)malloc(addressLen + 1)) == NULL)
        {
            LogError("Malloc failed for sendTargetAddress");
            result = NULL;
        }
        else if ((snprintf(buffer, addressLen + 1, AMQP_SEND_TARGET_ADDRESS_FMT, messagingHandle->hostname)) < 0)
        {
            LogError("sprintf_s failed for sendTargetAddress.");
            free((char*)buffer);
            result = NULL;
        }
        else
        {
            result = buffer;
        }
    }
    return result;
}

static char* createDeviceDestinationString(const char* deviceId, const char* moduleId)
{
    char* result;

    if (deviceId == NULL)
    {
        LogError("createDeviceDestinationString failed - deviceId cannot be NULL");
        result = NULL;
    }
    else
    {
        size_t deviceDestLen = strlen(AMQP_ADDRESS_PATH_MODULE_FMT) + strlen(deviceId) + (moduleId == NULL ? 0 : strlen(moduleId)) + 1;

        char* buffer = (char*)malloc(deviceDestLen);
        if (buffer == NULL)
        {
            LogError("Could not create device destination string.");
            result = NULL;
        }
        else
        {
            if ((moduleId == NULL) && (snprintf(buffer, deviceDestLen, AMQP_ADDRESS_PATH_FMT, deviceId)) < 0)
            {
                LogError("sprintf_s failed for deviceDestinationString.");
                free((char*)buffer);
                result = NULL;
            }
            else if ((moduleId != NULL) && (snprintf(buffer, deviceDestLen, AMQP_ADDRESS_PATH_MODULE_FMT, deviceId, moduleId)) < 0)
            {
                LogError("sprintf_s failed for deviceDestinationString for module.");
                free((char*)buffer);
                result = NULL;
            }
            else
            {
                result = buffer;
            }
        }
    }
    return result;
}

static void IoTHubMessaging_LL_SenderStateChanged(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
    (void)previous_state;
    if (context != NULL)
    {
        IOTHUB_MESSAGING* messagingData = (IOTHUB_MESSAGING*)context;
        messagingData->message_sender_state = new_state;

        if ((messagingData->message_sender_state == MESSAGE_SENDER_STATE_OPEN) && (messagingData->message_receiver_state == MESSAGE_RECEIVER_STATE_OPEN))
        {
            messagingData->isOpened = true;
            if (messagingData->callback_data->openCompleteCompleteCallback != NULL)
            {
                (messagingData->callback_data->openCompleteCompleteCallback)(messagingData->callback_data->openUserContext);
            }
        }
        else
        {
            messagingData->isOpened = false;
        }
    }
}

static void IoTHubMessaging_LL_ReceiverStateChanged(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
    (void)previous_state;
    if (context != NULL)
    {
        IOTHUB_MESSAGING* messagingData = (IOTHUB_MESSAGING*)context;
        messagingData->message_receiver_state = new_state;

        if ((messagingData->message_sender_state == MESSAGE_SENDER_STATE_OPEN) && (messagingData->message_receiver_state == MESSAGE_RECEIVER_STATE_OPEN))
        {
            messagingData->isOpened = true;
            if (messagingData->callback_data->openCompleteCompleteCallback != NULL)
            {
                (messagingData->callback_data->openCompleteCompleteCallback)(messagingData->callback_data->openUserContext);
            }
        }
        else
        {
            messagingData->isOpened = false;
        }
    }
}

static void IoTHubMessaging_LL_SendMessageComplete(void* context, MESSAGE_SEND_RESULT send_result, AMQP_VALUE delivery_state)
{
    (void)delivery_state;
    if (context != NULL)
    {
        IOTHUB_MESSAGING* messagingData = (IOTHUB_MESSAGING*)context;
        if (messagingData->callback_data->sendCompleteCallback != NULL)
        {
            // Convert a send result to an
            IOTHUB_MESSAGING_RESULT msg_result;
            switch (send_result)
            {
                case MESSAGE_SEND_OK:
                    msg_result = IOTHUB_MESSAGING_OK;
                    break;
                case MESSAGE_SEND_ERROR:
                case MESSAGE_SEND_TIMEOUT:
                case MESSAGE_SEND_CANCELLED:
                default:
                    msg_result = IOTHUB_MESSAGING_ERROR;
                    break;
            }
            (messagingData->callback_data->sendCompleteCallback)(messagingData->callback_data->sendUserContext, msg_result);
        }
    }
}

static AMQP_VALUE IoTHubMessaging_LL_FeedbackMessageReceived(const void* context, MESSAGE_HANDLE message)
{
    AMQP_VALUE result;

    if (context == NULL)
    {
        result = messaging_delivery_accepted();
    }
    else
    {
        IOTHUB_MESSAGING* messagingData = (IOTHUB_MESSAGING*)context;

        BINARY_DATA binary_data;
        JSON_Value* root_value = NULL;
        JSON_Object* feedback_object = NULL;
        JSON_Array* feedback_array = NULL;

        if (message_get_body_amqp_data_in_place(message, 0, &binary_data) != 0)
        {
            LogError("Cannot get message data");
            result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading message body");
        }
        else if ((root_value = json_parse_string((const char*)binary_data.bytes)) == NULL)
        {
            LogError("json_parse_string failed");
            result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed parsing json root");
        }
        else if ((feedback_array = json_value_get_array(root_value)) == NULL)
        {
            LogError("json_parse_string failed");
            result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed parsing json array");
        }
        else if (json_array_get_count(feedback_array) == 0)
        {
            LogError("json_array_get_count failed");
            result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "json_array_get_count failed");
        }
        else
        {
            IOTHUB_SERVICE_FEEDBACK_BATCH* feedbackBatch;

            if ((feedbackBatch = (IOTHUB_SERVICE_FEEDBACK_BATCH*)malloc(sizeof(IOTHUB_SERVICE_FEEDBACK_BATCH))) == NULL)
            {
                LogError("json_parse_string failed");
                result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed to allocate memory for feedback batch");
            }
            else
            {
                size_t array_count = 0;
                if ((array_count = json_array_get_count(feedback_array)) <= 0)
                {
                    LogError("json_array_get_count failed");
                    free(feedbackBatch);
                    result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "json_array_get_count failed");
                }
                else if ((feedbackBatch->feedbackRecordList = singlylinkedlist_create()) == NULL)
                {
                    LogError("singlylinkedlist_create failed");
                    free(feedbackBatch);
                    result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "singlylinkedlist_create failed");
                }
                else
                {
                    bool isLoopFailed = false;
                    for (size_t i = 0; i < array_count; i++)
                    {
                        if ((feedback_object = json_array_get_object(feedback_array, i)) == NULL)
                        {
                            isLoopFailed = true;
                            break;
                        }
                        else
                        {
                            IOTHUB_SERVICE_FEEDBACK_RECORD* feedbackRecord;
                            if ((feedbackRecord = (IOTHUB_SERVICE_FEEDBACK_RECORD*)malloc(sizeof(IOTHUB_SERVICE_FEEDBACK_RECORD))) == NULL)
                            {
                                isLoopFailed = true;
                                break;
                            }
                            else
                            {
                                feedbackRecord->deviceId = (char*)json_object_get_string(feedback_object, FEEDBACK_RECORD_KEY_DEVICE_ID);
                                feedbackRecord->generationId = (char*)json_object_get_string(feedback_object, FEEDBACK_RECORD_KEY_DEVICE_GENERATION_ID);
                                feedbackRecord->description = (char*)json_object_get_string(feedback_object, FEEDBACK_RECORD_KEY_DESCRIPTION);
                                feedbackRecord->enqueuedTimeUtc = (char*)json_object_get_string(feedback_object, FEEDBACK_RECORD_KEY_ENQUED_TIME_UTC);
                                feedbackRecord->originalMessageId = (char*)json_object_get_string(feedback_object, FEEDBACK_RECORD_KEY_ORIGINAL_MESSAGE_ID);
                                feedbackRecord->correlationId = "";

                                if (feedbackRecord->description == NULL)
                                {
                                    feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
                                }
                                else
                                {
                                    size_t j;
                                    for (j = 0; feedbackRecord->description[j]; j++)
                                    {
                                        feedbackRecord->description[j] = (char)tolower(feedbackRecord->description[j]);
                                    }

                                    if (strcmp(feedbackRecord->description, "success") == 0)
                                    {
                                        feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_SUCCESS;
                                    }
                                    else if (strcmp(feedbackRecord->description, "expired") == 0)
                                    {
                                        feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_EXPIRED;
                                    }
                                    else if (strcmp(feedbackRecord->description, "deliverycountexceeded") == 0)
                                    {
                                        feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_DELIVER_COUNT_EXCEEDED;
                                    }
                                    else if (strcmp(feedbackRecord->description, "rejected") == 0)
                                    {
                                        feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_REJECTED;
                                    }
                                    else
                                    {
                                        feedbackRecord->statusCode = IOTHUB_FEEDBACK_STATUS_CODE_UNKNOWN;
                                    }
                                }
                                if (singlylinkedlist_add(feedbackBatch->feedbackRecordList, feedbackRecord) == NULL)
                                {
                                    LogError("singlylinkedlist_add failed");
                                    free(feedbackRecord);
                                }
                            }
                        }
                    }
                    feedbackBatch->lockToken = "";
                    feedbackBatch->userId = "";

                    if (isLoopFailed)
                    {
                        LogError("Failed to read feedback records");
                        result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed to read feedback records");
                    }
                    else
                    {
                        if (messagingData->callback_data->feedbackMessageCallback != NULL)
                        {
                            (messagingData->callback_data->feedbackMessageCallback)(messagingData->callback_data->feedbackUserContext, feedbackBatch);
                        }
                        result = messaging_delivery_accepted();
                    }

                    LIST_ITEM_HANDLE feedbackRecord = singlylinkedlist_get_head_item(feedbackBatch->feedbackRecordList);
                    while (feedbackRecord != NULL)
                    {
                        IOTHUB_SERVICE_FEEDBACK_RECORD* feedback = (IOTHUB_SERVICE_FEEDBACK_RECORD*)singlylinkedlist_item_get_value(feedbackRecord);
                        feedbackRecord = singlylinkedlist_get_next_item(feedbackRecord);
                        free(feedback);
                    }
                    singlylinkedlist_destroy(feedbackBatch->feedbackRecordList);
                    free(feedbackBatch);
                }
            }
        }
        json_array_clear(feedback_array);
        json_value_free(root_value);
    }
    return result;
}

IOTHUB_MESSAGING_HANDLE IoTHubMessaging_LL_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    IOTHUB_MESSAGING_HANDLE result;
    CALLBACK_DATA* callback_data;

    if (serviceClientHandle == NULL)
    {
        LogError("serviceClientHandle input parameter cannot be NULL");
        result = NULL;
    }
    else
    {
        IOTHUB_SERVICE_CLIENT_AUTH* serviceClientAuth = (IOTHUB_SERVICE_CLIENT_AUTH*)serviceClientHandle;

        if (serviceClientAuth->hostname == NULL)
        {
            LogError("authInfo->hostName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->iothubName == NULL)
        {
            LogError("authInfo->iothubName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->iothubSuffix == NULL)
        {
            LogError("authInfo->iothubSuffix input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->keyName == NULL)
        {
            LogError("authInfo->keyName input parameter cannot be NULL");
            result = NULL;
        }
        else if (serviceClientAuth->sharedAccessKey == NULL)
        {
            LogError("authInfo->sharedAccessKey input parameter cannot be NULL");
            result = NULL;
        }
        else if ((result = (IOTHUB_MESSAGING*)malloc(sizeof(IOTHUB_MESSAGING))) == NULL)
        {
            LogError("Malloc failed for IOTHUB_REGISTRYMANAGER");
        }
        else
        {
            memset(result, 0, sizeof(IOTHUB_MESSAGING) );

            if (mallocAndStrcpy_s(&result->hostname, serviceClientAuth->hostname) != 0)
            {
                LogError("mallocAndStrcpy_s failed for hostName");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->iothubName, serviceClientAuth->iothubName) != 0)
            {
                LogError("mallocAndStrcpy_s failed for iothubName");
                free(result->hostname);
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->iothubSuffix, serviceClientAuth->iothubSuffix) != 0)
            {
                LogError("mallocAndStrcpy_s failed for iothubSuffix");
                free(result->hostname);
                free(result->iothubName);
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->sharedAccessKey, serviceClientAuth->sharedAccessKey) != 0)
            {
                LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                free(result->hostname);
                free(result->iothubName);
                free(result->iothubSuffix);
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->keyName, serviceClientAuth->keyName) != 0)
            {
                LogError("mallocAndStrcpy_s failed for keyName");
                free(result->hostname);
                free(result->iothubName);
                free(result->iothubSuffix);
                free(result->sharedAccessKey);
                free(result);
                result = NULL;
            }
            else if ((callback_data = (CALLBACK_DATA*)malloc(sizeof(CALLBACK_DATA))) == NULL)
            {
                LogError("Malloc failed for callback_data");
                free(result->hostname);
                free(result->iothubName);
                free(result->iothubSuffix);
                free(result->sharedAccessKey);
                free(result->keyName);
                free(result);
                result = NULL;
            }
            else
            {
                callback_data->openCompleteCompleteCallback = NULL;
                callback_data->sendCompleteCallback = NULL;
                callback_data->feedbackMessageCallback = NULL;
                callback_data->openUserContext = NULL;
                callback_data->sendUserContext = NULL;
                callback_data->feedbackUserContext = NULL;

                result->callback_data = callback_data;
            }
        }
    }
    return result;
}

void IoTHubMessaging_LL_Destroy(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    if (messagingHandle != NULL)
    {
        IOTHUB_MESSAGING* messHandle = (IOTHUB_MESSAGING*)messagingHandle;

        free(messHandle->callback_data);
        free(messHandle->hostname);
        free(messHandle->iothubName);
        free(messHandle->iothubSuffix);
        free(messHandle->sharedAccessKey);
        free(messHandle->keyName);
        free(messHandle->trusted_cert);
        free(messHandle);
    }
}

static int attachServiceClientTypeToLink(LINK_HANDLE link)
{
    fields attach_properties;
    AMQP_VALUE serviceClientTypeKeyName;
    AMQP_VALUE serviceClientTypeValue;
    int result;

    if ((attach_properties = amqpvalue_create_map()) == NULL)
    {
        LogError("Failed to create the map for service client type.");
        result = MU_FAILURE;
    }
    else
    {
        if ((serviceClientTypeKeyName = amqpvalue_create_symbol("com.microsoft:client-version")) == NULL)
        {
            LogError("Failed to create the key name for the service client type.");
            result = MU_FAILURE;
        }
        else
        {
            if ((serviceClientTypeValue = amqpvalue_create_string(IOTHUB_SERVICE_CLIENT_TYPE_PREFIX IOTHUB_SERVICE_CLIENT_BACKSLASH IOTHUB_SERVICE_CLIENT_VERSION)) == NULL)
            {
                LogError("Failed to create the key value for the service client type.");
                result = MU_FAILURE;
            }
            else
            {
                if ((result = amqpvalue_set_map_value(attach_properties, serviceClientTypeKeyName, serviceClientTypeValue)) != 0)
                {
                    LogError("Failed to set the property map for the service client type.  Error code is: %d", result);
                }
                else if ((result = link_set_attach_properties(link, attach_properties)) != 0)
                {
                    LogError("Unable to attach the service client type to the link properties. Error code is: %d", result);
                }
                else
                {
                    result = 0;
                }

                amqpvalue_destroy(serviceClientTypeValue);
            }

            amqpvalue_destroy(serviceClientTypeKeyName);
        }

        amqpvalue_destroy(attach_properties);
    }
    return result;
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_LL_Open(IOTHUB_MESSAGING_HANDLE messagingHandle, IOTHUB_OPEN_COMPLETE_CALLBACK openCompleteCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    char* send_target_address = NULL;
    char* receive_target_address = NULL;

    TLSIO_CONFIG tls_io_config;
    SASLCLIENTIO_CONFIG sasl_io_config;

    AMQP_VALUE sendSource = NULL;
    AMQP_VALUE sendTarget = NULL;

    AMQP_VALUE receiveSource = NULL;
    AMQP_VALUE receiveTarget = NULL;

    if (messagingHandle == NULL)
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else if (messagingHandle->isOpened != 0)
    {
        LogError("Messaging is already opened");
        result = IOTHUB_MESSAGING_OK;
    }
    else
    {
        messagingHandle->message_sender = NULL;
        messagingHandle->connection = NULL;
        messagingHandle->session = NULL;
        messagingHandle->sender_link = NULL;
        messagingHandle->sasl_plain_config.authzid = NULL;
        messagingHandle->sasl_mechanism_handle = NULL;
        messagingHandle->tls_io = NULL;
        messagingHandle->sasl_io = NULL;

        if ((send_target_address = createSendTargetAddress(messagingHandle)) == NULL)
        {
            LogError("Could not create sendTargetAddress");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else if ((receive_target_address = createReceiveTargetAddress(messagingHandle)) == NULL)
        {
            LogError("Could not create receiveTargetAddress");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else if ((messagingHandle->sasl_plain_config.authcid = createAuthCid(messagingHandle)) == NULL)
        {
            LogError("Could not create authCid");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else if ((messagingHandle->sasl_plain_config.passwd = createSasToken(messagingHandle)) == NULL)
        {
            LogError("Could not create sasToken");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface;

            if ((sasl_mechanism_interface = saslplain_get_interface()) == NULL)
            {
                LogError("Could not get SASL plain mechanism interface.");
                result = IOTHUB_MESSAGING_ERROR;
            }
            else if ((messagingHandle->sasl_mechanism_handle = saslmechanism_create(sasl_mechanism_interface, &messagingHandle->sasl_plain_config)) == NULL)
            {
                LogError("Could not create SASL plain mechanism.");
                result = IOTHUB_MESSAGING_ERROR;
            }
            else
            {
                tls_io_config.hostname = messagingHandle->hostname;
                tls_io_config.port = 5671;
                tls_io_config.underlying_io_interface = NULL;
                tls_io_config.underlying_io_parameters = NULL;

                const IO_INTERFACE_DESCRIPTION* tlsio_interface;

                if ((tlsio_interface = platform_get_default_tlsio()) == NULL)
                {
                    LogError("Could not get default TLS IO interface.");
                    result = IOTHUB_MESSAGING_ERROR;
                }
                else if ((messagingHandle->tls_io = xio_create(tlsio_interface, &tls_io_config)) == NULL)
                {
                    LogError("Could not create TLS IO.");
                    result = IOTHUB_MESSAGING_ERROR;
                }
                else if (messagingHandle->trusted_cert != NULL && xio_setoption(messagingHandle->tls_io, OPTION_TRUSTED_CERT, messagingHandle->trusted_cert) != 0)
                {
                    LogError("Could set tlsio trusted certificate.");
                    xio_destroy(messagingHandle->tls_io);
                    messagingHandle->tls_io = NULL;
                    result = IOTHUB_MESSAGING_ERROR;
                }
                else
                {
                    messagingHandle->callback_data->openCompleteCompleteCallback = openCompleteCallback;
                    messagingHandle->callback_data->openUserContext = userContextCallback;

                    sasl_io_config.sasl_mechanism = messagingHandle->sasl_mechanism_handle;
                    sasl_io_config.underlying_io = messagingHandle->tls_io;

                    const IO_INTERFACE_DESCRIPTION* saslclientio_interface;

                    if ((saslclientio_interface = saslclientio_get_interface_description()) == NULL)
                    {
                        LogError("Could not create get SASL IO interface description.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->sasl_io = xio_create(saslclientio_interface, &sasl_io_config)) == NULL)
                    {
                        LogError("Could not create SASL IO.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->connection = connection_create(messagingHandle->sasl_io, messagingHandle->hostname, "some", NULL, NULL)) == NULL)
                    {
                        LogError("Could not create connection.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->session = session_create(messagingHandle->connection, NULL, NULL)) == NULL)
                    {
                        LogError("Could not create session.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (session_set_incoming_window(messagingHandle->session, 2147483647) != 0)
                    {
                        LogError("Could not set incoming window.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (session_set_outgoing_window(messagingHandle->session, 255 * 1024) != 0)
                    {
                        LogError("Could not set outgoing window.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((sendSource = messaging_create_source("ingress")) == NULL)
                    {
                        LogError("Could not create source for link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((sendTarget = messaging_create_target(send_target_address)) == NULL)
                    {
                        LogError("Could not create target for link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->sender_link = link_create(messagingHandle->session, "sender-link", role_sender, sendSource, sendTarget)) == NULL)
                    {
                        LogError("Could not create link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (attachServiceClientTypeToLink(messagingHandle->sender_link) != 0)
                    {
                        LogError("Could not set the sender attach properties.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (link_set_snd_settle_mode(messagingHandle->sender_link, sender_settle_mode_unsettled) != 0)
                    {
                        LogError("Could not set the sender settle mode.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->message_sender = messagesender_create(messagingHandle->sender_link, IoTHubMessaging_LL_SenderStateChanged, messagingHandle)) == NULL)
                    {
                        LogError("Could not create message sender.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (messagesender_open(messagingHandle->message_sender) != 0)
                    {
                        LogError("Could not open the message sender.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((receiveSource = messaging_create_source(receive_target_address)) == NULL)
                    {
                        LogError("Could not create source for link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((receiveTarget = messaging_create_target("receiver_001")) == NULL)
                    {
                        LogError("Could not create target for link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->receiver_link = link_create(messagingHandle->session, "receiver-link", role_receiver, receiveSource, receiveTarget)) == NULL)
                    {
                        LogError("Could not create link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (attachServiceClientTypeToLink(messagingHandle->receiver_link) != 0)
                    {
                        LogError("Could not create link.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (link_set_rcv_settle_mode(messagingHandle->receiver_link, receiver_settle_mode_first) != 0)
                    {
                        LogError("Could not set the sender settle mode.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if ((messagingHandle->message_receiver = messagereceiver_create(messagingHandle->receiver_link, IoTHubMessaging_LL_ReceiverStateChanged, messagingHandle)) == NULL)
                    {
                        LogError("Could not create message receiver.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (messagereceiver_open(messagingHandle->message_receiver, IoTHubMessaging_LL_FeedbackMessageReceived, messagingHandle) != 0)
                    {
                        LogError("Could not open the message receiver.");
                        messagereceiver_destroy(messagingHandle->message_receiver);
                        messagingHandle->message_receiver = NULL;
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else
                    {
                        messagingHandle->isOpened = true;
                        result = IOTHUB_MESSAGING_OK;
                    }
                }
            }
            if (result != IOTHUB_MESSAGING_OK)
            {
                if (messagingHandle->message_sender != NULL)
                {
                    messagesender_destroy(messagingHandle->message_sender);
                    messagingHandle->message_sender = NULL;
                }
                if (messagingHandle->tls_io != NULL)
                {
                    xio_destroy(messagingHandle->tls_io);
                    messagingHandle->tls_io = NULL;
                }
                if (messagingHandle->sasl_io != NULL)
                {
                    xio_destroy(messagingHandle->sasl_io);
                    messagingHandle->sasl_io = NULL;
                }
                if (messagingHandle->session != NULL)
                {
                    session_destroy(messagingHandle->session);
                    messagingHandle->session = NULL;
                }
                if (messagingHandle->connection != NULL)
                {
                    connection_destroy(messagingHandle->connection);
                    messagingHandle->connection = NULL;
                }
                if (messagingHandle->receiver_link != NULL)
                {
                    link_destroy(messagingHandle->receiver_link);
                    messagingHandle->receiver_link = NULL;
                }
            }
        }
        if (result != IOTHUB_MESSAGING_OK)
        {
            if (messagingHandle->sasl_plain_config.authcid != NULL)
            {
                free((char*)messagingHandle->sasl_plain_config.authcid);
                messagingHandle->sasl_plain_config.authcid = NULL;
            }
            if (messagingHandle->sasl_plain_config.passwd != NULL)
            {
                free((char*)messagingHandle->sasl_plain_config.passwd);
                messagingHandle->sasl_plain_config.passwd = NULL;
            }
        }
    }
    amqpvalue_destroy(sendSource);
    amqpvalue_destroy(sendTarget);
    amqpvalue_destroy(receiveSource);
    amqpvalue_destroy(receiveTarget);

    if (send_target_address != NULL)
    {
        free(send_target_address);
    }
    if (receive_target_address != NULL)
    {
        free(receive_target_address);
    }
    return result;
}

void IoTHubMessaging_LL_Close(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    if (messagingHandle == NULL)
    {
        LogError("Input parameter cannot be NULL");
    }
    else
    {
        messagesender_destroy(messagingHandle->message_sender);
        messagereceiver_destroy(messagingHandle->message_receiver);

        link_destroy(messagingHandle->sender_link);
        link_destroy(messagingHandle->receiver_link);

        session_destroy(messagingHandle->session);
        connection_destroy(messagingHandle->connection);
        xio_destroy(messagingHandle->sasl_io);
        xio_destroy(messagingHandle->tls_io);
        saslmechanism_destroy(messagingHandle->sasl_mechanism_handle);

        if (messagingHandle->sasl_plain_config.authcid != NULL)
        {
            free((char*)messagingHandle->sasl_plain_config.authcid);
        }
        if (messagingHandle->sasl_plain_config.passwd != NULL)
        {
            free((char*)messagingHandle->sasl_plain_config.passwd);
        }
        if (messagingHandle->sasl_plain_config.authzid != NULL)
        {
            free((char*)messagingHandle->sasl_plain_config.authzid);
        }
        messagingHandle->isOpened = false;
    }
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_LL_SetFeedbackMessageCallback(IOTHUB_MESSAGING_HANDLE messagingHandle, IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK feedbackMessageReceivedCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    if (messagingHandle == NULL)
    {
        LogError("Input parameter cannot be NULL");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        messagingHandle->callback_data->feedbackMessageCallback = feedbackMessageReceivedCallback;
        messagingHandle->callback_data->feedbackUserContext = userContextCallback;
        result = IOTHUB_MESSAGING_OK;
    }
    return result;
}


IOTHUB_MESSAGING_RESULT IoTHubMessaging_LL_Send(IOTHUB_MESSAGING_HANDLE messagingHandle, const char* deviceId, IOTHUB_MESSAGE_HANDLE message, IOTHUB_SEND_COMPLETE_CALLBACK sendCompleteCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    // There is no support for module sending message for callers, but most of plumbing is available should this be enabled via a new API.
    const char* moduleId = NULL;

    char* deviceDestinationString;

    if (messagingHandle == NULL)
    {
        LogError("Input parameter messagingHandle cannot be NULL");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else if (deviceId == NULL)
    {
        LogError("Input parameter deviceId cannot be NULL");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else if (message == NULL)
    {
        LogError("Input parameter message cannot be NULL");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else if (messagingHandle->isOpened == 0)
    {
        LogError("Messaging is not opened - call IoTHubMessaging_LL_Open to open");
        result = IOTHUB_MESSAGING_ERROR;
    }
    else if ((deviceDestinationString = createDeviceDestinationString(deviceId, moduleId)) == NULL)
    {
        LogError("Could not create a message.");
        result = IOTHUB_MESSAGING_ERROR;
    }
    else
    {
        unsigned const char* messageContent;
        size_t messageContentSize;

        if (getMessageContentAndSize(message, &messageContent, &messageContentSize) != 0)
        {
            LogError("Failed getting the message content and message size from IOTHUB_MESSAGE_HANDLE instance.");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            MESSAGE_HANDLE amqpMessage;
            AMQP_VALUE to_amqp_value;

            if ((amqpMessage = message_create()) == NULL)
            {
                LogError("Could not create a message.");
                result = IOTHUB_MESSAGING_ERROR;
            }
            else
            {
                if ((to_amqp_value = amqpvalue_create_string(deviceDestinationString)) == NULL)
                {
                    LogError("Could not create properties for message - amqpvalue_create_string");
                    result = IOTHUB_MESSAGING_ERROR;
                }
                else
                {
                    BINARY_DATA binary_data;

                    binary_data.bytes = messageContent;
                    binary_data.length = messageContentSize;

                    if (message_add_body_amqp_data(amqpMessage, binary_data) != 0)
                    {
                        
                        LogError("Failed setting the body of the uAMQP message.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (addPropertiesToAMQPMessage(message, amqpMessage, to_amqp_value) != 0)
                    {
                        LogError("Failed setting properties of the uAMQP message.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else if (addApplicationPropertiesToAMQPMessage(message, amqpMessage) != 0)
                    {
                        LogError("Failed setting application properties of the uAMQP message.");
                        result = IOTHUB_MESSAGING_ERROR;
                    }
                    else
                    {
                        messagingHandle->callback_data->sendCompleteCallback = sendCompleteCallback;
                        messagingHandle->callback_data->sendUserContext = userContextCallback;

                        if (messagesender_send_async(messagingHandle->message_sender, amqpMessage, IoTHubMessaging_LL_SendMessageComplete, messagingHandle, 0) == NULL)
                        {
                            LogError("Could not set outgoing window.");
                            result = IOTHUB_MESSAGING_ERROR;
                        }
                        else
                        {
                            result = IOTHUB_MESSAGING_OK;
                        }
                    }
                    amqpvalue_destroy(to_amqp_value);
                }
                message_destroy(amqpMessage);
            }
        }
        free(deviceDestinationString);
    }
    return result;
}


void IoTHubMessaging_LL_DoWork(IOTHUB_MESSAGING_HANDLE messagingHandle)
{
    if (messagingHandle != 0)
    {
        connection_dowork(messagingHandle->connection);
    }
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_LL_SetTrustedCert(IOTHUB_MESSAGING_HANDLE messagingHandle, const char* trusted_cert)
{
    IOTHUB_MESSAGING_RESULT result;
    if (messagingHandle == NULL || trusted_cert == NULL)
    {
        LogError("Invalid argument messagingHandle: %p trusted_cert: %p", messagingHandle, trusted_cert);
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        char* temp_cert;
        if (mallocAndStrcpy_s(&temp_cert, trusted_cert) != 0)
        {
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            if (messagingHandle->trusted_cert != NULL)
            {
                free(messagingHandle->trusted_cert);
            }
            messagingHandle->trusted_cert = temp_cert;
            result = IOTHUB_MESSAGING_OK;
        }
    }
    return result;
}

