// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"

#include <time.h>
#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include "iothub_transport_ll.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "internal/iothub_message_private.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport.h"
#include "internal/iothub_transport_ll_private.h"
#include "internal/iothub_internal_consts.h"

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/agenttime.h"

#define IOTHUB_APP_PREFIX "iothub-app-"
static const char* IOTHUB_MESSAGE_ID = "iothub-messageid";
static const char* IOTHUB_CORRELATION_ID = "iothub-correlationid";
static const char* IOTHUB_CONTENT_TYPE_D2C = "iothub-contenttype";
static const char* IOTHUB_CONTENT_ENCODING_D2C = "iothub-contentencoding";
static const char* IOTHUB_CONTENT_TYPE_C2D = "ContentType";
static const char* IOTHUB_CONTENT_ENCODING_C2D = "ContentEncoding";

static const char* IOTHUB_AUTH_HEADER_VALUE = "Authorization";

#define CONTENT_TYPE "Content-Type"
#define APPLICATION_OCTET_STREAM "application/octet-stream"
#define APPLICATION_VND_MICROSOFT_IOTHUB_JSON "application/vnd.microsoft.iothub.json"
#define API_VERSION "?api-version=2016-11-14"
#define EVENT_ENDPOINT "/messages/events"
#define MESSAGE_ENDPOINT_HTTP "/messages/devicebound"
#define MESSAGE_ENDPOINT_HTTP_ETAG "/messages/devicebound/"

/*DEFAULT_GETMINIMUMPOLLINGTIME is the minimum time in seconds allowed between 2 consecutive GET issues to the service (GET=fetch messages)*/
/*the default is 25 minutes*/
#define DEFAULT_GETMINIMUMPOLLINGTIME ((unsigned int)25*60)

#define MAXIMUM_MESSAGE_SIZE (255*1024-1)
#define MAXIMUM_PAYLOAD_OVERHEAD 384
#define MAXIMUM_PROPERTY_OVERHEAD 16

/*forward declaration*/
static int appendMapToJSON(STRING_HANDLE existing, const char* const* keys, const char* const* values, size_t count);

typedef struct HTTPTRANSPORT_HANDLE_DATA_TAG
{
    STRING_HANDLE hostName;
    HTTPAPIEX_HANDLE httpApiExHandle;
    bool doBatchedTransfers;
    unsigned int getMinimumPollingTime;
    VECTOR_HANDLE perDeviceList;

    TRANSPORT_CALLBACKS_INFO transport_callbacks;
    void* transport_ctx;

}HTTPTRANSPORT_HANDLE_DATA;

typedef struct HTTPTRANSPORT_PERDEVICE_DATA_TAG
{
    HTTPTRANSPORT_HANDLE_DATA* transportHandle;

    STRING_HANDLE deviceId;
    STRING_HANDLE deviceKey;
    STRING_HANDLE deviceSasToken;
    STRING_HANDLE eventHTTPrelativePath;
    STRING_HANDLE messageHTTPrelativePath;
    HTTP_HEADERS_HANDLE eventHTTPrequestHeaders;
    HTTP_HEADERS_HANDLE messageHTTPrequestHeaders;
    STRING_HANDLE abandonHTTPrelativePathBegin;
    HTTPAPIEX_SAS_HANDLE sasObject;
    bool DoWork_PullMessage;
    time_t lastPollTime;
    bool isFirstPoll;

    void* device_transport_ctx;
    PDLIST_ENTRY waitingToSend;
    DLIST_ENTRY eventConfirmations; /*holds items for event confirmations*/
} HTTPTRANSPORT_PERDEVICE_DATA;

typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG
{
    char* etagValue;
} MESSAGE_DISPOSITION_CONTEXT;

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

static void destroy_eventHTTPrelativePath(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->eventHTTPrelativePath);
    handleData->eventHTTPrelativePath = NULL;
}

static bool create_eventHTTPrelativePath(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceId)
{
    bool result;
    STRING_HANDLE urlEncodedDeviceId;
    handleData->eventHTTPrelativePath = STRING_construct("/devices/");
    if (handleData->eventHTTPrelativePath == NULL)
    {
        LogError("STRING_construct failed.");
        result = false;
    }
    else
    {
        if (!(
            ((urlEncodedDeviceId = URL_EncodeString(deviceId)) != NULL) &&
            (STRING_concat_with_STRING(handleData->eventHTTPrelativePath, urlEncodedDeviceId) == 0) &&
            (STRING_concat(handleData->eventHTTPrelativePath, EVENT_ENDPOINT API_VERSION) == 0)
            ))
        {
            destroy_eventHTTPrelativePath(handleData);
            LogError("Creating HTTP event relative path failed.");
            result = false;
        }
        else
        {
            result = true;
        }
        STRING_delete(urlEncodedDeviceId);
    }
    return result;
}

