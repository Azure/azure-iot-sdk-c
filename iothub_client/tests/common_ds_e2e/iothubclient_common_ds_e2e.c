// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include "iothubclient_common_ds_e2e.h"
#include "testrunnerswitcher.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_module_client.h"
#include "iothub_client_streaming.h"
#include "iothub_client_options.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/connection_string_parser.h"

#include "azure_c_shared_utility/ws_url.h"
#include "azure_c_shared_utility/uws_client.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/wsio.h"
#include "azure_c_shared_utility/platform.h"

#include "parson.h"
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(WS_OPEN_RESULT, WS_OPEN_RESULT_VALUES)
MU_DEFINE_ENUM_STRINGS(WS_ERROR, WS_ERROR_VALUES)
MU_DEFINE_ENUM_STRINGS(WS_SEND_FRAME_RESULT, WS_SEND_FRAME_RESULT_VALUES)

static const char* HTTP_HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* HTTP_HEADER_VAL_AUTHORIZATION = " ";
static const char* HTTP_HEADER_KEY_REQUEST_ID = "Request-Id";
static const char* HTTP_HEADER_KEY_USER_AGENT = "User-Agent";
static const char* HTTP_HEADER_VAL_USER_AGENT = "iotHubClientTestHttpAgent";
static const char* HTTP_HEADER_KEY_ACCEPT = "Accept";;
static const char* HTTP_HEADER_VAL_ACCEPT = "application/json";
static const char* HTTP_HEADER_KEY_CONTENT_TYPE = "Content-Type";
static const char* HTTP_HEADER_VAL_CONTENT_TYPE = "application/json; charset=utf-8";
static const char* HTTP_HEADER_KEY_CONNECT_TIMEOUT_SECS = "iothub-streaming-connect-timeout-in-seconds";
static const char* HTTP_HEADER_VAL_CONNECT_TIMEOUT_SECS = "60";
static const char* HTTP_HEADER_KEY_RESPONSE_TIMEOUT_SECS = "iothub-streaming-response-timeout-in-seconds";
static const char* HTTP_HEADER_VAL_RESPONSE_TIMEOUT_SECS = "60";
static const int UID_LENGTH = 37;
static const char* const URL_API_VERSION = "api-version=2018-06-30";
static const char* const DEVICE_STREAMING_URI_RELATIVE_PATH_FMT = "/twins/%s/streams/%s?%s";
static const char* const MODULE_STREAMING_URI_RELATIVE_PATH_FMT = "/twins/%s/modules/%s/streams/%s?%s";

static IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;
static IOTHUB_DEVICE_CLIENT_HANDLE iothub_deviceclient_handle = NULL;
static IOTHUB_MODULE_CLIENT_HANDLE iothub_moduleclient_handle = NULL;

static const char* STREAM_NAME_FMT = "iothubclient_e2etest_%s";
static const size_t STREAM_NAME_FMT_LENGTH = 21;

static const char* DEVICE_STREAMING_RESPONSE_FIELD_IS_ACCEPTED = "iothub-streaming-is-accepted";
static const char* DEVICE_STREAMING_RESPONSE_FIELD_URL = "iothub-streaming-url";
static const char* DEVICE_STREAMING_RESPONSE_FIELD_AUTH_TOKEN = "iothub-streaming-auth-token";
static const char* DEVICE_STREAMING_RESPONSE_IS_ACCEPTED_TRUE = "True";
static const char* DEVICE_STREAMING_RESPONSE_IS_ACCEPTED_FALSE = "False";


typedef struct TEST_DEVICE_STREAMING_RESPONSE_TAG
{
    bool isAccepted;
    char* url;
    char* authorizationToken;
} TEST_DEVICE_STREAMING_RESPONSE;

typedef struct EXPECTED_DEVICE_STREAMING_REQUEST_TAG
{
    char* streamName;
    bool shouldAccept;
    unsigned int responseDelaySecs;
    DEVICE_STREAM_C2D_REQUEST* request;
    TEST_DEVICE_STREAMING_RESPONSE* response;
} EXPECTED_DEVICE_STREAMING_REQUEST;

typedef struct WEBSOCKET_CLIENT_CONTEXT_TAG
{
    bool isDeviceClient;
    bool isOpen;
    bool hadOpenError;
    bool hadSendError;
    bool isFaulty;
    bool remoteCloseReceived;
    bool localCloseConfirmed;
    char* dataSent;
    char* dataReceived;
    EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq;
} WEBSOCKET_CLIENT_CONTEXT;

typedef struct DEVICE_STREAMING_TEST_CONTEXT_TAG
{
    SINGLYLINKEDLIST_HANDLE expectedRequests;
    size_t numOfRequestsExpected;
    SINGLYLINKEDLIST_HANDLE requestsReceived;
    size_t numOfRequestsReceived;
} DEVICE_STREAMING_TEST_CONTEXT;


// ========== General Helpers ========== //

static int generate_new_int(void)
{
    int retValue;
    time_t nowTime = time(NULL);

    retValue = (int) nowTime;
    return retValue;
}

static char* generateUniqueString(void)
{
    char* result;

    if ((result = (char*)malloc(UID_LENGTH)) == NULL)
    {
        LogError("malloc failed");
    }
    else if (UniqueId_Generate(result, UID_LENGTH) != UNIQUEID_OK)
    {
        LogError("UniqueId_Generate failed");
        free(result);
        result = NULL;
    }

    return result;
}

