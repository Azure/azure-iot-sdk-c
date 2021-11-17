// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/buffer_.h"

#include "iothub_message.h"
#include "internal/iothub_message_private.h"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);

static const char* SECURITY_CLIENT_JSON_ENCODING = "application/json";

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG
{
    IOTHUBMESSAGE_CONTENT_TYPE contentType;
    union
    {
        BUFFER_HANDLE byteArray;
        STRING_HANDLE string;
    } value;
    MAP_HANDLE properties;
    char* messageId;
    char* correlationId;
    char* userDefinedContentType;
    char* contentEncoding;
    char* outputName;
    char* inputName;
    char* connectionModuleId;
    char* connectionDeviceId;
    IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA_HANDLE diagnosticData;
    bool is_security_message;
    char* creationTimeUtc;
    char* userId;
    MESSAGE_DISPOSITION_CONTEXT_HANDLE dispositionContext;
    MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION dispositionContextDestroyFunction;
}IOTHUB_MESSAGE_HANDLE_DATA;

static bool ContainsValidUsAscii(const char* asciiValue)
{
    bool result = true;
    const char* iterator = asciiValue;
    while (iterator != NULL && *iterator != '\0')
    {
        // Union of all allowed ASCII characters for the different protocols (HTTPS, MQTT, AMQP) is [dec 32, dec 126].
        if (*iterator < ' ' || *iterator > '~')
        {
            result = false;
            break;
        }
        iterator++;
    }

    return result;
}