static int retrieve_message_properties(HTTP_HEADERS_HANDLE resp_header, IOTHUB_MESSAGE_HANDLE recv_msg)
{
    int result = 0;
    size_t nHeaders;
    if (HTTPHeaders_GetHeaderCount(resp_header, &nHeaders) != HTTP_HEADERS_OK)
    {
        LogError("unable to get the count of HTTP headers");
        result = MU_FAILURE;
    }
    else
    {
        MAP_HANDLE properties = (nHeaders > 0) ? IoTHubMessage_Properties(recv_msg) : NULL;
        for (size_t index = 0; index < nHeaders; index++)
        {
            char* completeHeader;
            if (HTTPHeaders_GetHeader(resp_header, index, &completeHeader) != HTTP_HEADERS_OK)
            {
                LogError("Failed getting header %lu", (unsigned long)index);
                result = MU_FAILURE;
                break;
            }
            else
            {
                char* colon_pos;
                if (strncmp(IOTHUB_APP_PREFIX, completeHeader, strlen(IOTHUB_APP_PREFIX)) == 0)
                {
                    /*looks like a property headers*/
                    /*there's a guaranteed ':' in the completeHeader, by HTTP_HEADERS module*/
                    colon_pos = strchr(completeHeader, ':');
                    if (colon_pos != NULL)
                    {
                        *colon_pos = '\0'; /*cut it down*/
                        if (Map_AddOrUpdate(properties, completeHeader + strlen(IOTHUB_APP_PREFIX), colon_pos + 2) != MAP_OK) /*whereIsColon+1 is a space because HTTPEHADERS outputs a ": " between name and value*/
                        {
                            free(completeHeader);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }
                else if (strncmp(IOTHUB_MESSAGE_ID, completeHeader, strlen(IOTHUB_MESSAGE_ID)) == 0)
                {
                    char* whereIsColon = strchr(completeHeader, ':');
                    if (whereIsColon != NULL)
                    {
                        *whereIsColon = '\0'; /*cut it down*/
                        if (IoTHubMessage_SetMessageId(recv_msg, whereIsColon + 2) != IOTHUB_MESSAGE_OK)
                        {
                            free(completeHeader);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }
                else if (strncmp(IOTHUB_CORRELATION_ID, completeHeader, strlen(IOTHUB_CORRELATION_ID)) == 0)
                {
                    char* whereIsColon = strchr(completeHeader, ':');
                    if (whereIsColon != NULL)
                    {
                        *whereIsColon = '\0'; /*cut it down*/
                        if (IoTHubMessage_SetCorrelationId(recv_msg, whereIsColon + 2) != IOTHUB_MESSAGE_OK)
                        {
                            free(completeHeader);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }
                else if (strncmp(IOTHUB_CONTENT_TYPE_C2D, completeHeader, strlen(IOTHUB_CONTENT_TYPE_C2D)) == 0)
                {
                    char* whereIsColon = strchr(completeHeader, ':');
                    if (whereIsColon != NULL)
                    {
                        *whereIsColon = '\0'; /*cut it down*/
                        if (IoTHubMessage_SetContentTypeSystemProperty(recv_msg, whereIsColon + 2) != IOTHUB_MESSAGE_OK)
                        {
                            LogError("Failed setting IoTHubMessage content-type");
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }
                else if (strncmp(IOTHUB_CONTENT_ENCODING_C2D, completeHeader, strlen(IOTHUB_CONTENT_ENCODING_C2D)) == 0)
                {
                    char* whereIsColon = strchr(completeHeader, ':');
                    if (whereIsColon != NULL)
                    {
                        *whereIsColon = '\0'; /*cut it down*/
                        if (IoTHubMessage_SetContentEncodingSystemProperty(recv_msg, whereIsColon + 2) != IOTHUB_MESSAGE_OK)
                        {
                            LogError("Failed setting IoTHubMessage content-encoding");
                            break;
                        }
                    }
                }
                free(completeHeader);
            }
        }
    }
    return result;
}

static int set_system_properties(IOTHUB_MESSAGE_LIST* message, HTTP_HEADERS_HANDLE headers)
{
    int result;
    const char* msgId;
    const char* corrId;
    const char* content_type;
    const char* contentEncoding;

    // Add the Message Id and the Correlation Id
    msgId = IoTHubMessage_GetMessageId(message->messageHandle);
    if (msgId != NULL && HTTPHeaders_ReplaceHeaderNameValuePair(headers, IOTHUB_MESSAGE_ID, msgId) != HTTP_HEADERS_OK)
    {
        LogError("unable to HTTPHeaders_ReplaceHeaderNameValuePair");
        result = MU_FAILURE;
    }
    else if ((corrId = IoTHubMessage_GetCorrelationId(message->messageHandle)) != NULL && HTTPHeaders_ReplaceHeaderNameValuePair(headers, IOTHUB_CORRELATION_ID, corrId) != HTTP_HEADERS_OK)
    {
        LogError("unable to HTTPHeaders_ReplaceHeaderNameValuePair");
        result = MU_FAILURE;
    }
    else if ((content_type = IoTHubMessage_GetContentTypeSystemProperty(message->messageHandle)) != NULL && HTTPHeaders_ReplaceHeaderNameValuePair(headers, IOTHUB_CONTENT_TYPE_D2C, content_type) != HTTP_HEADERS_OK)
    {
        LogError("unable to HTTPHeaders_ReplaceHeaderNameValuePair (content-type)");
        result = MU_FAILURE;
    }
    else if ((contentEncoding = IoTHubMessage_GetContentEncodingSystemProperty(message->messageHandle)) != NULL && HTTPHeaders_ReplaceHeaderNameValuePair(headers, IOTHUB_CONTENT_ENCODING_D2C, contentEncoding) != HTTP_HEADERS_OK)
    {
        LogError("unable to HTTPHeaders_ReplaceHeaderNameValuePair (content-encoding)");
        result = MU_FAILURE;
    }
    else if (IoTHubMessage_IsSecurityMessage(message->messageHandle) && HTTPHeaders_ReplaceHeaderNameValuePair(headers, SECURITY_INTERFACE_ID, SECURITY_INTERFACE_ID_VALUE) != HTTP_HEADERS_OK)
    {
        LogError("unable to set security message header info");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

static bool set_message_properties(IOTHUB_MESSAGE_LIST* message, size_t* msg_size, HTTP_HEADERS_HANDLE headers, HTTPTRANSPORT_HANDLE_DATA* handleData, HTTPTRANSPORT_PERDEVICE_DATA* deviceData)
{
    bool result = true;

    MAP_HANDLE map;
    const char*const* keys;
    const char*const* values;
    size_t count;
    if (message == NULL)
    {
        LogError("IOTHUB_MESSAGE_LIST is invalid");
        result = false;
    }
    else if ((map = IoTHubMessage_Properties(message->messageHandle)) == NULL)
    {
        LogError("MAP_HANDLE is invalid");
        result = false;
    }
    else if (Map_GetInternals(map, &keys, &values, &count) != MAP_OK)
    {
        LogError("unable to Map_GetInternals");
        result = false;
    }
    else
    {
        for (size_t index = 0; index < count; index++)
        {
            *msg_size += (strlen(values[index]) + strlen(keys[index]) + MAXIMUM_PROPERTY_OVERHEAD);
            if (*msg_size > MAXIMUM_MESSAGE_SIZE)
            {
                PDLIST_ENTRY head = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                DList_InsertTailList(&(deviceData->eventConfirmations), head);
                handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_ERROR, deviceData->device_transport_ctx); // takes care of emptying the list too
                result = false;
                break;
            }
            else
            {
                STRING_HANDLE app_construct = STRING_construct_sprintf("%s%s", IOTHUB_APP_PREFIX, keys[index]);
                if (app_construct == NULL)
                {
                    LogError("Unable to construct app header");
                    result = false;
                    break;
                }
                else
                {
                    if (HTTPHeaders_ReplaceHeaderNameValuePair(headers, STRING_c_str(app_construct), values[index]) != HTTP_HEADERS_OK)
                    {
                        LogError("Unable to add app properties to http header");
                        result = false;
                        break;
                    }
                    STRING_delete(app_construct);
                }
            }
        }
    }
    return result;
}

static void destroy_messageHTTPrelativePath(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->messageHTTPrelativePath);
    handleData->messageHTTPrelativePath = NULL;
}

static bool create_messageHTTPrelativePath(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceId)
{
    bool result;
    handleData->messageHTTPrelativePath = STRING_construct("/devices/");
    if (handleData->messageHTTPrelativePath == NULL)
    {
        LogError("STRING_construct failed.");
        result = false;
    }
    else
    {
        STRING_HANDLE urlEncodedDeviceId;
        if (!(
            ((urlEncodedDeviceId = URL_EncodeString(deviceId)) != NULL) &&
            (STRING_concat_with_STRING(handleData->messageHTTPrelativePath, urlEncodedDeviceId) == 0) &&
            (STRING_concat(handleData->messageHTTPrelativePath, MESSAGE_ENDPOINT_HTTP API_VERSION) == 0)
            ))
        {
            LogError("Creating HTTP message relative path failed.");
            result = false;
            destroy_messageHTTPrelativePath(handleData);
        }
        else
        {
            result = true;
        }
        STRING_delete(urlEncodedDeviceId);
    }

    return result;
}

static void destroy_eventHTTPrequestHeaders(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    HTTPHeaders_Free(handleData->eventHTTPrequestHeaders);
    handleData->eventHTTPrequestHeaders = NULL;
}

static HTTP_HEADERS_RESULT addUserAgentHeaderInfo(HTTPTRANSPORT_HANDLE_DATA* transport_data, HTTP_HEADERS_HANDLE eventHTTPrequestHeaders)
{
    HTTP_HEADERS_RESULT result;
    const char* product_info = transport_data->transport_callbacks.prod_info_cb(transport_data->transport_ctx);
    if (product_info == NULL)
    {
        result = HTTPHeaders_AddHeaderNameValuePair(eventHTTPrequestHeaders, "User-Agent", CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION);
    }
    else
    {
        result = HTTPHeaders_AddHeaderNameValuePair(eventHTTPrequestHeaders, "User-Agent", product_info);
    }
    return result;
}

static bool create_eventHTTPrequestHeaders(HTTPTRANSPORT_PERDEVICE_DATA* handleData, HTTPTRANSPORT_HANDLE_DATA* transport_data, const char * deviceId, bool is_x509_used)
{
    bool result;
    handleData->eventHTTPrequestHeaders = HTTPHeaders_Alloc();
    if (handleData->eventHTTPrequestHeaders == NULL)
    {
        LogError("HTTPHeaders_Alloc failed.");
        result = false;
    }
    else
    {
        STRING_HANDLE temp = STRING_construct("/devices/");
        if (temp == NULL)
        {
            LogError("STRING_construct failed.");
            result = false;
            destroy_eventHTTPrequestHeaders(handleData);
        }
        else
        {
            STRING_HANDLE urlEncodedDeviceId;
            if (!(
                ((urlEncodedDeviceId = URL_EncodeString(deviceId)) != NULL) &&
                (STRING_concat_with_STRING(temp, urlEncodedDeviceId) == 0) &&
                (STRING_concat(temp, EVENT_ENDPOINT) == 0)
                ))
            {
                LogError("deviceId construction failed.");
                result = false;
                destroy_eventHTTPrequestHeaders(handleData);
            }
            else
            {
                if (!(
                    (HTTPHeaders_AddHeaderNameValuePair(handleData->eventHTTPrequestHeaders, "iothub-to", STRING_c_str(temp)) == HTTP_HEADERS_OK) &&
                    (is_x509_used || (HTTPHeaders_AddHeaderNameValuePair(handleData->eventHTTPrequestHeaders, IOTHUB_AUTH_HEADER_VALUE, " ") == HTTP_HEADERS_OK)) &&
                    (HTTPHeaders_AddHeaderNameValuePair(handleData->eventHTTPrequestHeaders, "Accept", "application/json") == HTTP_HEADERS_OK) &&
                    (HTTPHeaders_AddHeaderNameValuePair(handleData->eventHTTPrequestHeaders, "Connection", "Keep-Alive") == HTTP_HEADERS_OK) &&
                    (addUserAgentHeaderInfo(transport_data, handleData->eventHTTPrequestHeaders) == HTTP_HEADERS_OK)
                    ))
                {
                    LogError("adding header properties failed.");
                    result = false;
                    destroy_eventHTTPrequestHeaders(handleData);
                }
                else
                {
                    result = true;
                }
            }
            STRING_delete(urlEncodedDeviceId);
            STRING_delete(temp);
        }
    }
    return result;
}

static void destroy_messageHTTPrequestHeaders(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    HTTPHeaders_Free(handleData->messageHTTPrequestHeaders);
    handleData->messageHTTPrequestHeaders = NULL;
}

static bool create_messageHTTPrequestHeaders(HTTPTRANSPORT_PERDEVICE_DATA* handleData, HTTPTRANSPORT_HANDLE_DATA* transport_data, bool is_x509_used)
{
    bool result;
    handleData->messageHTTPrequestHeaders = HTTPHeaders_Alloc();
    if (handleData->messageHTTPrequestHeaders == NULL)
    {
        LogError("HTTPHeaders_Alloc failed.");
        result = false;
    }
    else
    {
        if (!(
            (addUserAgentHeaderInfo(transport_data, handleData->messageHTTPrequestHeaders) == HTTP_HEADERS_OK) &&
            (is_x509_used || (HTTPHeaders_AddHeaderNameValuePair(handleData->messageHTTPrequestHeaders, IOTHUB_AUTH_HEADER_VALUE, " ") == HTTP_HEADERS_OK))
            ))
        {
            destroy_messageHTTPrequestHeaders(handleData);
            LogError("adding header properties failed.");
            result = false;
        }
        else
        {
            result = true;
        }
    }
    return result;
}

static void destroy_abandonHTTPrelativePathBegin(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->abandonHTTPrelativePathBegin);
    handleData->abandonHTTPrelativePathBegin = NULL;
}

static bool create_abandonHTTPrelativePathBegin(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceId)
{
    bool result;
    handleData->abandonHTTPrelativePathBegin = STRING_construct("/devices/");
    if (handleData->abandonHTTPrelativePathBegin == NULL)
    {
        result = false;
    }
    else
    {
        STRING_HANDLE urlEncodedDeviceId;
        if (!(
            ((urlEncodedDeviceId = URL_EncodeString(deviceId)) != NULL) &&
            (STRING_concat_with_STRING(handleData->abandonHTTPrelativePathBegin, urlEncodedDeviceId) == 0) &&
            (STRING_concat(handleData->abandonHTTPrelativePathBegin, MESSAGE_ENDPOINT_HTTP_ETAG) == 0)
            ))
        {
            LogError("unable to create abandon path string.");
            STRING_delete(handleData->abandonHTTPrelativePathBegin);
            result = false;
        }
        else
        {
            result = true;
        }
        STRING_delete(urlEncodedDeviceId);
    }
    return result;
}

static void destroy_SASObject(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    HTTPAPIEX_SAS_Destroy(handleData->sasObject);
    handleData->sasObject = NULL;
}

static bool create_deviceSASObject(HTTPTRANSPORT_PERDEVICE_DATA* handleData, STRING_HANDLE hostName, const char * deviceId, const char * deviceKey)
{
    STRING_HANDLE keyName;
    bool result;
    keyName = URL_EncodeString(deviceId);
    if (keyName == NULL)
    {
        LogError("URL_EncodeString keyname failed");
        result = false;
    }
    else
    {
        STRING_HANDLE uriResource;
        uriResource = STRING_clone(hostName);
        if (uriResource != NULL)
        {
            if ((STRING_concat(uriResource, "/devices/") == 0) &&
                (STRING_concat_with_STRING(uriResource, keyName) == 0))
            {
                STRING_HANDLE key = STRING_construct(deviceKey);
                if (key != NULL)
                {
                    if (STRING_empty(keyName) != 0)
                    {
                        LogError("Unable to form the device key name for the SAS");
                        result = false;
                    }
                    else
                    {
                        handleData->sasObject = HTTPAPIEX_SAS_Create(key, uriResource, keyName);
                        result = (handleData->sasObject != NULL) ? (true) : (false);
                    }
                    STRING_delete(key);
                }
                else
                {
                    LogError("STRING_construct Key failed");
                    result = false;
                }
            }
            else
            {
                LogError("STRING_concat uri resource failed");
                result = false;
            }
            STRING_delete(uriResource);
        }
        else
        {
            LogError("STRING_staticclone uri resource failed");
            result = false;
        }
        STRING_delete(keyName);
    }
    return result;
}

static void destroy_deviceId(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->deviceId);
    handleData->deviceId = NULL;
}

static bool create_deviceId(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceId)
{
    bool result;
    handleData->deviceId = STRING_construct(deviceId);
    if (handleData->deviceId == NULL)
    {
        LogError("STRING_construct deviceId failed");
        result = false;
    }
    else
    {
        result = true;
    }
    return result;
}

static void destroy_deviceKey(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->deviceKey);
    handleData->deviceKey = NULL;
}

static void destroy_deviceSas(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->deviceSasToken);
    handleData->deviceSasToken = NULL;
}

static bool create_deviceKey(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceKey)
{
    bool result;
    handleData->deviceKey = STRING_construct(deviceKey);
    if (handleData->deviceKey == NULL)
    {
        LogError("STRING_construct deviceKey failed");
        result = false;
    }
    else
    {
        result = true;
    }
    return result;
}

static void destroy_deviceSasToken(HTTPTRANSPORT_PERDEVICE_DATA* handleData)
{
    STRING_delete(handleData->deviceSasToken);
    handleData->deviceSasToken = NULL;
}

static bool create_deviceSasToken(HTTPTRANSPORT_PERDEVICE_DATA* handleData, const char * deviceSasToken)
{
    bool result;
    handleData->deviceSasToken = STRING_construct(deviceSasToken);
    if (handleData->deviceSasToken == NULL)
    {
        LogError("STRING_construct deviceSasToken failed");
        result = false;
    }
    else
    {
        result = true;
    }
    return result;
}

/*
* List queries  Find by handle and find by device name
*/

static bool findDeviceHandle(const void* element, const void* value)
{
    bool result;
    /* data stored at element is device handle */
    const IOTHUB_DEVICE_HANDLE * guess = (const IOTHUB_DEVICE_HANDLE *)element;
    IOTHUB_DEVICE_HANDLE match = (IOTHUB_DEVICE_HANDLE)value;
    result = (*guess == match) ? true : false;
    return result;
}

static bool findDeviceById(const void* element, const void* value)
{
    bool result;
    const char* deviceId = (const char *)value;
    const HTTPTRANSPORT_PERDEVICE_DATA * perDeviceElement = *(const HTTPTRANSPORT_PERDEVICE_DATA **)element;

    result = (strcmp(STRING_c_str(perDeviceElement->deviceId), deviceId) == 0);

    return result;
}

static IOTHUB_DEVICE_HANDLE IoTHubTransportHttp_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    HTTPTRANSPORT_PERDEVICE_DATA* result;
    if (handle == NULL || device == NULL)
    {
        LogError("Transport handle is NULL");
        result = NULL;
    }
    else if (device->deviceId == NULL || ((device->deviceKey != NULL) && (device->deviceSasToken != NULL)) || waitingToSend == NULL)
    {
        LogError("invalid parameters detected TRANSPORT_LL_HANDLE handle=%p, const IOTHUB_DEVICE_CONFIG* device=%p, PDLIST_ENTRY waitingToSend=%p",
            handle, device, waitingToSend);
        result = NULL;
    }
    else
    {
        HTTPTRANSPORT_HANDLE_DATA* handleData = (HTTPTRANSPORT_HANDLE_DATA*)handle;
        void* listItem = VECTOR_find_if(handleData->perDeviceList, findDeviceById, device->deviceId);
        if (listItem != NULL)
        {
            LogError("Transport already has device registered by id: [%s]", device->deviceId);
            result = NULL;
        }
        else
        {
            bool was_create_deviceKey_ok = false;
            bool was_create_deviceSasToken_ok = false;
            bool was_sasObject_ok = false;
            bool was_x509_ok = false; /*there's nothing "created" in the case of x509, it is a flag indicating that x509 is used*/

            bool was_resultCreated_ok = ((result = (HTTPTRANSPORT_PERDEVICE_DATA *)malloc(sizeof(HTTPTRANSPORT_PERDEVICE_DATA))) != NULL);
            bool was_create_deviceId_ok = was_resultCreated_ok && create_deviceId(result, device->deviceId);

            if (was_create_deviceId_ok)
            {
                if (device->deviceSasToken != NULL)
                {
                    /*device->deviceKey is certainly NULL (because of the validation condition (else if (device->deviceId == NULL || ((device->deviceKey != NULL) && (device->deviceSasToken != NULL)) || waitingToSend == NULL))that does not accept that both satoken AND devicekey are non-NULL*/
                    was_create_deviceSasToken_ok = create_deviceSasToken(result, device->deviceSasToken);
                    result->deviceKey = NULL;
                    result->sasObject = NULL;
                }
                else /*when deviceSasToken == NULL*/
                {
                    if (device->deviceKey != NULL)
                    {
                        was_create_deviceKey_ok = create_deviceKey(result, device->deviceKey);
                        result->deviceSasToken = NULL;
                    }
                    else
                    {
                        /*when both of them are NULL*/
                        was_x509_ok = true;
                        result->deviceKey = NULL;
                        result->deviceSasToken = NULL;
                    }
                }
            }

            bool was_eventHTTPrelativePath_ok = (was_create_deviceKey_ok || was_create_deviceSasToken_ok || was_x509_ok) && create_eventHTTPrelativePath(result, device->deviceId);
            bool was_messageHTTPrelativePath_ok = was_eventHTTPrelativePath_ok && create_messageHTTPrelativePath(result, device->deviceId);
            bool was_eventHTTPrequestHeaders_ok;
            if (was_messageHTTPrelativePath_ok)
            {
                result->device_transport_ctx = handleData->transport_ctx;
                was_eventHTTPrequestHeaders_ok = create_eventHTTPrequestHeaders(result, handleData, device->deviceId, was_x509_ok);
            }
            else
            {
                was_eventHTTPrequestHeaders_ok = false;
            }
            bool was_messageHTTPrequestHeaders_ok = was_eventHTTPrequestHeaders_ok && create_messageHTTPrequestHeaders(result, handleData, was_x509_ok);
            bool was_abandonHTTPrelativePathBegin_ok = was_messageHTTPrequestHeaders_ok && create_abandonHTTPrelativePathBegin(result, device->deviceId);

            if (was_x509_ok)
            {
                result->sasObject = NULL; /**/
                was_sasObject_ok = true;
            }
            else
            {
                if (!was_create_deviceSasToken_ok)
                {
                    was_sasObject_ok = was_abandonHTTPrelativePathBegin_ok && create_deviceSASObject(result, handleData->hostName, device->deviceId, device->deviceKey);
                }
            }

            bool was_list_add_ok = (was_sasObject_ok || was_create_deviceSasToken_ok || was_x509_ok) && (VECTOR_push_back(handleData->perDeviceList, &result, 1) == 0);

            if (was_list_add_ok)
            {
                result->DoWork_PullMessage = false;
                result->isFirstPoll = true;
                result->waitingToSend = waitingToSend;
                DList_InitializeListHead(&(result->eventConfirmations));
                result->transportHandle = (HTTPTRANSPORT_HANDLE_DATA *)handle;
            }
            else
            {
                if (was_sasObject_ok) destroy_SASObject(result);
                if (was_abandonHTTPrelativePathBegin_ok) destroy_abandonHTTPrelativePathBegin(result);
                if (was_messageHTTPrelativePath_ok) destroy_messageHTTPrelativePath(result);
                if (was_eventHTTPrequestHeaders_ok) destroy_eventHTTPrequestHeaders(result);
                if (was_messageHTTPrequestHeaders_ok) destroy_messageHTTPrequestHeaders(result);
                if (was_eventHTTPrelativePath_ok) destroy_eventHTTPrelativePath(result);
                if (was_create_deviceId_ok) destroy_deviceId(result);
                if (was_create_deviceKey_ok) destroy_deviceKey(result);
                if (was_create_deviceSasToken_ok) destroy_deviceSasToken(result);
                if (was_resultCreated_ok) free(result);
                result = NULL;
            }
        }

    }
    return (IOTHUB_DEVICE_HANDLE)result;
}


static void destroy_perDeviceData(HTTPTRANSPORT_PERDEVICE_DATA * perDeviceItem)
{
    destroy_deviceId(perDeviceItem);
    destroy_deviceKey(perDeviceItem);
    destroy_deviceSas(perDeviceItem);
    destroy_eventHTTPrelativePath(perDeviceItem);
    destroy_messageHTTPrelativePath(perDeviceItem);
    destroy_eventHTTPrequestHeaders(perDeviceItem);
    destroy_messageHTTPrequestHeaders(perDeviceItem);
    destroy_abandonHTTPrelativePathBegin(perDeviceItem);
    destroy_SASObject(perDeviceItem);
}

static IOTHUB_DEVICE_HANDLE* get_perDeviceDataItem(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    HTTPTRANSPORT_PERDEVICE_DATA* deviceHandleData = (HTTPTRANSPORT_PERDEVICE_DATA*)deviceHandle;
    IOTHUB_DEVICE_HANDLE* listItem;

    HTTPTRANSPORT_HANDLE_DATA* handleData = deviceHandleData->transportHandle;

    listItem = (IOTHUB_DEVICE_HANDLE *)VECTOR_find_if(handleData->perDeviceList, findDeviceHandle, deviceHandle);
    if (listItem == NULL)
    {
        LogError("device handle not found in transport device list");
        listItem = NULL;
    }
    else
    {
        /* sucessfully found device in list. */
    }

    return listItem;
}

static void IoTHubTransportHttp_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    if (deviceHandle == NULL)
    {
        LogError("Unregister a NULL device handle");
    }
    else
    {
        HTTPTRANSPORT_PERDEVICE_DATA* deviceHandleData = (HTTPTRANSPORT_PERDEVICE_DATA*)deviceHandle;
        HTTPTRANSPORT_HANDLE_DATA* handleData = deviceHandleData->transportHandle;
        IOTHUB_DEVICE_HANDLE* listItem = get_perDeviceDataItem(deviceHandle);
        if (listItem == NULL)
        {
            LogError("Device Handle [%p] not found in transport", deviceHandle);
        }
        else
        {
            HTTPTRANSPORT_PERDEVICE_DATA * perDeviceItem = (HTTPTRANSPORT_PERDEVICE_DATA *)(*listItem);

            destroy_perDeviceData(perDeviceItem);
            VECTOR_erase(handleData->perDeviceList, listItem, 1);
            free(deviceHandleData);
        }
    }

    return;
}

static void destroy_hostName(HTTPTRANSPORT_HANDLE_DATA* handleData)
{
    STRING_delete(handleData->hostName);
    handleData->hostName = NULL;
}

static bool create_hostName(HTTPTRANSPORT_HANDLE_DATA* handleData, const IOTHUBTRANSPORT_CONFIG* config)
{
    bool result;
    if (config->upperConfig->protocolGatewayHostName != NULL)
    {
        handleData->hostName = STRING_construct(config->upperConfig->protocolGatewayHostName);
        result = (handleData->hostName != NULL);
    }
    else
    {
        handleData->hostName = STRING_construct(config->upperConfig->iotHubName);

        if (handleData->hostName == NULL)
        {
            result = false;
        }
        else
        {
            if ((STRING_concat(handleData->hostName, ".") != 0) ||
                (STRING_concat(handleData->hostName, config->upperConfig->iotHubSuffix) != 0))
            {
                destroy_hostName(handleData);
                result = false;
            }
            else
            {
                result = true;
            }
        }
    }
    return result;
}

static void destroy_httpApiExHandle(HTTPTRANSPORT_HANDLE_DATA* handleData)
{
    HTTPAPIEX_Destroy(handleData->httpApiExHandle);
    handleData->httpApiExHandle = NULL;
}

static bool create_httpApiExHandle(HTTPTRANSPORT_HANDLE_DATA* handleData, const IOTHUBTRANSPORT_CONFIG* config)
{
    bool result;
    (void)config;
    handleData->httpApiExHandle = HTTPAPIEX_Create(STRING_c_str(handleData->hostName));
    if (handleData->httpApiExHandle == NULL)
    {
        result = false;
    }
    else
    {
        result = true;
    }
    return result;
}

static void destroy_perDeviceList(HTTPTRANSPORT_HANDLE_DATA* handleData)
{
    VECTOR_destroy(handleData->perDeviceList);
    handleData->perDeviceList = NULL;
}

static bool create_perDeviceList(HTTPTRANSPORT_HANDLE_DATA* handleData)
{
    bool result;
    handleData->perDeviceList = VECTOR_create(sizeof(IOTHUB_DEVICE_HANDLE));
    if (handleData == NULL || handleData->perDeviceList == NULL)
    {
        result = false;
    }
    else
    {
        result = true;
    }
    return result;
}

static TRANSPORT_LL_HANDLE IoTHubTransportHttp_Create(const IOTHUBTRANSPORT_CONFIG* config, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    HTTPTRANSPORT_HANDLE_DATA* result;
    if (config == NULL)
    {
        LogError("invalid arg (configuration is missing)");
        result = NULL;
    }
    else if (config->upperConfig == NULL)
    {
        LogError("invalid arg (upperConfig is NULL)");
        result = NULL;
    }
    else if (config->upperConfig->protocol == NULL)
    {
        LogError("invalid arg (protocol is NULL)");
        result = NULL;
    }
    else if (config->upperConfig->iotHubName == NULL)
    {
        LogError("invalid arg (iotHubName is NULL)");
        result = NULL;
    }
    else if (config->upperConfig->iotHubSuffix == NULL)
    {
        LogError("invalid arg (iotHubSuffix is NULL)");
        result = NULL;
    }
    else if (IoTHub_Transport_ValidateCallbacks(cb_info) != 0)
    {
        LogError("Invalid transport callback information");
        result = NULL;
    }
    else
    {
        result = (HTTPTRANSPORT_HANDLE_DATA*)malloc(sizeof(HTTPTRANSPORT_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
        }
        else if (HTTPAPIEX_Init() == HTTPAPIEX_ERROR)
        {
            LogError("Error initializing HTTP");
            free(result);
            result = NULL;
        }
        else
        {
            bool was_hostName_ok = create_hostName(result, config);
            bool was_httpApiExHandle_ok = was_hostName_ok && create_httpApiExHandle(result, config);
            bool was_perDeviceList_ok = was_httpApiExHandle_ok && create_perDeviceList(result);

            if (was_perDeviceList_ok)
            {
                result->doBatchedTransfers = false;
                result->getMinimumPollingTime = DEFAULT_GETMINIMUMPOLLINGTIME;

                result->transport_ctx = ctx;
                memcpy(&result->transport_callbacks, cb_info, sizeof(TRANSPORT_CALLBACKS_INFO));
            }
            else
            {
                if (was_httpApiExHandle_ok) destroy_httpApiExHandle(result);
                if (was_hostName_ok) destroy_hostName(result);
                HTTPAPIEX_Deinit();

                free(result);
                result = NULL;
            }
        }
    }
    return result;
}

static void IoTHubTransportHttp_Destroy(TRANSPORT_LL_HANDLE handle)
{
    if (handle != NULL)
    {
        HTTPTRANSPORT_HANDLE_DATA* handleData = (HTTPTRANSPORT_HANDLE_DATA*)handle;
        IOTHUB_DEVICE_HANDLE* listItem;

        size_t deviceListSize = VECTOR_size(handleData->perDeviceList);

        for (size_t i = 0; i < deviceListSize; i++)
        {
            listItem = (IOTHUB_DEVICE_HANDLE *)VECTOR_element(handleData->perDeviceList, i);
            HTTPTRANSPORT_PERDEVICE_DATA* perDeviceItem = (HTTPTRANSPORT_PERDEVICE_DATA*)(*listItem);
            destroy_perDeviceData(perDeviceItem);
            free(perDeviceItem);
        }

        destroy_hostName((HTTPTRANSPORT_HANDLE_DATA *)handle);
        destroy_httpApiExHandle((HTTPTRANSPORT_HANDLE_DATA *)handle);
        destroy_perDeviceList((HTTPTRANSPORT_HANDLE_DATA *)handle);
        HTTPAPIEX_Deinit();
        free(handle);
    }
}

static int IoTHubTransportHttp_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("invalid arg passed to IoTHubTransportHttp_Subscribe");
        result = MU_FAILURE;
    }
    else
    {
        IOTHUB_DEVICE_HANDLE* listItem = get_perDeviceDataItem(handle);

        if (listItem == NULL)
        {
            LogError("did not find device in transport handle");
            result = MU_FAILURE;
        }
        else
        {
            HTTPTRANSPORT_PERDEVICE_DATA * perDeviceItem;

            perDeviceItem = (HTTPTRANSPORT_PERDEVICE_DATA *)(*listItem);
            perDeviceItem->DoWork_PullMessage = true;
            result = 0;
        }
    }
    return result;
}

static void IoTHubTransportHttp_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    if (handle != NULL)
    {
        IOTHUB_DEVICE_HANDLE* listItem = get_perDeviceDataItem(handle);
        if (listItem != NULL)
        {
            HTTPTRANSPORT_PERDEVICE_DATA * perDeviceItem = (HTTPTRANSPORT_PERDEVICE_DATA *)(*listItem);
            perDeviceItem->DoWork_PullMessage = false;
        }
        else
        {
            LogError("Device not found to unsuscribe.");
        }
    }
    else
    {
        LogError("Null handle passed to Unsuscribe.");
    }
}

