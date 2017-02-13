// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "mqtt_message.h"
#include "gballoc.h"

typedef struct MQTT_MESSAGE_TAG
{
    uint16_t packetId;
    char* topicName;
    QOS_VALUE qosInfo;
    APP_PAYLOAD appPayload;
    bool isDuplicateMsg;
    bool isMessageRetained;
} MQTT_MESSAGE;

MQTT_MESSAGE_HANDLE mqttmessage_create(uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength)
{
    /* Codes_SRS_MQTTMESSAGE_07_001:[If the parameters topicName is NULL is zero then mqttmessage_create shall return NULL.] */
    MQTT_MESSAGE* result;
    if (topicName == NULL)
    {
        result = NULL;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_002: [mqttmessage_create shall allocate and copy the topicName and appMsg parameters.] */
        result = malloc(sizeof(MQTT_MESSAGE));
        if (result != NULL)
        {
            if (mallocAndStrcpy_s(&result->topicName, topicName) != 0)
            {
                /* Codes_SRS_MQTTMESSAGE_07_003: [If any memory allocation fails mqttmessage_create shall free any allocated memory and return NULL.] */
                free(result);
                result = NULL;
            }
            else
            {
                result->packetId = packetId;
                result->isDuplicateMsg = false;
                result->isMessageRetained = false;
                result->qosInfo = qosValue;

                /* Codes_SRS_MQTTMESSAGE_07_002: [mqttmessage_create shall allocate and copy the topicName and appMsg parameters.] */
                result->appPayload.length = appMsgLength;
                if (result->appPayload.length > 0)
                {
                    result->appPayload.message = malloc(appMsgLength);
                    if (result->appPayload.message == NULL)
                    {
                        /* Codes_SRS_MQTTMESSAGE_07_003: [If any memory allocation fails mqttmessage_create shall free any allocated memory and return NULL.] */
                        free(result->topicName);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        memcpy(result->appPayload.message, appMsg, appMsgLength);
                    }
                }
                else
                {
                    result->appPayload.message = NULL;
                }
            }
        }
    }
    /* Codes_SRS_MQTTMESSAGE_07_004: [If mqttmessage_createMessage succeeds the it shall return a NON-NULL MQTT_MESSAGE_HANDLE value.] */
    return result;
}

void mqttmessage_destroy(MQTT_MESSAGE_HANDLE handle)
{
    MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
    /* Codes_SRS_MQTTMESSAGE_07_005: [If the handle parameter is NULL then mqttmessage_destroyMessage shall do nothing] */
    if (msgInfo != NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_006: [mqttmessage_destroyMessage shall free all resources associated with the MQTT_MESSAGE_HANDLE value] */
        free(msgInfo->topicName);
        if (msgInfo->appPayload.message != NULL)
        {
            free(msgInfo->appPayload.message);
        }
        free(msgInfo);
    }
}

MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle)
{
    MQTT_MESSAGE_HANDLE result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_007: [If handle parameter is NULL then mqttmessage_clone shall return NULL.] */
        result = NULL;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_008: [mqttmessage_clone shall create a new MQTT_MESSAGE_HANDLE with data content identical of the handle value.] */
        MQTT_MESSAGE* mqtt_message = (MQTT_MESSAGE*)handle;
        result = mqttmessage_create(mqtt_message->packetId, mqtt_message->topicName, mqtt_message->qosInfo, mqtt_message->appPayload.message, mqtt_message->appPayload.length);
        if (result != NULL)
        {
            (void)mqttmessage_setIsDuplicateMsg(result, mqtt_message->isDuplicateMsg);
            (void)mqttmessage_setIsRetained(result, mqtt_message->isMessageRetained);
        }
    }
    return result;
}

uint16_t mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle)
{
    uint16_t result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_010: [If handle is NULL then mqttmessage_getPacketId shall return 0.] */
        result = 0;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_011: [mqttmessage_getPacketId shall return the packetId value contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = msgInfo->packetId;
    }
    return result;
}

const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_012: [If handle is NULL then mqttmessage_getTopicName shall return a NULL string.] */
        result = NULL;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_013: [mqttmessage_getTopicName shall return the topicName contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = msgInfo->topicName;
    }
    return result;
}

QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle)
{
    QOS_VALUE result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_014: [If handle is NULL then mqttmessage_getQosType shall return the default DELIVER_AT_MOST_ONCE value.] */
        result = DELIVER_AT_MOST_ONCE;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_015: [mqttmessage_getQosType shall return the QOS Type value contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = msgInfo->qosInfo;
    }
    return result;
}

bool mqttmessage_getIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_016: [If handle is NULL then mqttmessage_getIsDuplicateMsg shall return false.] */
        result = false;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_017: [mqttmessage_getIsDuplicateMsg shall return the isDuplicateMsg value contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = msgInfo->isDuplicateMsg;
    }
    return result;
}

bool mqttmessage_getIsRetained(MQTT_MESSAGE_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_018: [If handle is NULL then mqttmessage_getIsRetained shall return false.] */
        result = false;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_019: [mqttmessage_getIsRetained shall return the isRetained value contained in MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = msgInfo->isMessageRetained;
    }
    return result;
}

int mqttmessage_setIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle, bool duplicateMsg)
{
    int result;
    /* Codes_SRS_MQTTMESSAGE_07_022: [If handle is NULL then mqttmessage_setIsDuplicateMsg shall return a non-zero value.] */
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_023: [mqttmessage_setIsDuplicateMsg shall store the duplicateMsg value in the MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        msgInfo->isDuplicateMsg = duplicateMsg;
        result = 0;
    }
    return result;
}

int mqttmessage_setIsRetained(MQTT_MESSAGE_HANDLE handle, bool retainMsg)
{
    int result;
    /* Codes_SRS_MQTTMESSAGE_07_024: [If handle is NULL then mqttmessage_setIsRetained shall return a non-zero value.] */
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_025: [mqttmessage_setIsRetained shall store the retainMsg value in the MQTT_MESSAGE_HANDLE handle.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        msgInfo->isMessageRetained = retainMsg;
        result = 0;
    }
    return result;
}

const APP_PAYLOAD* mqttmessage_getApplicationMsg(MQTT_MESSAGE_HANDLE handle)
{
    const APP_PAYLOAD* result;
    if (handle == NULL)
    {
        /* Codes_SRS_MQTTMESSAGE_07_020: [If handle is NULL or if msgLen is 0 then mqttmessage_getApplicationMsg shall return NULL.] */
        result = NULL;
    }
    else
    {
        /* Codes_SRS_MQTTMESSAGE_07_021: [mqttmessage_getApplicationMsg shall return the applicationMsg value contained in MQTT_MESSAGE_HANDLE handle and the length of the appMsg in the msgLen parameter.] */
        MQTT_MESSAGE* msgInfo = (MQTT_MESSAGE*)handle;
        result = &msgInfo->appPayload;
    }
    return result;
}