void ds_e2e_init(bool testing_modules)
{
    int result = IoTHub_Init();
    ASSERT_ARE_EQUAL(int, 0, result, "IoTHub_Init failed");

    g_iothubAcctInfo = IoTHubAccount_Init(testing_modules);
    ASSERT_IS_NOT_NULL(g_iothubAcctInfo);
    /* the return value from the second init is deliberatly ignored. */
    (void)IoTHub_Init();
}

void ds_e2e_deinit(void)
{
    IoTHubAccount_deinit(g_iothubAcctInfo);

    // Need a double deinit
    IoTHub_Deinit();
    IoTHub_Deinit();
}


// ========== Service Client Helpers ========== //

static HTTPAPIEX_SAS_HANDLE createHttpExApiSasHandle()
{
    HTTPAPIEX_SAS_HANDLE result;
    STRING_HANDLE uriResource;

    if ((uriResource = STRING_construct(IoTHubAccount_GetIoTHubHostName(g_iothubAcctInfo))) == NULL)
    {
        LogError("STRING_construct failed for uriResource");
        result = NULL;
    }
    else
    {
        STRING_HANDLE accessKey;

        if ((accessKey = STRING_construct(IoTHubAccount_GetSharedAccessKey(g_iothubAcctInfo))) == NULL)
        {
            LogError("STRING_construct failed for accessKey");
            result = NULL;
        }
        else
        {
            STRING_HANDLE keyName;
        
            if ((keyName = STRING_construct(IoTHubAccount_GetSharedAccessKeyName(g_iothubAcctInfo))) == NULL)
            {
                LogError("STRING_construct failed for keyName");
                result = NULL;
            }
            else
            {
                if ((result = HTTPAPIEX_SAS_Create(accessKey, uriResource, keyName)) == NULL)
                {
                    LogError("HTTPAPIEX_SAS_Create failed");
                }

                STRING_delete(keyName);
            }

            STRING_delete(accessKey);
        }


        STRING_delete(uriResource);
    }

    return result;
}

static HTTP_HEADERS_HANDLE createHttpHeader()
{
    HTTP_HEADERS_HANDLE result;

    if ((result = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("Failed allocating HTTP headers");
    }
    else
    {
        if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_AUTHORIZATION, HTTP_HEADER_VAL_AUTHORIZATION) != HTTP_HEADERS_OK)
        {
            LogError("HTTPHeaders_AddHeaderNameValuePair failed for Authorization header");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else
        {
            char* guid;

            if ((guid = generateUniqueString()) == NULL)
            {
                LogError("Failed creating GUID");
                HTTPHeaders_Free(result);
                result = NULL;
            }
            else
            {
                if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_REQUEST_ID, (const char*)guid) != HTTP_HEADERS_OK)
                {
                    LogError("Failed adding HTTP request ID header");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }
                else if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_USER_AGENT, HTTP_HEADER_VAL_USER_AGENT) != HTTP_HEADERS_OK)
                {
                    LogError("Failed adding HTTP User Agent header");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }
                else if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_CONTENT_TYPE, HTTP_HEADER_VAL_CONTENT_TYPE) != HTTP_HEADERS_OK)
                {
                    LogError("Failed adding HTTP content type header");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }
                else if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_CONNECT_TIMEOUT_SECS, HTTP_HEADER_VAL_CONNECT_TIMEOUT_SECS) != HTTP_HEADERS_OK)
                {
                    LogError("Failed adding connect timeout header");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }
                else if (HTTPHeaders_AddHeaderNameValuePair(result, HTTP_HEADER_KEY_RESPONSE_TIMEOUT_SECS, HTTP_HEADER_VAL_RESPONSE_TIMEOUT_SECS) != HTTP_HEADERS_OK)
                {
                    LogError("Failed adding response header");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }


                free(guid);
            }
        }
    }

    return result;
}

static STRING_HANDLE createRelativePath(const char* deviceId, const char* moduleId, const char* streamName)
{
    STRING_HANDLE result;

    if (moduleId == NULL)
    {
        result = STRING_construct_sprintf(DEVICE_STREAMING_URI_RELATIVE_PATH_FMT, deviceId, streamName, URL_API_VERSION);
    }
    else
    {
        result = STRING_construct_sprintf(MODULE_STREAMING_URI_RELATIVE_PATH_FMT, deviceId, moduleId, streamName, URL_API_VERSION);
    }

    return result;
}

static char* findAndCloneHttpHeaderValue(HTTP_HEADERS_HANDLE headers, const char* name)
{
    char* result;
    const char* value;

    if ((value = HTTPHeaders_FindHeaderValue(headers, name)) == NULL)
    {
        LogError("Could not find HTTP header with name '%s'", name);
        result = NULL;
    }
    else if (mallocAndStrcpy_s(&result, value) != 0)
    {
        LogError("Failed cloning value of header '%s'", name);
        result = NULL;
    }

    return result;
}