static int IoTHubTransportHttp_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    int result = MU_FAILURE;
    LogError("IoTHubTransportHttp_Subscribe_DeviceTwin Not supported");
    return result;
}

static void IoTHubTransportHttp_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("IoTHubTransportHttp_Unsubscribe_DeviceTwin Not supported");
}

static IOTHUB_CLIENT_RESULT IoTHubTransportHttp_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    (void)handle;
    (void)completionCallback;
    (void)callbackContext;
    LogError("Currently Not Supported.");
    return IOTHUB_CLIENT_ERROR;
}

static int IoTHubTransportHttp_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    int result = MU_FAILURE;
    LogError("not implemented (yet)");
    return result;
}

static void IoTHubTransportHttp_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("not implemented (yet)");
}

static int IoTHubTransportHttp_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    (void)handle;
    (void)methodId;
    (void)response;
    (void)response_size;
    (void)status_response;
    LogError("not implemented (yet)");
    return MU_FAILURE;
}

/*produces a representation of the properties, if they exist*/
/*if they do not exist, produces ""*/
static int concat_Properties(STRING_HANDLE existing, MAP_HANDLE map, size_t* propertiesMessageSizeContribution)
{
    int result;
    const char*const* keys;
    const char*const* values;
    size_t count;
    if (Map_GetInternals(map, &keys, &values, &count) != MAP_OK)
    {
        result = MU_FAILURE;
        LogError("error while Map_GetInternals");
    }
    else
    {

        if (count == 0)
        {
            /*no properties - do nothing with existing*/
            result = 0;
            *propertiesMessageSizeContribution = 0;
        }
        else
        {
            if (STRING_concat(existing, ",\"properties\":") != 0)
            {
                /*go ahead and return it*/
                result = MU_FAILURE;
                LogError("failed STRING_concat");
            }
            else if (appendMapToJSON(existing, keys, values, count) != 0)
            {
                result = MU_FAILURE;
                LogError("unable to append the properties");
            }
            else
            {
                /*all is fine*/
                size_t i;
                *propertiesMessageSizeContribution = 0;
                for (i = 0; i < count; i++)
                {
                    *propertiesMessageSizeContribution += (strlen(keys[i]) + strlen(values[i]) + MAXIMUM_PROPERTY_OVERHEAD);
                }
                result = 0;
            }
        }
    }
    return result;
}