static int ValidateAsciiCharactersFilter(const char* mapKey, const char* mapValue)
{
    int result;
    if (!ContainsValidUsAscii(mapKey) || !ContainsValidUsAscii(mapValue))
    {
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

static void DestroyDiagnosticPropertyData(IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA_HANDLE diagnosticHandle)
{
    if (diagnosticHandle != NULL)
    {
        free(diagnosticHandle->diagnosticId);
        free(diagnosticHandle->diagnosticCreationTimeUtc);
    }
    free(diagnosticHandle);
}

static void DestroyMessageData(IOTHUB_MESSAGE_HANDLE_DATA* handleData)
{
    if (handleData->contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        BUFFER_delete(handleData->value.byteArray);
    }
    else if (handleData->contentType == IOTHUBMESSAGE_STRING)
    {
        STRING_delete(handleData->value.string);
    }

    Map_Destroy(handleData->properties);
    free(handleData->messageId);
    handleData->messageId = NULL;
    free(handleData->correlationId);
    handleData->correlationId = NULL;
    free(handleData->userDefinedContentType);
    free(handleData->contentEncoding);
    DestroyDiagnosticPropertyData(handleData->diagnosticData);
    free(handleData->outputName);
    free(handleData->inputName);
    free(handleData->connectionModuleId);
    free(handleData->connectionDeviceId);
    free(handleData->creationTimeUtc);
    free(handleData->userId);

    if (handleData->dispositionContext != NULL && handleData->dispositionContextDestroyFunction != NULL)
    {
        handleData->dispositionContextDestroyFunction(handleData->dispositionContext);
    }

    free(handleData);
}

static int set_content_encoding(IOTHUB_MESSAGE_HANDLE_DATA* handleData, const char* encoding)
{
    int result;
    char* tmp_encoding;

    if (mallocAndStrcpy_s(&tmp_encoding, encoding) != 0)
    {
        LogError("Failed saving a copy of contentEncoding");
        result = MU_FAILURE;
    }
    else
    {
        if (handleData->contentEncoding != NULL)
        {
            free(handleData->contentEncoding);
        }
        handleData->contentEncoding = tmp_encoding;
        result = 0;
    }
    return result;
}

static int set_message_creation_time(IOTHUB_MESSAGE_HANDLE_DATA* handleData, const char* messageCreationTimeUtc)
{
    int result;
    char* tmp_message_creation_time;

    if (handleData->creationTimeUtc != NULL)
    {
        free(handleData->creationTimeUtc);
        handleData->creationTimeUtc = NULL;
    }

    if (mallocAndStrcpy_s(&tmp_message_creation_time, messageCreationTimeUtc) != 0)
    {
        LogError("Failed saving a copy of messageCreationTimeUtc");
        result = MU_FAILURE;
    }
    else
    {
        handleData->creationTimeUtc = tmp_message_creation_time;
        result = 0;
    }
    return result;
}

static int set_message_user_id(IOTHUB_MESSAGE_HANDLE_DATA* handleData, const char* userId)
{
    int result;
    char* tmp_message_user_id;

    if (handleData->userId != NULL)
    {
        free(handleData->userId);
        handleData->userId = NULL;
    }

    if (mallocAndStrcpy_s(&tmp_message_user_id, userId) != 0)
    {
        LogError("Failed saving a copy of userId");
        result = MU_FAILURE;
    }
    else
    {
        handleData->userId = tmp_message_user_id;
        result = 0;
    }
    return result;
}

static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA_HANDLE CloneDiagnosticPropertyData(const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* source)
{
    IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA_HANDLE result = NULL;
    if (source == NULL)
    {
        LogError("Invalid argument - source is NULL");
    }
    else
    {
        result = (IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA_HANDLE)malloc(sizeof(IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA));
        if (result == NULL)
        {
            LogError("malloc failed");
        }
        else
        {
            result->diagnosticCreationTimeUtc = NULL;
            result->diagnosticId = NULL;
            if (source->diagnosticCreationTimeUtc != NULL && mallocAndStrcpy_s(&result->diagnosticCreationTimeUtc, source->diagnosticCreationTimeUtc) != 0)
            {
                LogError("mallocAndStrcpy_s for diagnosticCreationTimeUtc failed");
                free(result);
                result = NULL;
            }
            else if (source->diagnosticId != NULL && mallocAndStrcpy_s(&result->diagnosticId, source->diagnosticId) != 0)
            {
                LogError("mallocAndStrcpy_s for diagnosticId failed");
                free(result->diagnosticCreationTimeUtc);
                free(result);
                result = NULL;
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromByteArray(const unsigned char* byteArray, size_t size)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    if ((byteArray == NULL) && (size != 0))
    {
        LogError("Invalid argument - byteArray is NULL");
        result = NULL;
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
            /*let it go through*/
        }
        else
        {
            const unsigned char* source;
            unsigned char temp = 0x00;

            memset(result, 0, sizeof(*result));
            result->contentType = IOTHUBMESSAGE_BYTEARRAY;

            if (size != 0)
            {
                if (byteArray == NULL)
                {
                    LogError("Attempted to create a Hub Message from a NULL pointer!");
                    DestroyMessageData(result);
                    result = NULL;
                    source = NULL;
                }
                else
                {
                    source = byteArray;
                }
            }
            else
            {
                source = &temp;
            }
            if (result != NULL)
            {
                if ((result->value.byteArray = BUFFER_create(source, size)) == NULL)
                {
                    LogError("BUFFER_create failed");
                    DestroyMessageData(result);
                    result = NULL;
                }
                else if ((result->properties = Map_Create(ValidateAsciiCharactersFilter)) == NULL)
                {
                    LogError("Map_Create for properties failed");
                    DestroyMessageData(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    if (source == NULL)
    {
        LogError("Invalid argument - source is NULL");
        result = NULL;
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc failed");
            /*let it go through*/
        }
        else
        {
            memset(result, 0, sizeof(*result));
            result->contentType = IOTHUBMESSAGE_STRING;

            if ((result->value.string = STRING_construct(source)) == NULL)
            {
                LogError("STRING_construct failed");
                DestroyMessageData(result);
                result = NULL;
            }
            else if ((result->properties = Map_Create(ValidateAsciiCharactersFilter)) == NULL)
            {
                LogError("Map_Create for properties failed");
                DestroyMessageData(result);
                result = NULL;
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_Clone(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    const IOTHUB_MESSAGE_HANDLE_DATA* source = (const IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;
    if (source == NULL)
    {
        result = NULL;
        LogError("iotHubMessageHandle parameter cannot be NULL for IoTHubMessage_Clone");
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        if (result == NULL)
        {
            /*do nothing and return as is*/
            LogError("unable to malloc");
        }
        else
        {
            memset(result, 0, sizeof(*result));
            result->contentType = source->contentType;
            result->is_security_message = source->is_security_message;

            if (source->messageId != NULL && mallocAndStrcpy_s(&result->messageId, source->messageId) != 0)
            {
                LogError("unable to Copy messageId");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->correlationId != NULL && mallocAndStrcpy_s(&result->correlationId, source->correlationId) != 0)
            {
                LogError("unable to Copy correlationId");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->userDefinedContentType != NULL && mallocAndStrcpy_s(&result->userDefinedContentType, source->userDefinedContentType) != 0)
            {
                LogError("unable to copy contentType");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->contentEncoding != NULL && mallocAndStrcpy_s(&result->contentEncoding, source->contentEncoding) != 0)
            {
                LogError("unable to copy contentEncoding");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->diagnosticData != NULL && (result->diagnosticData = CloneDiagnosticPropertyData(source->diagnosticData)) == NULL)
            {
                LogError("unable to copy CloneDiagnosticPropertyData");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->outputName != NULL && mallocAndStrcpy_s(&result->outputName, source->outputName) != 0)
            {
                LogError("unable to copy outputName");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->inputName != NULL && mallocAndStrcpy_s(&result->inputName, source->inputName) != 0)
            {
                LogError("unable to copy inputName");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->creationTimeUtc != NULL && mallocAndStrcpy_s(&result->creationTimeUtc, source->creationTimeUtc) != 0)
            {
                LogError("unable to copy creationTimeUtc");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->userId != NULL && mallocAndStrcpy_s(&result->userId, source->userId) != 0)
            {
                LogError("unable to copy userId");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->connectionModuleId != NULL && mallocAndStrcpy_s(&result->connectionModuleId, source->connectionModuleId) != 0)
            {
                LogError("unable to copy inputName");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->connectionDeviceId != NULL && mallocAndStrcpy_s(&result->connectionDeviceId, source->connectionDeviceId) != 0)
            {
                LogError("unable to copy inputName");
                DestroyMessageData(result);
                result = NULL;
            }
            else if (source->contentType == IOTHUBMESSAGE_BYTEARRAY)
            {
                if ((result->value.byteArray = BUFFER_clone(source->value.byteArray)) == NULL)
                {
                    LogError("unable to BUFFER_clone");
                    DestroyMessageData(result);
                    result = NULL;
                }
                else if ((result->properties = Map_Clone(source->properties)) == NULL)
                {
                    LogError("unable to Map_Clone");
                    DestroyMessageData(result);
                    result = NULL;
                }
            }
            else /*can only be STRING*/
            {
                if ((result->value.string = STRING_clone(source->value.string)) == NULL)
                {
                    LogError("failed to STRING_clone");
                    DestroyMessageData(result);
                    result = NULL;
                }
                else if ((result->properties = Map_Clone(source->properties)) == NULL)
                {
                    LogError("unable to Map_Clone");
                    DestroyMessageData(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_GetByteArray(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const unsigned char** buffer, size_t* size)
{
    IOTHUB_MESSAGE_RESULT result;
    if (
        (iotHubMessageHandle == NULL) ||
        (buffer == NULL) ||
        (size == NULL)
        )
    {
        LogError("invalid parameter (NULL) to IoTHubMessage_GetByteArray IOTHUB_MESSAGE_HANDLE iotHubMessageHandle=%p, const unsigned char** buffer=%p, size_t* size=%p", iotHubMessageHandle, buffer, size);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->contentType != IOTHUBMESSAGE_BYTEARRAY)
        {
            result = IOTHUB_MESSAGE_INVALID_ARG;
            LogError("invalid type of message %s", MU_ENUM_TO_STRING(IOTHUBMESSAGE_CONTENT_TYPE, handleData->contentType));
        }
        else
        {
            *buffer = BUFFER_u_char(handleData->value.byteArray);
            *size = BUFFER_length(handleData->value.byteArray);
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetString(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->contentType != IOTHUBMESSAGE_STRING)
        {
            result = NULL;
        }
        else
        {
            result = STRING_c_str(handleData->value.string);
        }
    }
    return result;
}

IOTHUBMESSAGE_CONTENT_TYPE IoTHubMessage_GetContentType(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUBMESSAGE_CONTENT_TYPE result;
    if (iotHubMessageHandle == NULL)
    {
        result = IOTHUBMESSAGE_UNKNOWN;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->contentType;
    }
    return result;
}

MAP_HANDLE IoTHubMessage_Properties(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    MAP_HANDLE result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_Properties");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;
        result = handleData->properties;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetProperty(IOTHUB_MESSAGE_HANDLE msg_handle, const char* key, const char* value)
{
    IOTHUB_MESSAGE_RESULT result;
    if (msg_handle == NULL || key == NULL || value == NULL)
    {
        LogError("invalid parameter (NULL) to IoTHubMessage_SetProperty iotHubMessageHandle=%p, key=%p, value=%p", msg_handle, key, value);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        MAP_RESULT map_result = Map_AddOrUpdate(msg_handle->properties, key, value);
        if (map_result == MAP_FILTER_REJECT)
        {
            LogError("Failure validating property as ASCII");
            result = IOTHUB_MESSAGE_INVALID_TYPE;
        }
        else if (map_result != MAP_OK)
        {
            LogError("Failure adding property to internal map");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetProperty(IOTHUB_MESSAGE_HANDLE msg_handle, const char* key)
{
    const char* result;
    if (msg_handle == NULL || key == NULL)
    {
        LogError("invalid parameter (NULL) to IoTHubMessage_GetProperty iotHubMessageHandle=%p, key=%p", msg_handle, key);
        result = NULL;
    }
    else
    {
        bool key_exists = false;
        // The return value is not necessary, just check the key_exist variable
        if ((Map_ContainsKey(msg_handle->properties, key, &key_exists) == MAP_OK) && key_exists)
        {
            result = Map_GetValueFromKey(msg_handle->properties, key);
        }
        else
        {
            result = NULL;
        }
    }
    return result;
}

const char* IoTHubMessage_GetCorrelationId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_GetCorrelationId");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->correlationId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* correlationId)
{
    IOTHUB_MESSAGE_RESULT result;
    if (iotHubMessageHandle == NULL || correlationId == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_SetCorrelationId");
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->correlationId != NULL)
        {
            free(handleData->correlationId);
            handleData->correlationId = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->correlationId, correlationId) != 0)
        {
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* messageId)
{
    IOTHUB_MESSAGE_RESULT result;
    if (iotHubMessageHandle == NULL || messageId == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_SetMessageId");
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->messageId != NULL)
        {
            free(handleData->messageId);
            handleData->messageId = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->messageId, messageId) != 0)
        {
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetMessageId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_GetMessageId");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->messageId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* contentType)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || contentType == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, contentType=%p)", iotHubMessageHandle, contentType);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        if (handleData->userDefinedContentType != NULL)
        {
            free(handleData->userDefinedContentType);
            handleData->userDefinedContentType = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->userDefinedContentType, contentType) != 0)
        {
            LogError("Failed saving a copy of contentType");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }

    return result;
}

const char* IoTHubMessage_GetContentTypeSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;

        result = (const char*)handleData->userDefinedContentType;
    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* contentEncoding)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || contentEncoding == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, contentEncoding=%p)", iotHubMessageHandle, contentEncoding);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        if (set_content_encoding(iotHubMessageHandle, contentEncoding) != 0)
        {
            LogError("Failed saving a copy of contentEncoding");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetContentEncodingSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;

        result = (const char*)handleData->contentEncoding;
    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageCreationTimeUtcSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* creationTimeUtc)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || creationTimeUtc == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, creationTimeUtc=%p)", iotHubMessageHandle, creationTimeUtc);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        if (set_message_creation_time(iotHubMessageHandle, creationTimeUtc) != 0)
        {
            LogError("Failed saving a copy of creationTimeUtc");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = (const char*)handleData->creationTimeUtc;
    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageUserIdSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* userId)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || userId == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, userId=%p)", iotHubMessageHandle, userId);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        if (set_message_user_id(iotHubMessageHandle, userId) != 0)
        {
            LogError("Failed saving a copy of userId");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetMessageUserIdSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = (const char*)handleData->userId;
    }

    return result;
}

const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* IoTHubMessage_GetDiagnosticPropertyData(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        result = iotHubMessageHandle->diagnosticData;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetDiagnosticPropertyData(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* diagnosticData)
{
    IOTHUB_MESSAGE_RESULT result;
    if (iotHubMessageHandle == NULL ||
        diagnosticData == NULL ||
        diagnosticData->diagnosticCreationTimeUtc == NULL ||
        diagnosticData->diagnosticId == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, diagnosticData=%p, diagnosticData->diagnosticId=%p, diagnosticData->diagnosticCreationTimeUtc=%p)",
            iotHubMessageHandle, diagnosticData,
            diagnosticData == NULL ? NULL : diagnosticData->diagnosticId,
            diagnosticData == NULL ? NULL : diagnosticData->diagnosticCreationTimeUtc);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        if (iotHubMessageHandle->diagnosticData != NULL)
        {
            DestroyDiagnosticPropertyData(iotHubMessageHandle->diagnosticData);
            iotHubMessageHandle->diagnosticData = NULL;
        }

        if ((iotHubMessageHandle->diagnosticData = CloneDiagnosticPropertyData(diagnosticData)) == NULL)
        {
            LogError("Failed saving a copy of diagnosticData");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}


const char* IoTHubMessage_GetOutputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        result = iotHubMessageHandle->outputName;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetOutputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* outputName)
{
    IOTHUB_MESSAGE_RESULT result;

    if ((iotHubMessageHandle == NULL) || (outputName == NULL))
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, outputName=%p)", iotHubMessageHandle, outputName);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        if (handleData->outputName != NULL)
        {
            free(handleData->outputName);
            handleData->outputName = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->outputName, outputName) != 0)
        {
            LogError("Failed saving a copy of outputName");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }

    }

    return result;
}

const char* IoTHubMessage_GetInputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        result = iotHubMessageHandle->inputName;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetInputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* inputName)
{
    IOTHUB_MESSAGE_RESULT result;

    if ((iotHubMessageHandle == NULL) || (inputName == NULL))
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, inputName=%p)", iotHubMessageHandle, inputName);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        if (handleData->inputName != NULL)
        {
            free(handleData->inputName);
            handleData->inputName = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->inputName, inputName) != 0)
        {
            LogError("Failed saving a copy of inputName");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }

    }

    return result;
}


const char* IoTHubMessage_GetConnectionModuleId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        result = iotHubMessageHandle->connectionModuleId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetConnectionModuleId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* connectionModuleId)
{
    IOTHUB_MESSAGE_RESULT result;

    if ((iotHubMessageHandle == NULL) || (connectionModuleId == NULL))
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, connectionModuleId=%p)", iotHubMessageHandle, connectionModuleId);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        if (handleData->connectionModuleId != NULL)
        {
            free(handleData->connectionModuleId);
            handleData->connectionModuleId = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->connectionModuleId, connectionModuleId) != 0)
        {
            LogError("Failed saving a copy of connectionModuleId");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }

    }

    return result;
}


const char* IoTHubMessage_GetConnectionDeviceId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        result = iotHubMessageHandle->connectionDeviceId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetConnectionDeviceId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* connectionDeviceId)
{
    IOTHUB_MESSAGE_RESULT result;

    if ((iotHubMessageHandle == NULL) || (connectionDeviceId == NULL))
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, connectionDeviceId=%p)", iotHubMessageHandle, connectionDeviceId);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        if (handleData->connectionDeviceId != NULL)
        {
            free(handleData->connectionDeviceId);
            handleData->connectionDeviceId = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->connectionDeviceId, connectionDeviceId) != 0)
        {
            LogError("Failed saving a copy of connectionDeviceId");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }

    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetAsSecurityMessage(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUB_MESSAGE_RESULT result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        if (set_content_encoding(iotHubMessageHandle, SECURITY_CLIENT_JSON_ENCODING) != 0)
        {
            LogError("Failure setting security message content encoding");
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            iotHubMessageHandle->is_security_message = true;
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

bool IoTHubMessage_IsSecurityMessage(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    bool result;
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = false;
    }
    else
    {
        result = iotHubMessageHandle->is_security_message;
    }
    return result;
}

void IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    if (iotHubMessageHandle != NULL)
    {
        DestroyMessageData((IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle);
    }
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetDispositionContext(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, MESSAGE_DISPOSITION_CONTEXT_HANDLE dispositionContext, MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION dispositionContextDestroyFunction)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || dispositionContext == NULL || dispositionContextDestroyFunction == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, dispositionContext=%p, dispositionContextDestroyFunction=%p)",
            iotHubMessageHandle, dispositionContext, dispositionContextDestroyFunction);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        iotHubMessageHandle->dispositionContext = dispositionContext;
        iotHubMessageHandle->dispositionContextDestroyFunction = dispositionContextDestroyFunction;
        result = IOTHUB_MESSAGE_OK;
    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_GetDispositionContext(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, MESSAGE_DISPOSITION_CONTEXT_HANDLE* dispositionContext)
{
    IOTHUB_MESSAGE_RESULT result;

    if (iotHubMessageHandle == NULL || dispositionContext == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, dispositionContext=%p)",
            iotHubMessageHandle, dispositionContext);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        *dispositionContext = iotHubMessageHandle->dispositionContext;
        result = IOTHUB_MESSAGE_OK;
    }

    return result;
}