static int parse_device_streaming_response(HTTP_HEADERS_HANDLE httpResponseHeaders,
    bool* isAccepted, char** url, char** authorizationToken)
{
    int result;
    const char* isAcceptedCharPtr;

    if ((isAcceptedCharPtr = HTTPHeaders_FindHeaderValue(httpResponseHeaders, DEVICE_STREAMING_RESPONSE_FIELD_IS_ACCEPTED)) == NULL)
    {
        LogError("Failed parsing device streaming response (%s)", DEVICE_STREAMING_RESPONSE_FIELD_IS_ACCEPTED);
        result = MU_FAILURE;
    }
    else
    {
        if (strcmp(isAcceptedCharPtr, DEVICE_STREAMING_RESPONSE_IS_ACCEPTED_TRUE) == 0)
        {
            *isAccepted = true;

            if ((*url = findAndCloneHttpHeaderValue(httpResponseHeaders, DEVICE_STREAMING_RESPONSE_FIELD_URL)) == NULL)
            {
                LogError("Failed parsing device streaming response (%s)", DEVICE_STREAMING_RESPONSE_FIELD_URL);
                result = MU_FAILURE;
            }
            else if ((*authorizationToken = findAndCloneHttpHeaderValue(httpResponseHeaders, DEVICE_STREAMING_RESPONSE_FIELD_AUTH_TOKEN)) == NULL)
            {
                LogError("Failed parsing device streaming response (%s)", DEVICE_STREAMING_RESPONSE_FIELD_AUTH_TOKEN);
                free(*url);
                *url = NULL;
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        else if (strcmp(isAcceptedCharPtr, DEVICE_STREAMING_RESPONSE_IS_ACCEPTED_FALSE) == 0)
        {
            *isAccepted = false;
            *url = NULL;
            *authorizationToken = NULL;
            result = 0;
        }
        else
        {
            LogError("Unexpected value for header '%s': %s", DEVICE_STREAMING_RESPONSE_FIELD_IS_ACCEPTED, isAcceptedCharPtr);
            result = MU_FAILURE;
        }
    }

    return result;
}

static int ds_e2e_send_device_streaming_request_async(const char* deviceId, const char* moduleId, EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq)
{
    int result = 0;

    HTTPAPIEX_HANDLE httpExApiHandle;

    if ((httpExApiHandle = HTTPAPIEX_Create(IoTHubAccount_GetIoTHubHostName(g_iothubAcctInfo))) == NULL)
    {
        LogError("Failed sending Device Streaming request");
        result = MU_FAILURE;
    }
    else
    {
        HTTPAPIEX_SAS_HANDLE httpExApiSasHandle;

        if ((httpExApiSasHandle = createHttpExApiSasHandle()) == NULL)
        {
            LogError("Failed creating the Http SAS handle");
            result = MU_FAILURE;
        }
        else
        {
            HTTP_HEADERS_HANDLE httpHeader;

            if ((httpHeader = createHttpHeader()) == NULL)
            {
                LogError("Failed creating headers for HTTP request");
                result = MU_FAILURE;
            }
            else
            {
                STRING_HANDLE relativePath;

                if ((relativePath = createRelativePath(deviceId, moduleId, expDSReq->streamName)) == NULL)
                {
                    LogError("Failure creating relative path");
                    result = MU_FAILURE;
                }
                else
                {
                    HTTP_HEADERS_HANDLE httpResponseHeaders;

                    if ((httpResponseHeaders = HTTPHeaders_Alloc()) == NULL)
                    {
                        LogError("Failure creating HTTP response headers");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        BUFFER_HANDLE responseBuffer;

                        if ((responseBuffer = BUFFER_new()) == NULL)
                        {
                            LogError("Failure creating buffer for HTTP response body");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            unsigned int statusCode = 0;

                            if (HTTPAPIEX_SAS_ExecuteRequest(httpExApiSasHandle, httpExApiHandle, HTTPAPI_REQUEST_POST, STRING_c_str(relativePath), httpHeader, NULL, &statusCode, httpResponseHeaders, responseBuffer) != HTTPAPIEX_OK)
                            {
                                LogError("Failed sending device streaming HTTP request");
                                result = MU_FAILURE;
                            }
                            else
                            {
                                if (statusCode == 200)
                                {
                                    bool isAccepted;
                                    char* url;
                                    char* authorizationToken;

                                    if (parse_device_streaming_response(httpResponseHeaders, &isAccepted, &url, &authorizationToken) != 0)
                                    {
                                        LogError("Failed parsing the device streaming response (%s)", expDSReq->streamName);
                                        result = MU_FAILURE;
                                    }
                                    else if ((expDSReq->response = (TEST_DEVICE_STREAMING_RESPONSE*)malloc(sizeof(TEST_DEVICE_STREAMING_RESPONSE))) == NULL)
                                    {
                                        LogError("Failed creating test device streaming response container (%s)", expDSReq->streamName);

                                        if (url != NULL)
                                        {
                                            free(url);
                                        }

                                        if (authorizationToken != NULL)
                                        {
                                            free(authorizationToken);
                                        }

                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        expDSReq->response->isAccepted = isAccepted;
                                        expDSReq->response->url = url;
                                        expDSReq->response->authorizationToken = authorizationToken;

                                        LogInfo("Service Client SGW url: %s", expDSReq->response->url);

                                        result = 0;
                                    }
                                }
                                else
                                {
                                    LogError("Received HTTP response code %d for device stream request", statusCode);
                                    result = MU_FAILURE;
                                }
                            }

                            BUFFER_delete(responseBuffer);
                        }

                        HTTPHeaders_Free(httpResponseHeaders);
                    }

                    STRING_delete(relativePath);
                }

                HTTPHeaders_Free(httpHeader);
            }

            HTTPAPIEX_SAS_Destroy(httpExApiSasHandle);
        }

        HTTPAPIEX_Destroy(httpExApiHandle);
    }

    return result;
}


// ========== Device Client Helpers ========== //

static void setoption_on_device_or_module(const char * optionName, const void * optionData, const char * errorMessage)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetOption(iothub_moduleclient_handle, optionName, optionData);
    }
    else
    {
        result = IoTHubDeviceClient_SetOption(iothub_deviceclient_handle, optionName, optionData);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, errorMessage);
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    (void)userContextCallback;
    LogInfo("[TEST] connection_status_callback: status=<%d>, reason=<%s>", status, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

static void setconnectionstatuscallback_on_device_or_module(DEVICE_STREAMING_TEST_CONTEXT* client_state)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetConnectionStatusCallback(iothub_moduleclient_handle, connection_status_callback, client_state);
    }
    else
    {
        result = IoTHubDeviceClient_SetConnectionStatusCallback(iothub_deviceclient_handle, connection_status_callback, client_state);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set connection Status Callback");
}

static void client_connect_to_hub(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, DEVICE_STREAMING_TEST_CONTEXT* device_client_state)
{
    ASSERT_IS_NULL(iothub_deviceclient_handle, "iothub_deviceclient_handle is non-NULL on test initialization");
    ASSERT_IS_NULL(iothub_moduleclient_handle, "iothub_moduleclient_handle is non-NULL on test initialization");

    if (deviceToUse->moduleConnectionString != NULL)
    {
        iothub_moduleclient_handle = IoTHubModuleClient_CreateFromConnectionString(deviceToUse->moduleConnectionString, protocol);
        ASSERT_IS_NOT_NULL(iothub_moduleclient_handle, "Could not invoke IoTHubModuleClient_CreateFromConnectionString");
    }
    else
    {
        iothub_deviceclient_handle = IoTHubDeviceClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
        ASSERT_IS_NOT_NULL(iothub_deviceclient_handle, "Could not invoke IoTHubDeviceClient_CreateFromConnectionString");
    }

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    setoption_on_device_or_module(OPTION_TRUSTED_CERT, certificates, "Cannot enable trusted cert");
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    // Set connection status change callback
    setconnectionstatuscallback_on_device_or_module(device_client_state);

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509)
    {
        setoption_on_device_or_module(OPTION_X509_CERT, deviceToUse->certificate, "Could not set the device x509 certificate");
        setoption_on_device_or_module(OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication, "Could not set the device x509 privateKey");
    }

    bool trace = true;
    setoption_on_device_or_module(OPTION_LOG_TRACE, &trace, "Cannot enable tracing");

    setoption_on_device_or_module(OPTION_PRODUCT_INFO, "MQTT_E2E/1.1.12", "Cannot set product info");

    //Turn on URL encoding/decoding (MQTT)
    if (false)
    {
        bool encodeDecode = true;
        setoption_on_device_or_module(OPTION_AUTO_URL_ENCODE_DECODE, &encodeDecode, "Cannot set auto_url_encode/decode");
    }
}