/*produces a JSON representation of the map : {"a": "value_of_a","b":"value_of_b"}*/
static int appendMapToJSON(STRING_HANDLE existing, const char* const* keys, const char* const* values, size_t count) /*under consideration: move to MAP module when it has more than 1 user*/
{
    int result;
    if (STRING_concat(existing, "{") != 0)
    {
        /*go on and return it*/
        LogError("STRING_construct failed");
        result = MU_FAILURE;
    }
    else
    {
        size_t i;
        for (i = 0; i < count; i++)
        {
            if (!(
                (STRING_concat(existing, (i == 0) ? "\"" IOTHUB_APP_PREFIX : ",\"" IOTHUB_APP_PREFIX) == 0) &&
                (STRING_concat(existing, keys[i]) == 0) &&
                (STRING_concat(existing, "\":\"") == 0) &&
                (STRING_concat(existing, values[i]) == 0) &&
                (STRING_concat(existing, "\"") == 0)
                ))
            {
                LogError("unable to STRING_concat");
                break;
            }
        }

        if (i < count)
        {
            result = MU_FAILURE;
            /*error, let it go through*/
        }
        else if (STRING_concat(existing, "}") != 0)
        {
            LogError("unable to STRING_concat");
            result = MU_FAILURE;
        }
        else
        {
            /*all is fine*/
            result = 0;
        }
    }
    return result;
}

