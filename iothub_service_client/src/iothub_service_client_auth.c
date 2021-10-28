// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/connection_string_parser.h"

#include "iothub_service_client_auth.h"

static const char* IOTHUBHOSTNAME = "HostName";
static const char* IOTHUBSHAREDACESSKEYNAME = "SharedAccessKeyName";
static const char* IOTHUBSHAREDACESSKEY = "SharedAccessKey";
static const char* IOTHUBSHAREDACESSSIGNATURE = "SharedAccessSignature";
static const char* IOTHUBDEVICEID = "DeviceId";
static const char* IOTHUBSASPREFIX = "sas=";

static void free_service_client_auth(IOTHUB_SERVICE_CLIENT_AUTH* authInfo)
{
    free(authInfo->hostname);
    free(authInfo->iothubName);
    free(authInfo->iothubSuffix);
    free(authInfo->sharedAccessKey);
    free(authInfo->keyName);
    free(authInfo->deviceId);
    free(authInfo);
}

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_DEVICE_STATUS, IOTHUB_DEVICE_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_DEVICE_CONNECTION_STATE, IOTHUB_DEVICE_CONNECTION_STATE_VALUES);

static IOTHUB_SERVICE_CLIENT_AUTH_HANDLE create_from_connection_string(const char* connectionString, bool useSharedAccessSignature)
{
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE result;

    if (connectionString == NULL)
    {
        LogError("Input parameter is NULL: connectionString");
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(IOTHUB_SERVICE_CLIENT_AUTH));
        if (result == NULL)
        {
            LogError("Malloc failed for IOTHUB_SERVICE_CLIENT_AUTH");
        }
        else
        {
            memset(result, 0, sizeof(*result));

            STRING_HANDLE connection_string;
            if ((connection_string = STRING_construct(connectionString)) == NULL)
            {
                LogError("STRING_construct failed");
                free_service_client_auth(result);
                result = NULL;
            }
            else
            {
                MAP_HANDLE connection_string_values_map;
                if ((connection_string_values_map = connectionstringparser_parse(connection_string)) == NULL)
                {
                    LogError("Tokenizing failed on connectionString");
                    free_service_client_auth(result);
                    result = NULL;
                }
                else
                {
                    STRING_TOKENIZER_HANDLE tokenizer = NULL;
                    STRING_HANDLE token_key_string = NULL;
                    STRING_HANDLE token_value_string = NULL;
                    STRING_HANDLE host_name_string = NULL;
                    STRING_HANDLE shared_access = NULL;
                    const char* hostName;
                    const char* keyName;
                    const char* deviceId;
                    const char* sharedAccess = NULL;
                    const char* iothubName;
                    const char* iothubSuffix;

                    keyName = Map_GetValueFromKey(connection_string_values_map, IOTHUBSHAREDACESSKEYNAME);
                    deviceId = Map_GetValueFromKey(connection_string_values_map, IOTHUBDEVICEID);

                    (void)memset(result, 0, sizeof(IOTHUB_SERVICE_CLIENT_AUTH));
                    if ((keyName == NULL) && (deviceId == NULL))
                    {
                        LogError("Couldn't find %s or %s in connection string", IOTHUBSHAREDACESSKEYNAME, IOTHUBDEVICEID);
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((keyName != NULL) && (deviceId != NULL))
                    {
                        LogError("Both %s and %s in connection string were set, they are mutually exclusive", IOTHUBSHAREDACESSKEYNAME, IOTHUBDEVICEID);
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((hostName = Map_GetValueFromKey(connection_string_values_map, IOTHUBHOSTNAME)) == NULL)
                    {
                        LogError("Couldn't find %s in connection string", IOTHUBHOSTNAME);
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (!useSharedAccessSignature && (sharedAccess = Map_GetValueFromKey(connection_string_values_map, IOTHUBSHAREDACESSKEY)) == NULL)
                    {
                        LogError("Couldn't find %s in connection string", IOTHUBSHAREDACESSKEY);
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (useSharedAccessSignature && (sharedAccess = Map_GetValueFromKey(connection_string_values_map, IOTHUBSHAREDACESSSIGNATURE)) == NULL)
                    {
                        LogError("Couldn't find %s in connection string", IOTHUBSHAREDACESSSIGNATURE);
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((host_name_string = STRING_construct(hostName)) == NULL)
                    {
                        LogError("STRING_construct failed");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((tokenizer = STRING_TOKENIZER_create(host_name_string)) == NULL)
                    {
                        LogError("Error creating STRING tokenizer");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((token_key_string = STRING_new()) == NULL)
                    {
                        LogError("Error creating key token STRING_HANDLE");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((token_value_string = STRING_new()) == NULL)
                    {
                        LogError("Error creating value token STRING_HANDLE");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (STRING_TOKENIZER_get_next_token(tokenizer, token_key_string, ".") != 0)
                    {
                        LogError("Error reading key token STRING");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (STRING_TOKENIZER_get_next_token(tokenizer, token_value_string, "0") != 0)
                    {
                        LogError("Error reading value token STRING");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (mallocAndStrcpy_s(&result->hostname, hostName) != 0)
                    {
                        LogError("mallocAndStrcpy_s failed for hostName");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((keyName != NULL) && (mallocAndStrcpy_s(&result->keyName, keyName) != 0))
                    {
                        LogError("mallocAndStrcpy_s failed for keyName");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((deviceId != NULL) && (mallocAndStrcpy_s(&result->deviceId, deviceId) != 0))
                    {
                        LogError("mallocAndStrcpy_s failed for keyName");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (!useSharedAccessSignature && (sharedAccess != NULL) && mallocAndStrcpy_s(&result->sharedAccessKey, sharedAccess) != 0)
                    {
                        LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (useSharedAccessSignature &&
                        (sharedAccess != NULL) &&
                        (((shared_access = STRING_construct(IOTHUBSASPREFIX)) == NULL) ||
                        (STRING_concat(shared_access, sharedAccess) != 0) ||
                        (mallocAndStrcpy_s(&result->sharedAccessKey, STRING_c_str(shared_access)) != 0)))
                    {
                        LogError("mallocAndStrcpy_s failed for sharedAccessSignature");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((iothubName = STRING_c_str(token_key_string)) == NULL)
                    {
                        LogError("STRING_c_str failed for iothubName");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if ((iothubSuffix = STRING_c_str(token_value_string)) == NULL)
                    {
                        LogError("STRING_c_str failed for iothubSuffix");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (mallocAndStrcpy_s(&result->iothubName, iothubName) != 0)
                    {
                        LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    else if (mallocAndStrcpy_s(&result->iothubSuffix, iothubSuffix) != 0)
                    {
                        LogError("mallocAndStrcpy_s failed for sharedAccessKey");
                        free_service_client_auth(result);
                        result = NULL;
                    }
                    STRING_delete(token_key_string);
                    STRING_delete(token_value_string);
                    STRING_delete(host_name_string);
                    STRING_TOKENIZER_destroy(tokenizer);
                    Map_Destroy(connection_string_values_map);
                    if (useSharedAccessSignature)
                    {
                        STRING_delete(shared_access);
                    }
                }
                STRING_delete(connection_string);
            }
        }
    }
    return result;
}

IOTHUB_SERVICE_CLIENT_AUTH_HANDLE IoTHubServiceClientAuth_CreateFromConnectionString(const char* connectionString)
{
    return create_from_connection_string(connectionString, false);
}

IOTHUB_SERVICE_CLIENT_AUTH_HANDLE IoTHubServiceClientAuth_CreateFromSharedAccessSignature(const char* connectionString)
{
    return create_from_connection_string(connectionString, true);
}

void IoTHubServiceClientAuth_Destroy(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    if (serviceClientHandle != NULL)
    {
        free_service_client_auth((IOTHUB_SERVICE_CLIENT_AUTH*)serviceClientHandle);
    }
}