static void destroy_on_device_or_module()
{
    if (iothub_deviceclient_handle != NULL)
    {
        IoTHubDeviceClient_Destroy(iothub_deviceclient_handle);
        iothub_deviceclient_handle = NULL;
    }

    if (iothub_moduleclient_handle != NULL)
    {
        IoTHubModuleClient_Destroy(iothub_moduleclient_handle);
        iothub_moduleclient_handle = NULL;
    }
}

static char* get_me_a_new_stream_name()
{
    char* result;
    char* guid = generateUniqueString();
    ASSERT_IS_NOT_NULL(guid, "Failed generating stream name (no guid for you!)");

    result = (char*)malloc(STREAM_NAME_FMT_LENGTH + strlen(guid) + 1);
    ASSERT_IS_NOT_NULL(guid, "Failed generating stream name (malloc failed)");

    if (sprintf(result, STREAM_NAME_FMT, guid) <= 0)
    {
        free(result);
        ASSERT_FAIL("Failed generating stream name (sprintf failed)");
    }

    free(guid);
    
    return result;
}

static EXPECTED_DEVICE_STREAMING_REQUEST* add_expected_streaming_request(
    DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx, bool shouldAccept, unsigned int responseDelaySecs)
{
    EXPECTED_DEVICE_STREAMING_REQUEST* result;
    LIST_ITEM_HANDLE list_item;

    result = (EXPECTED_DEVICE_STREAMING_REQUEST*)malloc(sizeof(EXPECTED_DEVICE_STREAMING_REQUEST));
    ASSERT_IS_NOT_NULL(result, "Failed creating EXPECTED_DEVICE_STREAMING_REQUEST");

    result->streamName = get_me_a_new_stream_name();
    result->shouldAccept = shouldAccept;
    result->responseDelaySecs = responseDelaySecs;
    result->request = NULL;
    result->response = NULL;

    list_item = singlylinkedlist_add(dsTestCtx->expectedRequests, result);
    ASSERT_IS_NOT_NULL(list_item, "Failed adding expected device streaming request to list");

    dsTestCtx->numOfRequestsExpected++;

    return result;
}