/*makes the following string:{"body":"base64 encoding of the message content"[,"properties":{"a":"valueOfA"}]}*/
/*return NULL if there was a failure, or a non-NULL STRING_HANDLE that contains the intended data*/
static STRING_HANDLE make1EventJSONitem(PDLIST_ENTRY item, size_t *messageSizeContribution)
{
    STRING_HANDLE result; /*temp wants to contain :{"body":"base64 encoding of the message content"[,"properties":{"a":"valueOfA"}]}*/
    IOTHUB_MESSAGE_LIST* message = containingRecord(item, IOTHUB_MESSAGE_LIST, entry);
    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message->messageHandle);

    switch (contentType)
    {
    case IOTHUBMESSAGE_BYTEARRAY:
    {
        result = STRING_construct("{\"body\":\"");
        if (result == NULL)
        {
            LogError("unable to STRING_construct");
        }
        else
        {
            const unsigned char* source;
            size_t size;

            if (IoTHubMessage_GetByteArray(message->messageHandle, &source, &size) != IOTHUB_MESSAGE_OK)
            {
                LogError("unable to get the data for the message.");
                STRING_delete(result);
                result = NULL;
            }
            else
            {
                STRING_HANDLE encoded = Azure_Base64_Encode_Bytes(source, size);
                if (encoded == NULL)
                {
                    LogError("unable to Azure_Base64_Encode_Bytes.");
                    STRING_delete(result);
                    result = NULL;
                }
                else
                {
                    size_t propertiesSize = 0;
                    if (!(
                        (STRING_concat_with_STRING(result, encoded) == 0) &&
                        (STRING_concat(result, "\"") == 0) && /*\" because closing value*/
                        (concat_Properties(result, IoTHubMessage_Properties(message->messageHandle), &propertiesSize) == 0) &&
                        (STRING_concat(result, "},") == 0) /*the last comma shall be replaced by a ']' by DaCr's suggestion (which is awesome enough to receive credits in the source code)*/
                        ))
                    {
                        STRING_delete(result);
                        result = NULL;
                        LogError("unable to STRING_concat_with_STRING.");
                    }
                    else
                    {
                        /*all is fine... */
                        *messageSizeContribution = size + MAXIMUM_PAYLOAD_OVERHEAD + propertiesSize;
                    }
                    STRING_delete(encoded);
                }
            }
        }
        break;
    }
    case IOTHUBMESSAGE_STRING:
    {
        result = STRING_construct("{\"body\":");
        if (result == NULL)
        {
            LogError("unable to STRING_construct");
        }
        else
        {
            const char* source = IoTHubMessage_GetString(message->messageHandle);
            if (source == NULL)
            {
                LogError("unable to IoTHubMessage_GetString");
                STRING_delete(result);
                result = NULL;
            }
            else
            {
                STRING_HANDLE asJson = STRING_new_JSON(source);
                if (asJson == NULL)
                {
                    LogError("unable to STRING_new_JSON");
                    STRING_delete(result);
                    result = NULL;
                }
                else
                {
                    size_t propertiesSize = 0;
                    if (!(
                        (STRING_concat_with_STRING(result, asJson) == 0) &&
                        (STRING_concat(result, ",\"base64Encoded\":false") == 0) &&
                        (concat_Properties(result, IoTHubMessage_Properties(message->messageHandle), &propertiesSize) == 0) &&
                        (STRING_concat(result, "},") == 0) /*the last comma shall be replaced by a ']' by DaCr's suggestion (which is awesome enough to receive credits in the source code)*/
                        ))
                    {
                        LogError("unable to STRING_concat_with_STRING");
                        STRING_delete(result);
                        result = NULL;
                    }
                    else
                    {
                        /*result has the intended content*/
                        *messageSizeContribution = strlen(source) + MAXIMUM_PAYLOAD_OVERHEAD + propertiesSize;
                    }
                    STRING_delete(asJson);
                }
            }
        }
        break;
    }
    default:
    {
        LogError("an unknown message type was encountered (%d)", contentType);
        result = NULL; /*unknown message type*/
        break;
    }
    }
    return result;
}

#define MAKE_PAYLOAD_RESULT_VALUES \
    MAKE_PAYLOAD_OK, /*returned when there is a payload to be later send by HTTP*/ \
    MAKE_PAYLOAD_NO_ITEMS, /*returned when there are no items to be send*/ \
    MAKE_PAYLOAD_ERROR, /*returned when there were errors*/ \
    MAKE_PAYLOAD_FIRST_ITEM_DOES_NOT_FIT /*returned when the first item doesn't fit*/

MU_DEFINE_ENUM(MAKE_PAYLOAD_RESULT, MAKE_PAYLOAD_RESULT_VALUES);

/*this function assembles several {"body":"base64 encoding of the message content"," base64Encoded": true} into 1 payload*/
static MAKE_PAYLOAD_RESULT makePayload(HTTPTRANSPORT_PERDEVICE_DATA* deviceData, STRING_HANDLE* payload)
{
    MAKE_PAYLOAD_RESULT result;
    size_t allMessagesSize = 0;
    *payload = STRING_construct("[");
    if (*payload == NULL)
    {
        LogError("unable to STRING_construct");
        result = MAKE_PAYLOAD_ERROR;
    }
    else
    {
        bool isFirst = true;
        PDLIST_ENTRY actual;
        bool keepGoing = true; /*keepGoing gets sometimes to false from within the loop*/
                               /*either all the items enter the list or only some*/
        result = MAKE_PAYLOAD_OK; /*optimistically initializing it*/
        while (keepGoing && ((actual = deviceData->waitingToSend->Flink) != deviceData->waitingToSend))
        {
            size_t messageSize;
            STRING_HANDLE temp = make1EventJSONitem(actual, &messageSize);
            if (isFirst)
            {
                isFirst = false;
                if (temp == NULL) /*first item failed to create, nothing to send*/
                {
                    result = MAKE_PAYLOAD_ERROR;
                    STRING_delete(*payload);
                    *payload = NULL;
                    keepGoing = false;
                }
                else
                {
                    if (messageSize > MAXIMUM_MESSAGE_SIZE)
                    {
                        PDLIST_ENTRY head = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                        DList_InsertTailList(&(deviceData->eventConfirmations), head);
                        result = MAKE_PAYLOAD_FIRST_ITEM_DOES_NOT_FIT;
                        STRING_delete(*payload);
                        *payload = NULL;
                        keepGoing = false;
                    }
                    else
                    {
                        if (STRING_concat_with_STRING(*payload, temp) != 0)
                        {
                            result = MAKE_PAYLOAD_ERROR;
                            STRING_delete(*payload);
                            *payload = NULL;
                            keepGoing = false;
                        }
                        else
                        {
                            /*first item was put nicely in the payload*/
                            PDLIST_ENTRY head = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                            DList_InsertTailList(&(deviceData->eventConfirmations), head);
                            allMessagesSize += messageSize;
                        }
                    }
                    STRING_delete(temp);
                }
            }
            else
            {
                /*there is at least 1 item already in the payload*/
                if (temp == NULL)
                {
                    /*there are multiple payloads encoded, the last one had an internal error, just go with those - closing the payload happens "after the loop"*/
                    result = MAKE_PAYLOAD_OK;
                    keepGoing = false;
                }
                else
                {
                    if (allMessagesSize + messageSize > MAXIMUM_MESSAGE_SIZE)
                    {
                        /*this item doesn't make it to the payload, but the payload is valid so far*/
                        result = MAKE_PAYLOAD_OK;
                        keepGoing = false;
                    }
                    else if (STRING_concat_with_STRING(*payload, temp) != 0)
                    {
                        /*should still send what there is so far...*/
                        result = MAKE_PAYLOAD_OK;
                        keepGoing = false;
                    }
                    else
                    {
                        /*cool, the payload made it there, let's continue... */
                        PDLIST_ENTRY head = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                        DList_InsertTailList(&(deviceData->eventConfirmations), head);
                        allMessagesSize += messageSize;
                    }
                    STRING_delete(temp);
                }
            }
        }

        /*closing the payload*/
        if (result == MAKE_PAYLOAD_OK)
        {
            ((char*)STRING_c_str(*payload))[STRING_length(*payload) - 1] = ']';
        }
        else
        {
            /*no need to close anything*/
        }
    }
    return result;
}

static void reversePutListBackIn(PDLIST_ENTRY source, PDLIST_ENTRY destination)
{
    /*this function takes a list, and inserts it in another list. When done in the context of this file, it reverses the effects of a not-able-to-send situation*/
    DList_AppendTailList(destination->Flink, source);
    DList_RemoveEntryList(source);
    DList_InitializeListHead(source);
}

static void DoEvent(HTTPTRANSPORT_HANDLE_DATA* handleData, HTTPTRANSPORT_PERDEVICE_DATA* deviceData)
{

    if (DList_IsListEmpty(deviceData->waitingToSend))
    {
    }
    else
    {
        if (handleData->doBatchedTransfers)
        {
            if (HTTPHeaders_ReplaceHeaderNameValuePair(deviceData->eventHTTPrequestHeaders, CONTENT_TYPE, APPLICATION_VND_MICROSOFT_IOTHUB_JSON) != HTTP_HEADERS_OK)
            {
                LogError("unable to HTTPHeaders_ReplaceHeaderNameValuePair");
            }
            else
            {
                STRING_HANDLE payload;
                switch (makePayload(deviceData, &payload))
                {
                case MAKE_PAYLOAD_OK:
                {
                    BUFFER_HANDLE temp = BUFFER_new();
                    if (temp == NULL)
                    {
                        LogError("unable to BUFFER_new");
                        reversePutListBackIn(&(deviceData->eventConfirmations), deviceData->waitingToSend);
                    }
                    else
                    {
                        if (BUFFER_build(temp, (const unsigned char*)STRING_c_str(payload), STRING_length(payload)) != 0)
                        {
                            LogError("unable to BUFFER_build");
                            //items go back to waitingToSend
                            reversePutListBackIn(&(deviceData->eventConfirmations), deviceData->waitingToSend);
                        }
                        else
                        {
                            unsigned int statusCode;
                            if (HTTPAPIEX_SAS_ExecuteRequest(
                                deviceData->sasObject,
                                handleData->httpApiExHandle,
                                HTTPAPI_REQUEST_POST,
                                STRING_c_str(deviceData->eventHTTPrelativePath),
                                deviceData->eventHTTPrequestHeaders,
                                temp,
                                &statusCode,
                                NULL,
                                NULL
                            ) != HTTPAPIEX_OK)
                            {
                                LogError("unable to HTTPAPIEX_ExecuteRequest");
                                //items go back to waitingToSend
                                reversePutListBackIn(&(deviceData->eventConfirmations), deviceData->waitingToSend);
                            }
                            else
                            {
                                if (statusCode < 300)
                                {
                                    handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_OK, deviceData->device_transport_ctx);
                                }
                                else
                                {
                                    //items go back to waitingToSend
                                    LogError("unexpected HTTP status code (%u)", statusCode);
                                    reversePutListBackIn(&(deviceData->eventConfirmations), deviceData->waitingToSend);
                                }
                            }
                        }
                        BUFFER_delete(temp);
                    }
                    STRING_delete(payload);
                    break;
                }
                case MAKE_PAYLOAD_FIRST_ITEM_DOES_NOT_FIT:
                {
                    handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_ERROR, deviceData->device_transport_ctx); // takes care of emptying the list too
                    break;
                }
                case MAKE_PAYLOAD_ERROR:
                {
                    LogError("unrecoverable errors while building a batch message");
                    break;
                }
                case MAKE_PAYLOAD_NO_ITEMS:
                {
                    /*do nothing*/
                    break;
                }
                default:
                {
                    LogError("internal error: switch's default branch reached when never intended");
                    break;
                }
                }
            }
        }
        else
        {
            const unsigned char* messageContent = NULL;
            size_t messageSize = 0;
            size_t originalMessageSize = 0;
            IOTHUB_MESSAGE_LIST* message = containingRecord(deviceData->waitingToSend->Flink, IOTHUB_MESSAGE_LIST, entry);
            IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(message->messageHandle);

            if (!(
                (((contentType == IOTHUBMESSAGE_BYTEARRAY) &&
                (IoTHubMessage_GetByteArray(message->messageHandle, &messageContent, &originalMessageSize) == IOTHUB_MESSAGE_OK))
                    ? ((void)(messageSize = originalMessageSize + MAXIMUM_PAYLOAD_OVERHEAD), 1)
                    : 0)

                ||

                ((contentType == IOTHUBMESSAGE_STRING) &&
                ((void)(messageContent = (const unsigned char*)IoTHubMessage_GetString(message->messageHandle)),
                    ((void)(messageSize = MAXIMUM_PAYLOAD_OVERHEAD + (originalMessageSize = ((messageContent == NULL)
                        ? 0
                        : strlen((const char*)messageContent))))),
                    messageContent != NULL)
                    )
                ))
            {
                LogError("unable to get the message content");
                /*go on...*/
            }
            else
            {
                if (messageSize > MAXIMUM_MESSAGE_SIZE)
                {
                    PDLIST_ENTRY head = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                    DList_InsertTailList(&(deviceData->eventConfirmations), head);
                    handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_ERROR, deviceData->device_transport_ctx); // takes care of emptying the list too
                }
                else
                {
                    HTTP_HEADERS_HANDLE clonedEventHTTPrequestHeaders = HTTPHeaders_Clone(deviceData->eventHTTPrequestHeaders);
                    if (clonedEventHTTPrequestHeaders == NULL)
                    {
                        LogError("HTTPHeaders_Clone failed");
                    }
                    else
                    {
                        if (HTTPHeaders_ReplaceHeaderNameValuePair(clonedEventHTTPrequestHeaders, CONTENT_TYPE, APPLICATION_OCTET_STREAM) != HTTP_HEADERS_OK)
                        {
                            LogError("HTTPHeaders_ReplaceHeaderNameValuePair failed");
                        }
                        else
                        {
                            // set_message_properties returning false does not necessarily mean the the function failed, it just means
                            // the the adding of messages should not continue and should try the next time.  So you should not log if this
                            // returns false
                            if (set_message_properties(message, &messageSize, clonedEventHTTPrequestHeaders, handleData, deviceData))
                            {

                                if (set_system_properties(message, clonedEventHTTPrequestHeaders) != 0)
                                {
                                }
                                else
                                {
                                    BUFFER_HANDLE toBeSend = BUFFER_create(messageContent, originalMessageSize);
                                    if (toBeSend == NULL)
                                    {
                                        LogError("unable to BUFFER_new");
                                    }
                                    else
                                    {
                                        unsigned int statusCode = 0;
                                        HTTPAPIEX_RESULT r;
                                        if (deviceData->deviceSasToken != NULL)
                                        {
                                            if (HTTPHeaders_ReplaceHeaderNameValuePair(clonedEventHTTPrequestHeaders, IOTHUB_AUTH_HEADER_VALUE, STRING_c_str(deviceData->deviceSasToken)) != HTTP_HEADERS_OK)
                                            {
                                                r = HTTPAPIEX_ERROR;
                                                LogError("Unable to replace the old SAS Token.");
                                            }
                                            else if ((r = HTTPAPIEX_ExecuteRequest(
                                                handleData->httpApiExHandle, HTTPAPI_REQUEST_POST, STRING_c_str(deviceData->eventHTTPrelativePath),
                                                clonedEventHTTPrequestHeaders, toBeSend, &statusCode, NULL, NULL)) != HTTPAPIEX_OK)
                                            {
                                                LogError("Unable to HTTPAPIEX_ExecuteRequest.");
                                            }
                                        }
                                        else
                                        {
                                            if ((r = HTTPAPIEX_SAS_ExecuteRequest(deviceData->sasObject, handleData->httpApiExHandle, HTTPAPI_REQUEST_POST, STRING_c_str(deviceData->eventHTTPrelativePath),
                                                clonedEventHTTPrequestHeaders, toBeSend, &statusCode, NULL, NULL )) != HTTPAPIEX_OK)
                                            {
                                                LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                                            }
                                        }
                                        if (r == HTTPAPIEX_OK)
                                        {
                                            if (statusCode < 300)
                                            {
                                                PDLIST_ENTRY justSent = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                                                DList_InsertTailList(&(deviceData->eventConfirmations), justSent);
                                                handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_OK, deviceData->device_transport_ctx); // takes care of emptying the list too
                                            }
                                            else
                                            {
                                                LogError("unexpected HTTP status code (%u)", statusCode);
                                            }
                                        }
                                        else if (r == HTTPAPIEX_RECOVERYFAILED)
                                        {
                                            PDLIST_ENTRY justSent = DList_RemoveHeadList(deviceData->waitingToSend); /*actually this is the same as "actual", but now it is removed*/
                                            DList_InsertTailList(&(deviceData->eventConfirmations), justSent);
                                            handleData->transport_callbacks.send_complete_cb(&(deviceData->eventConfirmations), IOTHUB_CLIENT_CONFIRMATION_ERROR, deviceData->device_transport_ctx); // takes care of emptying the list too
                                        }
                                    }
                                    BUFFER_delete(toBeSend);
                                }
                            }
                        }
                        HTTPHeaders_Free(clonedEventHTTPrequestHeaders);
                    }
                }
            }
        }
    }
}