static void destroy_expected_streaming_request(EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq)
{
    // expDSReq->request is destroyed in another function call.

    if (expDSReq->response != NULL)
    {
        free(expDSReq->response->url);
        free(expDSReq->response->authorizationToken);
        free(expDSReq->response);
    }

    free(expDSReq->streamName);
    free(expDSReq);
}

static DEVICE_STREAMING_TEST_CONTEXT* create_device_streaming_test_context()
{
    DEVICE_STREAMING_TEST_CONTEXT* result;

    result = (DEVICE_STREAMING_TEST_CONTEXT*)malloc(sizeof(DEVICE_STREAMING_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(result, "Failed creating device streaming test context");

    result->expectedRequests = singlylinkedlist_create();
    ASSERT_IS_NOT_NULL(result->expectedRequests, "Failed creating list for expected requests");
    
    result->requestsReceived = singlylinkedlist_create();
    ASSERT_IS_NOT_NULL(result->requestsReceived, "Failed creating list for incoming requests");

    result->numOfRequestsExpected = 0;
    result->numOfRequestsReceived = 0;

    return result;
}

static bool destroy_expected_streaming_request_in_list(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    destroy_expected_streaming_request((EXPECTED_DEVICE_STREAMING_REQUEST*)item);
    *continue_processing = true;
    return true;
}

static bool destroy_device_streaming_requests_in_list(const void* item, const void* match_context, bool* continue_processing)
{
    (void)match_context;
    stream_c2d_request_destroy((DEVICE_STREAM_C2D_REQUEST*)item);
    *continue_processing = true;
    return true;
}

static void destroy_device_streaming_test_context(DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx)
{
    (void)singlylinkedlist_remove_if(dsTestCtx->expectedRequests, destroy_expected_streaming_request_in_list, NULL);
    (void)singlylinkedlist_remove_if(dsTestCtx->requestsReceived, destroy_device_streaming_requests_in_list, NULL);
    singlylinkedlist_destroy(dsTestCtx->expectedRequests);
    singlylinkedlist_destroy(dsTestCtx->requestsReceived);
    free(dsTestCtx);
}

static bool find_expected_request_by_name(LIST_ITEM_HANDLE list_item, const void* match_context)
{
    const char* target_name = (const char*)match_context;
    EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq = (EXPECTED_DEVICE_STREAMING_REQUEST*)singlylinkedlist_item_get_value(list_item);

    return (target_name != NULL && expDSReq != NULL && (strcmp(expDSReq->streamName, target_name) == 0));
}

static DEVICE_STREAM_C2D_RESPONSE* on_device_streaming_request_received(DEVICE_STREAM_C2D_REQUEST* request, void* context)
{
    DEVICE_STREAM_C2D_RESPONSE* response;
    DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx = (DEVICE_STREAMING_TEST_CONTEXT*)context;
    EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq;
    bool shouldAccept;
    DEVICE_STREAM_C2D_REQUEST* copy_of_request;

    LIST_ITEM_HANDLE list_item = singlylinkedlist_find(dsTestCtx->expectedRequests, find_expected_request_by_name, request->name);

    if (list_item != NULL)
    {
        expDSReq = (EXPECTED_DEVICE_STREAMING_REQUEST*)singlylinkedlist_item_get_value(list_item);
        shouldAccept = (expDSReq != NULL && expDSReq->shouldAccept);

        if (expDSReq->responseDelaySecs > 0)
        {
            ThreadAPI_Sleep(expDSReq->responseDelaySecs * 1000);
        }
    }
    else
    {
        expDSReq = NULL;
        shouldAccept = false;
    }

    copy_of_request = stream_c2d_request_clone(request);
    ASSERT_IS_NOT_NULL(copy_of_request, "Failed cloning request");

    response = stream_c2d_response_create(request, shouldAccept);
    ASSERT_IS_NOT_NULL(response, "Failed creating device streaming response");

    ASSERT_IS_NOT_NULL(singlylinkedlist_add(dsTestCtx->requestsReceived, copy_of_request));
    dsTestCtx->numOfRequestsReceived++;

    if (expDSReq != NULL)
    {
        expDSReq->request = copy_of_request;
    }

    LogInfo("Device Client SGW url: %s", expDSReq->request->url);

    return response;
}

static void set_device_streaming_callback(DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx)
{
    IOTHUB_CLIENT_RESULT result;

    if (iothub_moduleclient_handle != NULL)
    {
        result = IoTHubModuleClient_SetStreamRequestCallback(iothub_moduleclient_handle, on_device_streaming_request_received, dsTestCtx);
    }
    else
    {
        result = IoTHubDeviceClient_SetStreamRequestCallback(iothub_deviceclient_handle, on_device_streaming_request_received, dsTestCtx);
    }

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "SetStreamRequestCallback failed");
}

static int verify_device_streaming_requests_received(DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx)
{
    int result;

    if (dsTestCtx->numOfRequestsReceived != dsTestCtx->numOfRequestsExpected)
    {
        LogError("[TEST] Expected %d stream requests, but got %d", (int)dsTestCtx->numOfRequestsExpected, (int)dsTestCtx->numOfRequestsReceived);
        result = MU_FAILURE;
    }
    else
    {
        LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(dsTestCtx->expectedRequests);

        result = 0;

        while (list_item != NULL)
        {
            EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq = (EXPECTED_DEVICE_STREAMING_REQUEST*)singlylinkedlist_item_get_value(list_item);

            if (expDSReq->request == NULL)
            {
                LogError("[TEST] Did not received request for stream %s", expDSReq->streamName);
                result = MU_FAILURE;
            }
            else if (expDSReq->response->isAccepted != expDSReq->shouldAccept)
            {
                LogError("[TEST] Unexpected response for stream request (%s, shouldAccept=%d, isAccepted=%d", expDSReq->streamName, (int)expDSReq->shouldAccept, (int)expDSReq->response->isAccepted);
                result = MU_FAILURE;
            }

            list_item = singlylinkedlist_get_next_item(list_item);
        }
    }

    return result;
}


// ========== WebSocket Helpers ========== //

static void on_ws_open_complete(void* context, WS_OPEN_RESULT ws_open_result)
{
    WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;

    if (ws_open_result == WS_OPEN_OK)
    {
        clientCtx->isOpen = true;
    }
    else
    {
        LogError("[TEST] on_ws_open_complete (%s, %d, %s)", clientCtx->expDSReq->streamName, (int)clientCtx->isDeviceClient, MU_ENUM_TO_STRING(WS_OPEN_RESULT, ws_open_result));
        clientCtx->hadOpenError = true;
    }
}

static void on_ws_send_frame_complete(void* context, WS_SEND_FRAME_RESULT ws_send_frame_result)
{
    if (ws_send_frame_result != WS_SEND_FRAME_OK && ws_send_frame_result != WS_SEND_FRAME_CANCELLED)
    {
        WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;
        LogError("[TEST] Failed sending stream data (%s, %d, %s)", clientCtx->expDSReq->streamName, (int)clientCtx->isDeviceClient, MU_ENUM_TO_STRING(WS_SEND_FRAME_RESULT, ws_send_frame_result));
        clientCtx->hadSendError = true;
    }
}

static char* cloneToNullTerminatedString(const unsigned char* string, size_t length)
{
    char* result;

    if ((result = (char*)malloc(length + 1)) == NULL)
    {
        LogError("[TEST] Failed allocating string");
    }
    else
    {
        (void)memcpy(result, string, length);
        result[length] = '\0';
    }

    return result;
}

static void on_ws_frame_received(void* context, unsigned char frame_type, const unsigned char* buffer, size_t size)
{
    (void)frame_type;

    WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;

    if (clientCtx->dataReceived != NULL)
    {
        free(clientCtx->dataReceived);
    }

    clientCtx->dataReceived = cloneToNullTerminatedString(buffer, size);

    ASSERT_IS_NOT_NULL(clientCtx->dataReceived, "Failed copying received data frame (%s; %d)", clientCtx->expDSReq->streamName, clientCtx->isDeviceClient);
}

static void on_ws_peer_closed(void* context, uint16_t* close_code, const unsigned char* extra_data, size_t extra_data_length)
{
    WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;
    LogInfo("[TEST] on_ws_peer_closed (%s; %d; Code=%d; Data=%.*s)", clientCtx->expDSReq->streamName, (int)clientCtx->isDeviceClient, *(int*)close_code, (int)extra_data_length, (const char*)extra_data);
    clientCtx->remoteCloseReceived = true;
}

static void on_ws_closed(void* context)
{
    WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;
    clientCtx->localCloseConfirmed = true;
}

static void on_ws_error(void* context, WS_ERROR error_code)
{
    WEBSOCKET_CLIENT_CONTEXT* clientCtx = (WEBSOCKET_CLIENT_CONTEXT*)context;
    LogError("[TEST] on_ws_error (%s, %d, %s)\r\n", clientCtx->expDSReq->streamName, (int)clientCtx->isDeviceClient, MU_ENUM_TO_STRING(WS_ERROR, error_code));
    clientCtx->isFaulty = true;
}

static int parse_streaming_gateway_url(char* url, char** hostAddress, size_t* port, char** resourceName)
{
    int result;
    WS_URL_HANDLE ws_url;

    if ((ws_url = ws_url_create(url)) == NULL)
    {
        LogError("[TEST] Failed creating WS url parser");
        result = MU_FAILURE;
    }
    else
    {
        const char* host_value;
        size_t host_length;

        if (ws_url_get_host(ws_url, &host_value, &host_length) != 0)
        {
            LogError("[TEST] Failed getting the host part of the WS url");
            result = MU_FAILURE;
        }
        else
        {
            const char* path_value;
            size_t path_length;

            if (ws_url_get_path(ws_url, &path_value, &path_length) != 0)
            {
                LogError("[TEST] Failed getting the path part of the WS url");
                result = MU_FAILURE;
            }
            else
            {
                if ((*hostAddress = cloneToNullTerminatedString((const unsigned char*)host_value, host_length)) == NULL)
                {
                    LogError("[TEST] Failed setting host address");
                    result = MU_FAILURE;
                }
                else if ((*resourceName = cloneToNullTerminatedString((const unsigned char*)(path_value - 1), path_length + 1)) == NULL)
                {
                    LogError("[TEST] Failed setting resource name");
                    free(*hostAddress);
                    *hostAddress = NULL;
                    result = MU_FAILURE;
                }
                else if (ws_url_get_port(ws_url, port) != 0)
                {
                    LogError("[TEST] Failed getting the port part of the WS url");
                    free(*hostAddress);
                    *hostAddress = NULL;
                    free(*resourceName);
                    *resourceName = NULL;
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }

        ws_url_destroy(ws_url);
    }

    return result;
}

static UWS_CLIENT_HANDLE create_websocket_client(char* url, char* authorizationToken, WEBSOCKET_CLIENT_CONTEXT* context)
{
    UWS_CLIENT_HANDLE result;
    char* hostAddress = NULL;
    size_t port = 0;
    char* resourceName = NULL;

    if (parse_streaming_gateway_url(url, &hostAddress, &port, &resourceName) != 0)
    {
        LogError("[TEST] Could not parse the ws url");
        result = NULL;
    }
    else
    {
        char auth_header_value[1024];
        WS_PROTOCOL protocols;

        protocols.protocol = "AMQP";

        if (sprintf(auth_header_value, "Bearer %s", authorizationToken) <= 0)
        {
            LogError("[TEST] Failed setting authorization header");
            result = NULL;
        }
        else if ((result = uws_client_create(hostAddress, (unsigned int)port, resourceName, true, &protocols, 1)) == NULL)
        {
            LogError("[TEST] Failed creating uws_client");
            result = NULL;
        }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        else if (uws_client_set_option(result, OPTION_TRUSTED_CERT, certificates) != 0)
        {
            LogError("[TEST] Failed setting trusted certificate on uws_client");
            uws_client_destroy(result);
            result = NULL;
        }
#endif
        else if (uws_client_set_request_header(result, "Authorization", auth_header_value) != 0)
        {
            LogError("[TEST] Failed setting authorization header in uws_client");
            uws_client_destroy(result);
            result = NULL;
        }
        else if (uws_client_open_async(result, on_ws_open_complete, context, on_ws_frame_received, context, on_ws_peer_closed, context, on_ws_error, context) != 0)
        {
            LogError("[TEST] Failed opening uws_client");
            uws_client_destroy(result);
            result = NULL;
        }

        free(hostAddress);
        free(resourceName);
    }

    return result;
}

static int verify_single_streaming_through_gateway(EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq)
{
    int result;
    WEBSOCKET_CLIENT_CONTEXT deviceClientCtx;
    UWS_CLIENT_HANDLE deviceWSClient;

    deviceClientCtx.isDeviceClient = true;
    deviceClientCtx.isOpen = false;
    deviceClientCtx.hadOpenError = false;
    deviceClientCtx.hadSendError = false;
    deviceClientCtx.isFaulty = false;
    deviceClientCtx.remoteCloseReceived = false;
    deviceClientCtx.localCloseConfirmed = false;
    deviceClientCtx.dataSent = NULL;
    deviceClientCtx.dataReceived = NULL;
    deviceClientCtx.expDSReq = expDSReq;

    if ((deviceWSClient = create_websocket_client(expDSReq->request->url, expDSReq->request->authorization_token, &deviceClientCtx)) == NULL)
    {
        LogError("[TEST] Failed creating/opening device websocket client (%s)", expDSReq->streamName);
        result = MU_FAILURE;
    }
    else
    {
        WEBSOCKET_CLIENT_CONTEXT serviceClientCtx;
        UWS_CLIENT_HANDLE serviceWSClient;

        serviceClientCtx.isDeviceClient = false;
        serviceClientCtx.isOpen = false;
        serviceClientCtx.hadOpenError = false;
        serviceClientCtx.hadSendError = false;
        serviceClientCtx.isFaulty = false;
        serviceClientCtx.remoteCloseReceived = false;
        serviceClientCtx.localCloseConfirmed = false;
        serviceClientCtx.dataSent = NULL;
        serviceClientCtx.dataReceived = NULL;
        serviceClientCtx.expDSReq = expDSReq;

        serviceWSClient = create_websocket_client(expDSReq->response->url, expDSReq->response->authorizationToken, &serviceClientCtx);

        if (serviceWSClient == NULL)
        {
            LogError("[TEST] Failed creating/opening service websocket client (%s)", expDSReq->streamName);
            result = MU_FAILURE;
        }
        else
        {
            result = 0;

            while (!serviceClientCtx.localCloseConfirmed && !serviceClientCtx.remoteCloseReceived && 
                !deviceClientCtx.localCloseConfirmed && !deviceClientCtx.remoteCloseReceived)
            {
                if (serviceClientCtx.hadOpenError || serviceClientCtx.isFaulty || serviceClientCtx.hadSendError)
                {
                    LogError("[TEST] WS client is faulty (service client; %s)", expDSReq->streamName);
                    result = MU_FAILURE;
                    break;
                }
                else if (serviceClientCtx.isOpen)
                {
                    if (serviceClientCtx.dataSent == NULL)
                    {
                        const unsigned char* buffer = (const unsigned char*)serviceClientCtx.expDSReq->streamName;
                        size_t size = strlen(serviceClientCtx.expDSReq->streamName);

                        serviceClientCtx.dataSent = serviceClientCtx.expDSReq->streamName;
                        
                        if (uws_client_send_frame_async(serviceWSClient, 2, buffer, size, true, on_ws_send_frame_complete, &serviceClientCtx) != 0)
                        {
                            LogError("[TEST] Failed sending data to streaming gateway (service client; %s)", expDSReq->streamName);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                    else if (serviceClientCtx.dataReceived != NULL)
                    {
                        serviceClientCtx.isOpen = false;
                        if (uws_client_close_async(serviceWSClient, on_ws_closed, &serviceClientCtx) != 0)
                        {
                            LogError("[TEST] Failed closing connection with streaming gateway (service client; %s)", expDSReq->streamName);
                            result = MU_FAILURE;
                            break;
                        }

                        deviceClientCtx.isOpen = false;
                        if (uws_client_close_async(deviceWSClient, on_ws_closed, &deviceClientCtx) != 0)
                        {
                            LogError("[TEST] Failed closing connection with streaming gateway (device client; %s)", expDSReq->streamName);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }

                if (deviceClientCtx.hadOpenError || deviceClientCtx.isFaulty || deviceClientCtx.hadSendError)
                {
                    LogError("[TEST] WS client is faulty (device client; %s)", expDSReq->streamName);
                    result = MU_FAILURE;
                    break;
                }
                else if (deviceClientCtx.isOpen)
                {
                    if (deviceClientCtx.dataReceived != NULL)
                    {
                        const unsigned char* buffer = (const unsigned char*)deviceClientCtx.dataReceived;
                        size_t size = strlen(deviceClientCtx.dataReceived);

                        if (uws_client_send_frame_async(deviceWSClient, 2, buffer, size, true, on_ws_send_frame_complete, &deviceClientCtx) != 0)
                        {
                            LogError("[TEST] Failed sending data to streaming gateway (device client; %s)", expDSReq->streamName);
                            result = MU_FAILURE;
                            break;
                        }
                    }
                }

                uws_client_dowork(deviceWSClient);
                uws_client_dowork(serviceWSClient);
            }

            uws_client_destroy(serviceWSClient);
        }

        uws_client_destroy(deviceWSClient);

        if (result == 0)
        {
            if (strcmp(serviceClientCtx.dataSent, serviceClientCtx.dataReceived) != 0)
            {
                LogError("[TEST] Data sent and received by service WS client do not match ('%s'; '%s')",
                    serviceClientCtx.dataSent, serviceClientCtx.dataReceived);
                result = MU_FAILURE;
            }
        }

        if (deviceClientCtx.dataReceived != NULL)
        {
            free(deviceClientCtx.dataReceived);
        }
        if (serviceClientCtx.dataReceived != NULL)
        {
            free(serviceClientCtx.dataReceived);
        }
    }

    return result;
}

static int verify_streaming_through_gateway(DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx)
{
    int result;

    if (dsTestCtx->numOfRequestsExpected == 0)
    {
        result = 0;
    }
    else
    {
        LIST_ITEM_HANDLE list_item = singlylinkedlist_get_head_item(dsTestCtx->expectedRequests);

        if (list_item == NULL)
        {
            LogError("[TEST] Failed getting expected request from list");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;

            while (list_item != NULL)
            {
                EXPECTED_DEVICE_STREAMING_REQUEST* expDSReq = (EXPECTED_DEVICE_STREAMING_REQUEST*)singlylinkedlist_item_get_value(list_item);

                if (expDSReq == NULL)
                {
                    LogError("[TEST] Failed getting expected list item value");
                    result = MU_FAILURE;
                    break;
                }
                else if (expDSReq->response->isAccepted)
                {
                    if (verify_single_streaming_through_gateway(expDSReq) != 0)
                    {
                        result = MU_FAILURE;
                    }
                }

                list_item = singlylinkedlist_get_next_item(list_item);
            }
        }
    }

    return result;
}


// ========== Public API ========== //

#define MAX_SEND_REQUEST_COUNT 10

static void receive_device_streaming_request_test(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool shouldAcceptRequest)
{
    // arrange
    int verify_request_result;
    int streaming_result;
    int send_request_counter = 0;
    DEVICE_STREAMING_TEST_CONTEXT* dsTestCtx = create_device_streaming_test_context();

    // Create the IoT Hub Data
    client_connect_to_hub(deviceToUse, protocol, dsTestCtx);

    set_device_streaming_callback(dsTestCtx);

    EXPECTED_DEVICE_STREAMING_REQUEST* request = add_expected_streaming_request(dsTestCtx, shouldAcceptRequest, 0);

    while (true)
    {
        if (ds_e2e_send_device_streaming_request_async(deviceToUse->deviceId, deviceToUse->moduleId, request) == 0)
        {
            break;
        }
        else if (send_request_counter++ < MAX_SEND_REQUEST_COUNT)
        {
            ThreadAPI_Sleep(500);
        }
        else
        {
            ASSERT_FAIL("Timed-out sending device streaming request");
        }
    }

    verify_request_result = verify_device_streaming_requests_received(dsTestCtx);
    ASSERT_ARE_EQUAL(int, 0, verify_request_result);

    streaming_result = verify_streaming_through_gateway(dsTestCtx);
    ASSERT_ARE_EQUAL(int, 0, streaming_result);

    // close the client connection
    destroy_on_device_or_module();
    
    destroy_device_streaming_test_context(dsTestCtx);
}

void ds_e2e_receive_device_streaming_request(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool shouldAcceptRequest)
{
    receive_device_streaming_request_test(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, shouldAcceptRequest);
}

void ds_e2e_receive_module_streaming_request(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool shouldAcceptRequest)
{
    receive_device_streaming_request_test(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol, shouldAcceptRequest);
}