static bool abandonOrAcceptMessage(HTTPTRANSPORT_HANDLE_DATA* handleData, HTTPTRANSPORT_PERDEVICE_DATA* deviceData, const char* ETag, IOTHUBMESSAGE_DISPOSITION_RESULT action)
{
    bool result;
    STRING_HANDLE fullAbandonRelativePath = STRING_clone(deviceData->abandonHTTPrelativePathBegin);
    if (fullAbandonRelativePath == NULL)
    {
        LogError("unable to STRING_clone");
        result = false;
    }
    else
    {
        STRING_HANDLE ETagUnquoted = STRING_construct_n(ETag + 1, strlen(ETag) - 2); /*skip first character which is '"' and the last one (which is also '"')*/
        if (ETagUnquoted == NULL)
        {
            LogError("unable to STRING_construct_n");
            result = false;
        }
        else
        {
            if (!(
                (STRING_concat_with_STRING(fullAbandonRelativePath, ETagUnquoted) == 0) &&
                (STRING_concat(fullAbandonRelativePath, (action == IOTHUBMESSAGE_ABANDONED) ? "/abandon" API_VERSION : ((action == IOTHUBMESSAGE_REJECTED) ? API_VERSION "&reject" : API_VERSION)) == 0)
                ))
            {
                LogError("unable to STRING_concat");
                result = false;
            }
            else
            {
                HTTP_HEADERS_HANDLE abandonRequestHttpHeaders = HTTPHeaders_Alloc();
                if (abandonRequestHttpHeaders == NULL)
                {
                    LogError("unable to HTTPHeaders_Alloc");
                    result = false;
                }
                else
                {
                    if (!(
                        (addUserAgentHeaderInfo(handleData, abandonRequestHttpHeaders) == HTTP_HEADERS_OK) &&
                        (HTTPHeaders_AddHeaderNameValuePair(abandonRequestHttpHeaders, IOTHUB_AUTH_HEADER_VALUE, " ") == HTTP_HEADERS_OK) &&
                        (HTTPHeaders_AddHeaderNameValuePair(abandonRequestHttpHeaders, "If-Match", ETag) == HTTP_HEADERS_OK)
                        ))
                    {
                        LogError("unable to HTTPHeaders_AddHeaderNameValuePair");
                        result = false;
                    }
                    else
                    {
                        unsigned int statusCode = 0;
                        HTTPAPIEX_RESULT r;
                        if (deviceData->deviceSasToken != NULL)
                        {
                            if (HTTPHeaders_ReplaceHeaderNameValuePair(abandonRequestHttpHeaders, IOTHUB_AUTH_HEADER_VALUE, STRING_c_str(deviceData->deviceSasToken)) != HTTP_HEADERS_OK)
                            {
                                r = HTTPAPIEX_ERROR;
                                LogError("Unable to replace the old SAS Token.");
                            }
                            else if ((r = HTTPAPIEX_ExecuteRequest(
                                handleData->httpApiExHandle,
                                (action == IOTHUBMESSAGE_ABANDONED) ? HTTPAPI_REQUEST_POST : HTTPAPI_REQUEST_DELETE,                               /*-requestType: POST                                                                                                       */
                                STRING_c_str(fullAbandonRelativePath),              /*-relativePath: abandon relative path begin (as created by _Create) + value of ETag + "/abandon?api-version=2016-11-14"   */
                                abandonRequestHttpHeaders,                          /*- requestHttpHeadersHandle: an HTTP headers instance containing the following                                            */
                                NULL,                                               /*- requestContent: NULL                                                                                                   */
                                &statusCode,                                         /*- statusCode: a pointer to unsigned int which might be examined for logging                                              */
                                NULL,                                               /*- responseHeadearsHandle: NULL                                                                                           */
                                NULL                                                /*- responseContent: NULL]                                                                                                 */
                            )) != HTTPAPIEX_OK)
                            {
                                LogError("Unable to HTTPAPIEX_ExecuteRequest.");
                            }
                        }
                        else if ((r = HTTPAPIEX_SAS_ExecuteRequest(
                            deviceData->sasObject,
                            handleData->httpApiExHandle,
                            (action == IOTHUBMESSAGE_ABANDONED) ? HTTPAPI_REQUEST_POST : HTTPAPI_REQUEST_DELETE,                               /*-requestType: POST                                                                                                       */
                            STRING_c_str(fullAbandonRelativePath),              /*-relativePath: abandon relative path begin (as created by _Create) + value of ETag + "/abandon?api-version=2016-11-14"   */
                            abandonRequestHttpHeaders,                          /*- requestHttpHeadersHandle: an HTTP headers instance containing the following                                            */
                            NULL,                                               /*- requestContent: NULL                                                                                                   */
                            &statusCode,                                         /*- statusCode: a pointer to unsigned int which might be examined for logging                                              */
                            NULL,                                               /*- responseHeadearsHandle: NULL                                                                                           */
                            NULL                                                /*- responseContent: NULL]                                                                                                 */
                        )) != HTTPAPIEX_OK)
                        {
                            LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                        }
                        if (r == HTTPAPIEX_OK)
                        {
                            if (statusCode != 204)
                            {
                                LogError("unexpected status code returned %u (was expecting 204)", statusCode);
                                result = false;
                            }
                            else
                            {
                                /*all is fine*/
                                result = true;
                            }
                        }
                        else
                        {
                            result = false;
                        }
                    }
                    HTTPHeaders_Free(abandonRequestHttpHeaders);
                }
            }
            STRING_delete(ETagUnquoted);
        }
        STRING_delete(fullAbandonRelativePath);
    }
    return result;
}

static IOTHUB_CLIENT_RESULT IoTHubTransportHttp_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE message, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{

    IOTHUB_CLIENT_RESULT result;
    if (handle == NULL || message == NULL)
    {
        LogError("invalid argument (handle=%p, message=%p)", handle, message);
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        MESSAGE_DISPOSITION_CONTEXT* dispositionContext = NULL;
        
        if (IoTHubMessage_GetDispositionContext(message, &dispositionContext) != IOTHUB_MESSAGE_OK ||
            dispositionContext == NULL)
        {
            LogError("invalid message handle (no disposition context found)");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            if (dispositionContext->etagValue == NULL)
            {
                LogError("invalid transport context data");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                HTTPTRANSPORT_PERDEVICE_DATA* deviceData = (HTTPTRANSPORT_PERDEVICE_DATA*)handle;

                if (abandonOrAcceptMessage(deviceData->transportHandle, deviceData, dispositionContext->etagValue, disposition))
                {
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    LogError("HTTP Transport layer failed to report %s disposition", MU_ENUM_TO_STRING(IOTHUBMESSAGE_DISPOSITION_RESULT, disposition));
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }

        // This will destroy the disposition context as well.
        IoTHubMessage_Destroy(message);
    }
    return result;
}

static void DestroyMessageDispositionContext(MESSAGE_DISPOSITION_CONTEXT* dispositionContext)
{
    if (dispositionContext != NULL)
    {
        free(dispositionContext->etagValue);
        free(dispositionContext);
    }
}

static MESSAGE_DISPOSITION_CONTEXT* CreateMessageDispositionContext(const char* etagValue)
{
    MESSAGE_DISPOSITION_CONTEXT* result = calloc(1, sizeof(MESSAGE_DISPOSITION_CONTEXT));

    if (result == NULL)
    {
        LogError("malloc failed");
    }
    else if (mallocAndStrcpy_s(&result->etagValue, etagValue) != 0)
    {
        LogError("mallocAndStrcpy_s failed");
        free(result);
        result = NULL;
    }

    return result;
}

static void DoMessages(HTTPTRANSPORT_HANDLE_DATA* handleData, HTTPTRANSPORT_PERDEVICE_DATA* deviceData)
{
    if (deviceData->DoWork_PullMessage)
    {
        time_t timeNow = get_time(NULL);
        bool isPollingAllowed = deviceData->isFirstPoll || (timeNow == (time_t)(-1)) || (get_difftime(timeNow, deviceData->lastPollTime) > handleData->getMinimumPollingTime);
        if (isPollingAllowed)
        {
            HTTP_HEADERS_HANDLE responseHTTPHeaders = HTTPHeaders_Alloc();
            if (responseHTTPHeaders == NULL)
            {
                LogError("unable to HTTPHeaders_Alloc");
            }
            else
            {
                BUFFER_HANDLE responseContent = BUFFER_new();
                if (responseContent == NULL)
                {
                    LogError("unable to BUFFER_new");
                }
                else
                {
                    unsigned int statusCode = 0;
                    HTTPAPIEX_RESULT r;
                    if (deviceData->deviceSasToken != NULL)
                    {
                        if (HTTPHeaders_ReplaceHeaderNameValuePair(deviceData->messageHTTPrequestHeaders, IOTHUB_AUTH_HEADER_VALUE, STRING_c_str(deviceData->deviceSasToken)) != HTTP_HEADERS_OK)
                        {
                            r = HTTPAPIEX_ERROR;
                            LogError("Unable to replace the old SAS Token.");
                        }
                        else if ((r = HTTPAPIEX_ExecuteRequest(
                            handleData->httpApiExHandle,
                            HTTPAPI_REQUEST_GET,                                            /*requestType: GET*/
                            STRING_c_str(deviceData->messageHTTPrelativePath),         /*relativePath: the message HTTP relative path*/
                            deviceData->messageHTTPrequestHeaders,                     /*requestHttpHeadersHandle: message HTTP request headers created by _Create*/
                            NULL,                                                           /*requestContent: NULL*/
                            &statusCode,                                                    /*statusCode: a pointer to unsigned int which shall be later examined*/
                            responseHTTPHeaders,                                            /*responseHeadearsHandle: a new instance of HTTP headers*/
                            responseContent                                                 /*responseContent: a new instance of buffer*/
                        )) != HTTPAPIEX_OK)
                        {
                            LogError("Unable to HTTPAPIEX_ExecuteRequest.");
                        }
                    }
                    else if ((r = HTTPAPIEX_SAS_ExecuteRequest(
                        deviceData->sasObject,
                        handleData->httpApiExHandle,
                        HTTPAPI_REQUEST_GET,                                            /*requestType: GET*/
                        STRING_c_str(deviceData->messageHTTPrelativePath),         /*relativePath: the message HTTP relative path*/
                        deviceData->messageHTTPrequestHeaders,                     /*requestHttpHeadersHandle: message HTTP request headers created by _Create*/
                        NULL,                                                           /*requestContent: NULL*/
                        &statusCode,                                                    /*statusCode: a pointer to unsigned int which shall be later examined*/
                        responseHTTPHeaders,                                            /*responseHeadearsHandle: a new instance of HTTP headers*/
                        responseContent                                                 /*responseContent: a new instance of buffer*/
                    )) != HTTPAPIEX_OK)
                    {
                        LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                    }
                    if (r == HTTPAPIEX_OK)
                    {
                        /*HTTP dialogue was succesfull*/
                        if (timeNow == (time_t)(-1))
                        {
                            deviceData->isFirstPoll = true;
                        }
                        else
                        {
                            deviceData->isFirstPoll = false;
                            deviceData->lastPollTime = timeNow;
                        }
                        if (statusCode == 204)
                        {
                            /*this is an expected status code, means "no commands", but logging that creates panic*/

                            /*do nothing, advance to next action*/
                        }
                        else if (statusCode != 200)
                        {
                            LogError("expected status code was 200, but actually was received %u... moving on", statusCode);
                        }
                        else
                        {
                            const char* etagValue = HTTPHeaders_FindHeaderValue(responseHTTPHeaders, "ETag");
                            if (etagValue == NULL)
                            {
                                LogError("unable to find a received header called \"E-Tag\"");
                            }
                            else
                            {
                                size_t etagsize = strlen(etagValue);
                                if (
                                    (etagsize < 2) ||
                                    (etagValue[0] != '"') ||
                                    (etagValue[etagsize - 1] != '"')
                                    )
                                {
                                    LogError("ETag is not a valid quoted string");
                                }
                                else
                                {
                                    const unsigned char* resp_content;
                                    size_t resp_len;
                                    resp_content = BUFFER_u_char(responseContent);
                                    resp_len = BUFFER_length(responseContent);
                                    IOTHUB_MESSAGE_HANDLE receivedMessage = IoTHubMessage_CreateFromByteArray(resp_content, resp_len);
                                    bool abandon = false;
                                    if (receivedMessage == NULL)
                                    {
                                        LogError("unable to IoTHubMessage_CreateFromByteArray, trying to abandon the message... ");
                                        abandon = true;
                                    }
                                    else
                                    {
                                        if (retrieve_message_properties(responseHTTPHeaders, receivedMessage) != 0)
                                        {
                                            LogError("Failed retrieving message properties");
                                            abandon = true;
                                        }
                                        else
                                        {
                                            MESSAGE_DISPOSITION_CONTEXT* dispositionContext = CreateMessageDispositionContext(etagValue);

                                            if (dispositionContext == NULL)
                                            {
                                                LogError("failed to create disposition context");
                                                abandon = true;
                                            }
                                            else
                                            {
                                                if (IoTHubMessage_SetDispositionContext(receivedMessage, dispositionContext, DestroyMessageDispositionContext) != IOTHUB_MESSAGE_OK)
                                                {
                                                    LogError("Failed setting disposition context in IOTHUB_MESSAGE_HANDLE");
                                                    DestroyMessageDispositionContext(dispositionContext);
                                                    abandon = true;
                                                }
                                                else if (!handleData->transport_callbacks.msg_cb(receivedMessage, deviceData->device_transport_ctx))
                                                {
                                                    LogError("IoTHubClientCore_LL_MessageCallback failed");
                                                    abandon = true;
                                                }
                                            }
                                        }

                                        if (abandon)
                                        {
                                            // If IoTHubMessage_SetDispositionContext succeeds above, it transitions ownership of 
                                            // dispositionContext to the receivedMessage.
                                            // IoTHubMessage_Destroy() below will handle freeing it.
                                            IoTHubMessage_Destroy(receivedMessage);
                                        }
                                    }

                                    if (abandon)
                                    {
                                        if (!abandonOrAcceptMessage(handleData, deviceData, etagValue, IOTHUBMESSAGE_ABANDONED))
                                        {
                                            LogError("HTTP Transport layer failed to report ABANDON disposition");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    BUFFER_delete(responseContent);
                }
                HTTPHeaders_Free(responseHTTPHeaders);
            }
        }
        else
        {
            /*isPollingAllowed is false... */
            /*do nothing "shall be ignored*/
        }
    }
}

static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportHttp_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    (void)handle;
    (void)item_type;
    (void)iothub_item;
    LogError("Currently Not Supported.");
    return IOTHUB_PROCESS_ERROR;
}

static void IoTHubTransportHttp_DoWork(TRANSPORT_LL_HANDLE handle)
{
    if (handle != NULL)
    {
        HTTPTRANSPORT_HANDLE_DATA* handleData = (HTTPTRANSPORT_HANDLE_DATA*)handle;
        IOTHUB_DEVICE_HANDLE* listItem;
        size_t deviceListSize = VECTOR_size(handleData->perDeviceList);
        for (size_t i = 0; i < deviceListSize; i++)
        {
            listItem = (IOTHUB_DEVICE_HANDLE *)VECTOR_element(handleData->perDeviceList, i);
            HTTPTRANSPORT_PERDEVICE_DATA* perDeviceItem = *(HTTPTRANSPORT_PERDEVICE_DATA**)(listItem);
            DoEvent(handleData, perDeviceItem);
            DoMessages(handleData, perDeviceItem);
        }
    }
    else
    {
        LogError("Invalid Argument NULL call on DoWork.");
    }
}

static IOTHUB_CLIENT_RESULT IoTHubTransportHttp_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid handle to IoTHubClient HTTP transport instance.");
    }
    else if (iotHubClientStatus == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid pointer to output parameter IOTHUB_CLIENT_STATUS.");
    }
    else
    {
        IOTHUB_DEVICE_HANDLE* listItem = get_perDeviceDataItem(handle);
        if (listItem == NULL)
        {
            result = IOTHUB_CLIENT_INVALID_ARG;
            LogError("Device not found in transport list.");
        }
        else
        {
            HTTPTRANSPORT_PERDEVICE_DATA* deviceData = (HTTPTRANSPORT_PERDEVICE_DATA*)(*listItem);
            if (!DList_IsListEmpty(deviceData->waitingToSend))
            {
                *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
            }
            else
            {
                *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
            }
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

static IOTHUB_CLIENT_RESULT IoTHubTransportHttp_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    IOTHUB_CLIENT_RESULT result;
    if (
        (handle == NULL) ||
        (option == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid parameter (NULL) passed to IoTHubTransportHttp_SetOption");
    }
    else
    {
        HTTPTRANSPORT_HANDLE_DATA* handleData = (HTTPTRANSPORT_HANDLE_DATA*)handle;
        if (strcmp(OPTION_BATCHING, option) == 0)
        {
            handleData->doBatchedTransfers = *(bool*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_MIN_POLLING_TIME, option) == 0)
        {
            handleData->getMinimumPollingTime = *(unsigned int*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            HTTPAPIEX_RESULT HTTPAPIEX_result = HTTPAPIEX_SetOption(handleData->httpApiExHandle, option, value);
            if (HTTPAPIEX_result == HTTPAPIEX_OK)
            {
                result = IOTHUB_CLIENT_OK;
            }
            else if (HTTPAPIEX_result == HTTPAPIEX_INVALID_ARG)
            {
                result = IOTHUB_CLIENT_INVALID_ARG;
                LogError("HTTPAPIEX_SetOption failed");
            }
            else
            {
                result = IOTHUB_CLIENT_ERROR;
                LogError("HTTPAPIEX_SetOption failed");
            }
        }
    }
    return result;
}

static STRING_HANDLE IoTHubTransportHttp_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    STRING_HANDLE result;
    if (handle == NULL)
    {
        LogError("invalid parameter handle=%p", handle);
        result = NULL;
    }
    else if ((result = STRING_clone(((HTTPTRANSPORT_HANDLE_DATA*)(handle))->hostName)) == NULL)
    {
        LogError("Cannot provide the target host name (STRING_clone failed).");
    }

    return result;
}

static int IoTHubTransportHttp_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    int result;
    (void)handle;
    (void)retryPolicy;
    (void)retryTimeoutLimitInSeconds;

    /* Retry Policy is not currently not available for HTTP */

    result = 0;
    return result;
}

static int IotHubTransportHttp_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("HTTP does not support input queues");
    return MU_FAILURE;
}

static void IotHubTransportHttp_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("HTTP does not support input queues");
}

int IoTHubTransportHttp_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid parameter specified handle: %p", handle);
        result = MU_FAILURE;
    }
    else
    {
        HTTPTRANSPORT_HANDLE_DATA* handleData = (HTTPTRANSPORT_HANDLE_DATA*)handle;
        handleData->transport_ctx = ctx;
        result = 0;
    }
    return result;
}

static int IoTHubTransportHttp_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
{
    int result;
    if (handle == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        *info = PLATFORM_INFO_OPTION_DEFAULT;
        result = 0;
    }

    return result;
}

static TRANSPORT_PROVIDER thisTransportProvider =
{
    IoTHubTransportHttp_SendMessageDisposition,     /*pfIotHubTransport_SendMessageDisposition IoTHubTransport_SendMessageDisposition;*/
    IoTHubTransportHttp_Subscribe_DeviceMethod,     /*pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;*/
    IoTHubTransportHttp_Unsubscribe_DeviceMethod,   /*pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;*/
    IoTHubTransportHttp_DeviceMethod_Response,      /*pfIoTHubTransport_DeviceMethod_Response IoTHubTransport_DeviceMethod_Response;*/
    IoTHubTransportHttp_Subscribe_DeviceTwin,       /*pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin;*/
    IoTHubTransportHttp_Unsubscribe_DeviceTwin,     /*pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin;*/
    IoTHubTransportHttp_ProcessItem,                /*pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem;*/
    IoTHubTransportHttp_GetHostname,                /*pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname;*/
    IoTHubTransportHttp_SetOption,                  /*pfIoTHubTransport_SetOption IoTHubTransport_SetOption;*/
    IoTHubTransportHttp_Create,                     /*pfIoTHubTransport_Create IoTHubTransport_Create;*/
    IoTHubTransportHttp_Destroy,                    /*pfIoTHubTransport_Destroy IoTHubTransport_Destroy;*/
    IoTHubTransportHttp_Register,                   /*pfIotHubTransport_Register IoTHubTransport_Register;*/
    IoTHubTransportHttp_Unregister,                 /*pfIotHubTransport_Unregister IoTHubTransport_Unregister;*/
    IoTHubTransportHttp_Subscribe,                  /*pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;*/
    IoTHubTransportHttp_Unsubscribe,                /*pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;*/
    IoTHubTransportHttp_DoWork,                     /*pfIoTHubTransport_DoWork IoTHubTransport_DoWork;*/
    IoTHubTransportHttp_SetRetryPolicy,             /*pfIoTHubTransport_DoWork IoTHubTransport_SetRetryPolicy;*/
    IoTHubTransportHttp_GetSendStatus,              /*pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus;*/
    IotHubTransportHttp_Subscribe_InputQueue,       /*pfIoTHubTransport_Subscribe_InputQueue IoTHubTransport_Subscribe_InputQueue; */
    IotHubTransportHttp_Unsubscribe_InputQueue,     /*pfIoTHubTransport_Unsubscribe_InputQueue IoTHubTransport_Unsubscribe_InputQueue; */
    IoTHubTransportHttp_SetCallbackContext,         /*pfIoTHubTransport_SetTransportCallbacks IoTHubTransport_SetTransportCallbacks; */
    IoTHubTransportHttp_GetTwinAsync,               /*pfIoTHubTransport_GetTwinAsync IoTHubTransport_GetTwinAsync;*/
    IoTHubTransportHttp_GetSupportedPlatformInfo      /*pfIoTHubTransport_GetSupportedPlatformInfo IoTHubTransport_GetSupportedPlatformInfo;*/
};

const TRANSPORT_PROVIDER* HTTP_Protocol(void)
{
    return &thisTransportProvider;
}
