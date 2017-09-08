// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef DONT_USE_UPLOADTOBLOB
#error "trying to compile iothub_client_ll_uploadtoblob.c while the symbol DONT_USE_UPLOADTOBLOB is #define'd"
#else

#include <stdlib.h>
#include <string.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/lock.h"

#include "iothub_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_client_private.h"
#include "iothub_client_version.h"
#include "iothub_transport_ll.h"
#include "parson.h"
#include "iothub_client_ll_uploadtoblob.h"
#include "blob.h"


#ifdef WINCE
#include <stdarg.h>
// Returns number of characters copied.
int snprintf(char * s, size_t n, const char * format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsnprintf(s, n, format, args);
    va_end(args);
    return result;
}
#endif


/*Codes_SRS_IOTHUBCLIENT_LL_02_085: [ IoTHubClient_LL_UploadToBlob shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters: ]*/
#define FILE_UPLOAD_FAILED_BODY "{ \"isSuccess\":false, \"statusCode\":-1,\"statusDescription\" : \"client not able to connect with the server\" }"

#define AUTHORIZATION_SCHEME_VALUES \
    DEVICE_KEY, \
    X509,       \
    SAS_TOKEN
DEFINE_ENUM(AUTHORIZATION_SCHEME, AUTHORIZATION_SCHEME_VALUES);

typedef struct UPLOADTOBLOB_X509_CREDENTIALS_TAG
{
    const char* x509certificate;
    const char* x509privatekey;
}UPLOADTOBLOB_X509_CREDENTIALS;

typedef struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA_TAG
{
    STRING_HANDLE deviceId;                     /*needed for file upload*/
    const char* hostname;                       /*needed for file upload*/
    AUTHORIZATION_SCHEME authorizationScheme;   /*needed for file upload*/
    union {
        STRING_HANDLE deviceKey;    /*used when authorizationScheme is DEVICE_KEY*/
        STRING_HANDLE sas;          /*used when authorizationScheme is SAS_TOKEN*/
        UPLOADTOBLOB_X509_CREDENTIALS x509credentials; /*assumed to be used when both deviceKey and deviceSasToken are NULL*/
    } credentials;                              /*needed for file upload*/
    char* certificates; /*if there are any certificates used*/
    HTTP_PROXY_OPTIONS http_proxy_options;
}IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA;

IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE IoTHubClient_LL_UploadToBlob_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData = malloc(sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA));
    if (handleData == NULL)
    {
        LogError("oom - malloc");
        /*return as is*/
    }
    else
    {
        size_t iotHubNameLength = strlen(config->iotHubName);
        size_t iotHubSuffixLength = strlen(config->iotHubSuffix);
        handleData->deviceId = STRING_construct(config->deviceId);
        if (handleData->deviceId == NULL)
        {
            LogError("unable to STRING_construct");
            free(handleData);
            handleData = NULL;
        }
        else
        {
            handleData->hostname = malloc(iotHubNameLength + 1 + iotHubSuffixLength + 1); /*first +1 is because "." the second +1 is because \0*/
            if (handleData->hostname == NULL)
            {
                LogError("malloc failed");
                STRING_delete(handleData->deviceId);
                free(handleData);
                handleData = NULL;
            }
            else
            {
                (void)memcpy((char*)handleData->hostname, config->iotHubName, iotHubNameLength);
                ((char*)handleData->hostname)[iotHubNameLength] = '.';
                (void)memcpy((char*)handleData->hostname + iotHubNameLength + 1, config->iotHubSuffix, iotHubSuffixLength + 1); /*+1 will copy the \0 too*/
                handleData->certificates = NULL;
                memset(&(handleData->http_proxy_options), 0, sizeof(HTTP_PROXY_OPTIONS));

                if ((config->deviceSasToken != NULL) && (config->deviceKey == NULL))
                {
                    handleData->authorizationScheme = SAS_TOKEN;
                    handleData->credentials.sas = STRING_construct(config->deviceSasToken);
                    if (handleData->credentials.sas == NULL)
                    {
                        LogError("unable to STRING_construct");
                        free((void*)handleData->hostname);
                        STRING_delete(handleData->deviceId);
                        free(handleData);
                        handleData = NULL;
                    }
                    else
                    {
                        /*return as is*/
                    }
                }
                else if ((config->deviceSasToken == NULL) && (config->deviceKey != NULL))
                {
                    handleData->authorizationScheme = DEVICE_KEY;
                    handleData->credentials.deviceKey = STRING_construct(config->deviceKey);
                    if (handleData->credentials.deviceKey == NULL)
                    {
                        LogError("unable to STRING_construct");
                        free((void*)handleData->hostname);
                        STRING_delete(handleData->deviceId);
                        free(handleData);
                        handleData = NULL;
                    }
                    else
                    {
                        /*return as is*/
                    }
                }
                else if ((config->deviceSasToken == NULL) && (config->deviceKey == NULL))
                {
                    handleData->authorizationScheme = X509;
                    handleData->credentials.x509credentials.x509certificate = NULL;
                    handleData->credentials.x509credentials.x509privatekey = NULL;
                    /*return as is*/
                }
            }
        }
    }
    return (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)handleData;
    
}

/*returns 0 when correlationId, sasUri contain data*/
static int IoTHubClient_LL_UploadToBlob_step1and2(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData, HTTPAPIEX_HANDLE iotHubHttpApiExHandle, HTTP_HEADERS_HANDLE requestHttpHeaders, const char* destinationFileName,
    STRING_HANDLE correlationId, STRING_HANDLE sasUri)
{
    int result;

    /*Codes_SRS_IOTHUBCLIENT_LL_02_066: [ IoTHubClient_LL_UploadToBlob shall create an HTTP relative path formed from "/devices/" + deviceId + "/files/" + "?api-version=API_VERSION". ]*/
    STRING_HANDLE relativePath = STRING_construct("/devices/");
    if (relativePath == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_067: [ If creating the relativePath fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("unable to STRING_construct");
        result = __FAILURE__;
    }
    else
    {
        if (!(
            (STRING_concat_with_STRING(relativePath, handleData->deviceId) == 0) &&
            (STRING_concat(relativePath, "/files") == 0) &&
            (STRING_concat(relativePath, API_VERSION) == 0)
            ))
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_067: [ If creating the relativePath fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
            LogError("unable to concatenate STRING");
            result = __FAILURE__;
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_32_001: [ IoTHubClient_LL_UploadToBlob shall create a JSON string formed from "{ \"blobName\": \" + destinationFileName + "\" }" */
            STRING_HANDLE blobName = STRING_construct("{ \"blobName\": \"");
            if (blobName == NULL)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_32_002: [ If creating the JSON string fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                LogError("unable to STRING_construct");
                result = __FAILURE__;
            }
            else
            {
                if (!(
                    (STRING_concat(blobName, destinationFileName) == 0) &&
                    (STRING_concat(blobName, "\" }") == 0)
                    ))
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_32_002: [ If creating the JSON string fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("unable to concatenate STRING");
                    result = __FAILURE__;
                }
                else
                {
                    size_t len = STRING_length(blobName);
                    BUFFER_HANDLE blobBuffer = BUFFER_create((const unsigned char *)STRING_c_str(blobName), len);

                    if (blobBuffer == NULL)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_32_002: [ If creating the JSON string fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                        LogError("unable to create BUFFER");
                        result = __FAILURE__;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_068: [ IoTHubClient_LL_UploadToBlob shall create an HTTP responseContent BUFFER_HANDLE. ]*/
                        BUFFER_HANDLE responseContent = BUFFER_new();
                        if (responseContent == NULL)
                        {
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_069: [ If creating the HTTP response buffer handle fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                            result = __FAILURE__;
                            LogError("unable to BUFFER_new");
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_072: [ IoTHubClient_LL_UploadToBlob shall add the following name:value to request HTTP headers: ] "Content-Type": "application/json" "Accept": "application/json" "User-Agent": "iothubclient/" IOTHUB_SDK_VERSION*/
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_107: [ - "Authorization" header shall not be build. ]*/
                            if (!(
                                (HTTPHeaders_AddHeaderNameValuePair(requestHttpHeaders, "Content-Type", "application/json") == HTTP_HEADERS_OK) &&
                                (HTTPHeaders_AddHeaderNameValuePair(requestHttpHeaders, "Accept", "application/json") == HTTP_HEADERS_OK) &&
                                (HTTPHeaders_AddHeaderNameValuePair(requestHttpHeaders, "User-Agent", "iothubclient/" IOTHUB_SDK_VERSION) == HTTP_HEADERS_OK) &&
                                (handleData->authorizationScheme == X509 || (HTTPHeaders_AddHeaderNameValuePair(requestHttpHeaders, "Authorization", "") == HTTP_HEADERS_OK))
                                ))
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_071: [ If creating the HTTP headers fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                LogError("unable to HTTPHeaders_AddHeaderNameValuePair");
                                result = __FAILURE__;
                            }
                            else
                            {
                                int wasIoTHubRequestSuccess = 0; /*!=0 means responseContent has a buffer that should be parsed by parson after executing the below switch*/
                                /* set the result to error by default */
                                result = __FAILURE__;
                                switch (handleData->authorizationScheme)
                                {
                                default:
                                {
                                    /*wasIoTHubRequestSuccess takes care of the return value*/
                                    LogError("Internal Error: unexpected value in handleData->authorizationScheme = %d", handleData->authorizationScheme);
                                    result = __FAILURE__;
                                    break;
                                }
                                case(X509):
                                {
                                    unsigned int statusCode;
                                    /*Codes_SRS_IOTHUBCLIENT_LL_32_003: [ IoTHubClient_LL_UploadToBlob shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments: ]*/
                                    if (HTTPAPIEX_ExecuteRequest(
                                        iotHubHttpApiExHandle,          /*HTTPAPIEX_HANDLE handle - the handle created at the beginning of `IoTHubClient_LL_UploadToBlob`*/
                                        HTTPAPI_REQUEST_POST,           /*HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST*/
                                        STRING_c_str(relativePath),     /*const char* relativePath - the HTTP relative path*/
                                        requestHttpHeaders,             /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers*/
                                        blobBuffer,                     /*BUFFER_HANDLE requestContent - address of JSON with optional/directory/tree/filename*/
                                        &statusCode,                    /*unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code*/
                                        NULL,                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle - NULL*/
                                        responseContent                 /*BUFFER_HANDLE responseContent - the HTTP response BUFFER_HANDLE - responseContent*/
                                    ) != HTTPAPIEX_OK)
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_076: [ If HTTPAPIEX_ExecuteRequest call fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                        result = __FAILURE__;
                                        LogError("unable to HTTPAPIEX_ExecuteRequest");
                                    }
                                    else
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_077: [ If HTTP statusCode is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                        if (statusCode >= 300)
                                        {
                                            result = __FAILURE__;
                                            LogError("HTTP code was %u", statusCode);
                                        }
                                        else
                                        {
                                            wasIoTHubRequestSuccess = 1;
                                        }
                                    }
                                    break;
                                }
                                case (SAS_TOKEN):
                                {
                                    const char* sasToken = STRING_c_str(handleData->credentials.sas);
                                    /*Codes_SRS_IOTHUBCLIENT_LL_02_073: [ If the credentials used to create handle have "sasToken" then IoTHubClient_LL_UploadToBlob shall add the following HTTP request headers: ]*/
                                    if (HTTPHeaders_ReplaceHeaderNameValuePair(requestHttpHeaders, "Authorization", sasToken) != HTTP_HEADERS_OK)
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_074: [ If adding "Authorization" fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR ]*/
                                        result = __FAILURE__;
                                        LogError("unable to HTTPHeaders_AddHeaderNameValuePair");
                                    }
                                    else
                                    {
                                        unsigned int statusCode;
                                        /*Codes_SRS_IOTHUBCLIENT_LL_32_004: [ IoTHubClient_LL_UploadToBlob shall execute HTTPAPIEX_ExecuteRequest passing the following information for arguments: ]*/
                                        if (HTTPAPIEX_ExecuteRequest(
                                            iotHubHttpApiExHandle,          /*HTTPAPIEX_HANDLE handle - the handle created at the beginning of `IoTHubClient_LL_UploadToBlob`*/
                                            HTTPAPI_REQUEST_POST,           /*HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST*/
                                            STRING_c_str(relativePath),     /*const char* relativePath - the HTTP relative path*/
                                            requestHttpHeaders,             /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers*/
                                            blobBuffer,                     /*BUFFER_HANDLE requestContent - address of JSON with optional/directory/tree/filename*/
                                            &statusCode,                    /*unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code*/
                                            NULL,                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle - NULL*/
                                            responseContent                 /*BUFFER_HANDLE responseContent - the HTTP response BUFFER_HANDLE - responseContent*/
                                        ) != HTTPAPIEX_OK)
                                        {
                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_076: [ If HTTPAPIEX_ExecuteRequest call fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                            result = __FAILURE__;
                                            LogError("unable to HTTPAPIEX_ExecuteRequest");
                                        }
                                        else
                                        {
                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_077: [ If HTTP statusCode is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                            if (statusCode >= 300)
                                            {
                                                result = __FAILURE__;
                                                LogError("HTTP code was %u", statusCode);
                                            }
                                            else
                                            {
                                                wasIoTHubRequestSuccess = 1;
                                            }
                                        }
                                    }
                                    break;
                                }
                                case(DEVICE_KEY):
                                {
                                    /*Codes_SRS_IOTHUBCLIENT_LL_02_078: [ If the credentials used to create handle have "deviceKey" then IoTHubClient_LL_UploadToBlob shall create an HTTPAPIEX_SAS_HANDLE passing as arguments: ]*/
                                    STRING_HANDLE uriResource = STRING_construct(handleData->hostname);
                                    if (uriResource == NULL)
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_089: [ If creating the HTTPAPIEX_SAS_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                        result = __FAILURE__;
                                        LogError("unable to STRING_construct");
                                    }
                                    else
                                    {
                                        if (!(
                                            (STRING_concat(uriResource, "/devices/") == 0) &&
                                            (STRING_concat_with_STRING(uriResource, handleData->deviceId) == 0)
                                            ))
                                        {
                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_089: [ If creating the HTTPAPIEX_SAS_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                            LogError("unable to STRING_concat_with_STRING");
                                            result = __FAILURE__;
                                        }
                                        else
                                        {
                                            STRING_HANDLE empty = STRING_new();
                                            if (empty == NULL)
                                            {
                                                LogError("unable to STRING_new");
                                                result = __FAILURE__;
                                            }
                                            else
                                            {
                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_089: [ If creating the HTTPAPIEX_SAS_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                HTTPAPIEX_SAS_HANDLE sasHandle = HTTPAPIEX_SAS_Create(handleData->credentials.deviceKey, uriResource, empty);
                                                if (sasHandle == NULL)
                                                {
                                                    LogError("unable to HTTPAPIEX_SAS_Create");
                                                    result = __FAILURE__;
                                                }
                                                else
                                                {
                                                    unsigned int statusCode;
                                                    /*Codes_SRS_IOTHUBCLIENT_LL_32_005: [ IoTHubClient_LL_UploadToBlob shall call HTTPAPIEX_SAS_ExecuteRequest passing as arguments: ]*/
                                                    if (HTTPAPIEX_SAS_ExecuteRequest(
                                                        sasHandle,                      /*HTTPAPIEX_SAS_HANDLE sasHandle - the created HTTPAPIEX_SAS_HANDLE*/
                                                        iotHubHttpApiExHandle,          /*HTTPAPIEX_HANDLE handle - the created HTTPAPIEX_HANDLE*/
                                                        HTTPAPI_REQUEST_POST,           /*HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_POST*/
                                                        STRING_c_str(relativePath),     /*const char* relativePath - the HTTP relative path*/
                                                        requestHttpHeaders,             /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers*/
                                                        blobBuffer,                     /*BUFFER_HANDLE requestContent - address of JSON with optional/directory/tree/filename*/
                                                        &statusCode,                    /*unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code*/
                                                        NULL,                           /*HTTP_HEADERS_HANDLE responseHeadersHandle - NULL*/
                                                        responseContent
                                                    ) != HTTPAPIEX_OK)
                                                    {
                                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_079: [ If HTTPAPIEX_SAS_ExecuteRequest fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                        LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                                                        result = __FAILURE__;
                                                    }
                                                    else
                                                    {
                                                        if (statusCode >= 300)
                                                        {
                                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_080: [ If status code is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                            result = __FAILURE__;
                                                            LogError("HTTP code was %u", statusCode);
                                                        }
                                                        else
                                                        {
                                                            wasIoTHubRequestSuccess = 1;
                                                        }
                                                    }
                                                    HTTPAPIEX_SAS_Destroy(sasHandle);
                                                }
                                                STRING_delete(empty);
                                            }
                                        }
                                        STRING_delete(uriResource);
                                    }
                                }
                                } /*switch*/

                                if (wasIoTHubRequestSuccess == 0)
                                {
                                    /*do nothing, shall be reported as an error*/
                                }
                                else
                                {
                                    const unsigned char*responseContent_u_char = BUFFER_u_char(responseContent);
                                    size_t responseContent_length = BUFFER_length(responseContent);
                                    STRING_HANDLE responseAsString = STRING_from_byte_array(responseContent_u_char, responseContent_length);
                                    if (responseAsString == NULL)
                                    {
                                        result = __FAILURE__;
                                        LogError("unable to get the response as string");
                                    }
                                    else
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_081: [ Otherwise, IoTHubClient_LL_UploadToBlob shall use parson to extract and save the following information from the response buffer: correlationID and SasUri. ]*/
                                        JSON_Value* allJson = json_parse_string(STRING_c_str(responseAsString));
                                        if (allJson == NULL)
                                        {
                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                            LogError("unable to json_parse_string");
                                            result = __FAILURE__;
                                        }
                                        else
                                        {
                                            JSON_Object* jsonObject = json_value_get_object(allJson);
                                            if (jsonObject == NULL)
                                            {
                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                LogError("unable to json_value_get_object");
                                                result = __FAILURE__;
                                            }
                                            else
                                            {
                                                const char* json_correlationId;
                                                json_correlationId = json_object_get_string(jsonObject, "correlationId");
                                                if (json_correlationId == NULL)
                                                {
                                                    /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                    LogError("unable to json_object_get_string(jsonObject, \"correlationId\")");
                                                    result = __FAILURE__;
                                                }
                                                else
                                                {
                                                    if (STRING_copy(correlationId, json_correlationId) != 0)
                                                    {
                                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                        LogError("unable to copy json_correlationId");
                                                        result = __FAILURE__;
                                                    }
                                                    else
                                                    {
                                                        const char* json_hostName = json_object_get_string(jsonObject, "hostName");
                                                        if (json_hostName == NULL)
                                                        {
                                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                            LogError("unable to json_object_get_string(jsonObject, \"hostName\")");
                                                            result = __FAILURE__;
                                                        }
                                                        else
                                                        {
                                                            const char* json_containerName = json_object_get_string(jsonObject, "containerName");
                                                            if (json_containerName == NULL)
                                                            {
                                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                                LogError("unable to json_object_get_string(jsonObject, \"containerName\")");
                                                                result = __FAILURE__;
                                                            }
                                                            else
                                                            {
                                                                const char* json_blobName = json_object_get_string(jsonObject, "blobName");
                                                                if (json_blobName == NULL)
                                                                {
                                                                    /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                                    LogError("unable to json_object_get_string(jsonObject, \"blobName\")");
                                                                    result = __FAILURE__;
                                                                }
                                                                else
                                                                {
                                                                    const char* json_sasToken = json_object_get_string(jsonObject, "sasToken");
                                                                    if (json_sasToken == NULL)
                                                                    {
                                                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                                        LogError("unable to json_object_get_string(jsonObject, \"sasToken\")");
                                                                        result = __FAILURE__;
                                                                    }
                                                                    else
                                                                    {
                                                                        /*good JSON received from the service*/

                                                                        if (STRING_copy(sasUri, "https://") != 0)
                                                                        {
                                                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                                            LogError("unable to STRING_copy");
                                                                            result = __FAILURE__;
                                                                        }
                                                                        else
                                                                        {
                                                                            if (!(
                                                                                (STRING_concat(sasUri, json_hostName) == 0) &&
                                                                                (STRING_concat(sasUri, "/") == 0) &&
                                                                                (STRING_concat(sasUri, json_containerName) == 0) &&
                                                                                (STRING_concat(sasUri, "/") == 0) &&
                                                                                (STRING_concat(sasUri, json_blobName) == 0) &&
                                                                                (STRING_concat(sasUri, json_sasToken) == 0)
                                                                                ))
                                                                            {
                                                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                                                LogError("unable to STRING_concat");
                                                                                result = __FAILURE__;
                                                                            }
                                                                            else
                                                                            {
                                                                                result = 0; /*success in step 1*/
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            json_value_free(allJson);
                                        }
                                        STRING_delete(responseAsString);
                                    }
                                }
                            }
                            BUFFER_delete(responseContent);
                        }
                        BUFFER_delete(blobBuffer);
                    }
                }
                STRING_delete(blobName);
            }
        }
        STRING_delete(relativePath);
    }
    return result;
}

/*returns 0 when the IoTHub has been informed about the file upload status*/
static int IoTHubClient_LL_UploadToBlob_step3(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData, STRING_HANDLE correlationId, HTTPAPIEX_HANDLE iotHubHttpApiExHandle, HTTP_HEADERS_HANDLE requestHttpHeaders, BUFFER_HANDLE messageBody)
{
    int result;
    /*here is step 3. depending on the outcome of step 2 it needs to inform IoTHub about the file upload status*/
    /*if step 1 failed, there's nothing that step 3 needs to report.*/
    /*this POST "tries" to happen*/

    /*Codes_SRS_IOTHUBCLIENT_LL_02_085: [ IoTHubClient_LL_UploadToBlob shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters: ]*/
    STRING_HANDLE uriResource = STRING_construct(handleData->hostname);
    if (uriResource == NULL)
    {
        LogError("unable to construct URI");
        result = __FAILURE__;
    }
    else
    {
        if (!(
            (STRING_concat(uriResource, "/devices/") == 0) &&
            (STRING_concat_with_STRING(uriResource, handleData->deviceId) == 0) &&
            (STRING_concat(uriResource, "/files/notifications") == 0)
            ))
        {
            LogError("unable to STRING_concat");
            result = __FAILURE__;
        }
        else
        {
            STRING_HANDLE relativePathNotification = STRING_construct("/devices/");
            if (relativePathNotification == NULL)
            {
                result = __FAILURE__;
                LogError("unable to STRING_construct");
            }
            else
            {
                if (!(
                    (STRING_concat_with_STRING(relativePathNotification, handleData->deviceId) == 0) &&
                    (STRING_concat(relativePathNotification, "/files/notifications/") == 0) &&
                    (STRING_concat(relativePathNotification, STRING_c_str(correlationId)) == 0) &&
                    (STRING_concat(relativePathNotification, API_VERSION) == 0)
                    ))
                {
                    LogError("unable to STRING_concat_with_STRING");
                    result = __FAILURE__;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_086: [ If performing the HTTP request fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    switch (handleData->authorizationScheme)
                    {
                    default:
                    {
                        LogError("internal error: unknown authorization Scheme");
                        result = __FAILURE__;
                        break;
                    }
                    case (X509):
                    {
                        unsigned int notificationStatusCode;
                        if (HTTPAPIEX_ExecuteRequest(
                            iotHubHttpApiExHandle,
                            HTTPAPI_REQUEST_POST,
                            STRING_c_str(relativePathNotification),
                            requestHttpHeaders,
                            messageBody,
                            &notificationStatusCode,
                            NULL,
                            NULL) != HTTPAPIEX_OK)
                        {
                            LogError("unable to do HTTPAPIEX_ExecuteRequest");
                            result = __FAILURE__;
                        }
                        else
                        {
                            if (notificationStatusCode >= 300)
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_087: [If the statusCode of the HTTP request is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR]*/
                                LogError("server didn't like the notification request");
                                result = __FAILURE__;
                            }
                            else
                            {
                                result = 0;
                            }
                        }
                        break;
                    }
                    case (DEVICE_KEY):
                    {
                        STRING_HANDLE empty = STRING_new();
                        if (empty == NULL)
                        {
                            LogError("unable to STRING_new");
                            result = __FAILURE__;
                        }
                        else
                        {
                            HTTPAPIEX_SAS_HANDLE sasHandle = HTTPAPIEX_SAS_Create(handleData->credentials.deviceKey, uriResource, empty);
                            if (sasHandle == NULL)
                            {
                                LogError("unable to HTTPAPIEX_SAS_Create");
                                result = __FAILURE__;
                            }
                            else
                            {
                                unsigned int statusCode;
                                if (HTTPAPIEX_SAS_ExecuteRequest(
                                    sasHandle,                      /*HTTPAPIEX_SAS_HANDLE sasHandle - the created HTTPAPIEX_SAS_HANDLE*/
                                    iotHubHttpApiExHandle,          /*HTTPAPIEX_HANDLE handle - the created HTTPAPIEX_HANDLE*/
                                    HTTPAPI_REQUEST_POST,            /*HTTPAPI_REQUEST_TYPE requestType - HTTPAPI_REQUEST_GET*/
                                    STRING_c_str(relativePathNotification),     /*const char* relativePath - the HTTP relative path*/
                                    requestHttpHeaders,             /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle - request HTTP headers*/
                                    messageBody,                    /*BUFFER_HANDLE requestContent*/
                                    &statusCode,                    /*unsigned int* statusCode - the address of an unsigned int that will contain the HTTP status code*/
                                    NULL,                           /*HTTP_HEADERS_HANDLE responseHeadersHandle - NULL*/
                                    NULL
                                ) != HTTPAPIEX_OK)
                                {
                                    /*Codes_SRS_IOTHUBCLIENT_LL_02_079: [ If HTTPAPIEX_SAS_ExecuteRequest fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                    LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                                    result = __FAILURE__;
                                    ;
                                }
                                else
                                {
                                    if (statusCode >= 300)
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_LL_02_087: [If the statusCode of the HTTP request is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR]*/
                                        result = __FAILURE__;
                                        LogError("HTTP code was %u", statusCode);
                                    }
                                    else
                                    {
                                        result = 0;
                                    }
                                }
                                HTTPAPIEX_SAS_Destroy(sasHandle);
                            }
                            STRING_delete(empty);
                        }
                        break;
                    }
                    case(SAS_TOKEN):
                    {
                        unsigned int notificationStatusCode;
                        if (HTTPAPIEX_ExecuteRequest(
                            iotHubHttpApiExHandle,
                            HTTPAPI_REQUEST_POST,
                            STRING_c_str(relativePathNotification),
                            requestHttpHeaders,
                            messageBody,
                            &notificationStatusCode,
                            NULL,
                            NULL) != HTTPAPIEX_OK)
                        {
                            LogError("unable to do HTTPAPIEX_ExecuteRequest");
                            result = __FAILURE__;
                        }
                        else
                        {
                            if (notificationStatusCode >= 300)
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_087: [If the statusCode of the HTTP request is greater than or equal to 300 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR]*/
                                LogError("server didn't like the notification request");
                                result = __FAILURE__;
                            }
                            else
                            {
                                result = 0;
                            }
                        }
                        break;
                    }
                    } /*switch authorizationScheme*/
                }
                STRING_delete(relativePathNotification);
            }
        }
        STRING_delete(uriResource);
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_Impl(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, const unsigned char* source, size_t size)
{
    IOTHUB_CLIENT_RESULT result;
    BUFFER_HANDLE toBeTransmitted;
    int requiredStringLength;
    char* requiredString;

    /*Codes_SRS_IOTHUBCLIENT_LL_02_061: [ If handle is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_02_062: [ If destinationFileName is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_02_063: [ If source is NULL and size is greater than 0 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    if (
        (handle == NULL) ||
        (destinationFileName == NULL) ||
        ((source == NULL) && (size > 0))
        )
    {
        LogError("invalid argument detected handle=%p destinationFileName=%p source=%p size=%zu", handle, destinationFileName, source, size);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        /*Codes_SRS_IOTHUBCLIENT_LL_02_064: [ IoTHubClient_LL_UploadToBlob shall create an HTTPAPIEX_HANDLE to the IoTHub hostname. ]*/
        HTTPAPIEX_HANDLE iotHubHttpApiExHandle = HTTPAPIEX_Create(handleData->hostname);

        /*Codes_SRS_IOTHUBCLIENT_LL_02_065: [ If creating the HTTPAPIEX_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        if (iotHubHttpApiExHandle == NULL)
        {
            LogError("unable to HTTPAPIEX_Create");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            if (
                (handleData->authorizationScheme == X509) &&

                /*transmit the x509certificate and x509privatekey*/
                /*Codes_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
                (!(
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_X509_CERT, handleData->credentials.x509credentials.x509certificate) == HTTPAPIEX_OK) &&
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_X509_PRIVATE_KEY, handleData->credentials.x509credentials.x509privatekey) == HTTPAPIEX_OK)
                ))
                )
            {
                LogError("unable to HTTPAPIEX_SetOption for x509");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_111: [ If certificates is non-NULL then certificates shall be passed to HTTPAPIEX_SetOption with optionName TrustedCerts. ]*/
                if ((handleData->certificates != NULL) && (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, "TrustedCerts", handleData->certificates) != HTTPAPIEX_OK))
                {
                    LogError("unable to set TrustedCerts!");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {

                    if (handleData->http_proxy_options.host_address != NULL)
                    {
                        HTTP_PROXY_OPTIONS proxy_options;
                        proxy_options = handleData->http_proxy_options;

                        if (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_HTTP_PROXY, &proxy_options) != HTTPAPIEX_OK)
                        {
                            LogError("unable to set http proxy!");
                            result = IOTHUB_CLIENT_ERROR;
                        }
                        else
                        {
                            result = IOTHUB_CLIENT_OK;
                        }
                    }
                    else
                    {
                        result = IOTHUB_CLIENT_OK;
                    }
                        
                    if (result != IOTHUB_CLIENT_ERROR)
                    {
                        STRING_HANDLE correlationId = STRING_new();
                        if (correlationId == NULL)
                        {
                            LogError("unable to STRING_new");
                            result = IOTHUB_CLIENT_ERROR;
                        }
                        else
                        {
                            STRING_HANDLE sasUri = STRING_new();
                            if (sasUri == NULL)
                            {
                                LogError("unable to STRING_new");
                                result = IOTHUB_CLIENT_ERROR;
                            }
                            else
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_070: [ IoTHubClient_LL_UploadToBlob shall create request HTTP headers. ]*/
                                HTTP_HEADERS_HANDLE requestHttpHeaders = HTTPHeaders_Alloc(); /*these are build by step 1 and used by step 3 too*/
                                if (requestHttpHeaders == NULL)
                                {
                                    LogError("unable to HTTPHeaders_Alloc");
                                    result = IOTHUB_CLIENT_ERROR;
                                }
                                else
                                {
                                    /*do step 1*/
                                    if (IoTHubClient_LL_UploadToBlob_step1and2(handleData, iotHubHttpApiExHandle, requestHttpHeaders, destinationFileName, correlationId, sasUri) != 0)
                                    {
                                        LogError("error in IoTHubClient_LL_UploadToBlob_step1");
                                        result = IOTHUB_CLIENT_ERROR;
                                    }
                                    else
                                    {
                                        /*do step 2.*/

                                        unsigned int httpResponse;
                                        BUFFER_HANDLE responseToIoTHub = BUFFER_new();
                                        if (responseToIoTHub == NULL)
                                        {
                                            result = IOTHUB_CLIENT_ERROR;
                                            LogError("unable to BUFFER_new");
                                        }
                                        else
                                        {
                                            int step2success;
                                            /*Codes_SRS_IOTHUBCLIENT_LL_02_083: [ IoTHubClient_LL_UploadToBlob shall call Blob_UploadFromSasUri and capture the HTTP return code and HTTP body. ]*/
                                            step2success = (Blob_UploadFromSasUri(STRING_c_str(sasUri), source, size, &httpResponse, responseToIoTHub, handleData->certificates) == BLOB_OK);
                                            if (!step2success)
                                            {
                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_084: [ If Blob_UploadFromSasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                                LogError("unable to Blob_UploadFromSasUri");

                                                /*do step 3*/ /*try*/
                                                /*Codes_SRS_IOTHUBCLIENT_LL_02_091: [ If step 2 fails without establishing an HTTP dialogue, then the HTTP message body shall look like: ]*/
                                                if (BUFFER_build(responseToIoTHub, (const unsigned char*)FILE_UPLOAD_FAILED_BODY, sizeof(FILE_UPLOAD_FAILED_BODY) / sizeof(FILE_UPLOAD_FAILED_BODY[0])) == 0)
                                                {
                                                    if (IoTHubClient_LL_UploadToBlob_step3(handleData, correlationId, iotHubHttpApiExHandle, requestHttpHeaders, responseToIoTHub) != 0)
                                                    {
                                                        LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                                                    }
                                                }
                                                result = IOTHUB_CLIENT_ERROR;
                                            }
                                            else
                                            {
                                                /*must make a json*/

                                                requiredStringLength = snprintf(NULL, 0, "{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%s\"}", ((httpResponse < 300) ? "true" : "false"), httpResponse, BUFFER_u_char(responseToIoTHub));

                                                requiredString = malloc(requiredStringLength + 1);
                                                if (requiredString == 0)
                                                {
                                                    LogError("unable to malloc");
                                                    result = IOTHUB_CLIENT_ERROR;
                                                }
                                                else
                                                {
                                                    /*do again snprintf*/
                                                    (void)snprintf(requiredString, requiredStringLength + 1, "{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%s\"}", ((httpResponse < 300) ? "true" : "false"), httpResponse, BUFFER_u_char(responseToIoTHub));
                                                    toBeTransmitted = BUFFER_create((const unsigned char*)requiredString, requiredStringLength);
                                                    if (toBeTransmitted == NULL)
                                                    {
                                                        LogError("unable to BUFFER_create");
                                                        result = IOTHUB_CLIENT_ERROR;
                                                    }
                                                    else
                                                    {
                                                        if (IoTHubClient_LL_UploadToBlob_step3(handleData, correlationId, iotHubHttpApiExHandle, requestHttpHeaders, toBeTransmitted) != 0)
                                                        {
                                                            LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                                                            result = IOTHUB_CLIENT_ERROR;
                                                        }
                                                        else
                                                        {
                                                            result = (httpResponse < 300) ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR;
                                                        }
                                                        BUFFER_delete(toBeTransmitted);
                                                    }
                                                    free(requiredString);
                                                }
                                            }
                                            BUFFER_delete(responseToIoTHub);
                                        }
                                    }
                                    HTTPHeaders_Free(requestHttpHeaders);
                                }
                                STRING_delete(sasUri);
                            }
                            STRING_delete(correlationId);
                        }
                    }
                }
            }
            HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
        }
    }
    return result;
}

void IoTHubClient_LL_UploadToBlob_Destroy(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("unexpected NULL argument");
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;
        switch (handleData->authorizationScheme)
        {
            case(SAS_TOKEN):
            {
                STRING_delete(handleData->credentials.sas);
                break;
            }
            case(DEVICE_KEY):
            {
                STRING_delete(handleData->credentials.deviceKey);
                break;
            }
            case(X509):
            {
                if (handleData->credentials.x509credentials.x509certificate != NULL)
                {
                    free((void*)handleData->credentials.x509credentials.x509certificate);
                }
                if (handleData->credentials.x509credentials.x509privatekey != NULL)
                {
                    free((void*)handleData->credentials.x509credentials.x509privatekey);
                }
                break;
            }
            default:
            {
                LogError("INTERNAL ERROR");
                break;
            }
        }
        free((void*)handleData->hostname);
        STRING_delete(handleData->deviceId);
        if (handleData->certificates != NULL)
        {
            free(handleData->certificates);
        }
        if (handleData->http_proxy_options.host_address != NULL)
        {
            free((char *)handleData->http_proxy_options.host_address);
        }
        if (handleData->http_proxy_options.username != NULL)
        {
            free((char *)handleData->http_proxy_options.username);
        }
        if (handleData->http_proxy_options.password != NULL)
        {
            free((char *)handleData->http_proxy_options.password);
        }
        free(handleData);
    }
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_SetOption(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* optionName, const void* value)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_110: [ If parameter handle is NULL then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
    if (handle == NULL)
    {
        LogError("invalid argument detected: IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle=%p, const char* optionName=%s, const void* value=%p", handle, optionName, value);
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        /*Codes_SRS_IOTHUBCLIENT_LL_02_100: [ x509certificate - then value then is a null terminated string that contains the x509 certificate. ]*/
        if (strcmp(optionName, OPTION_X509_CERT) == 0)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            if (handleData->authorizationScheme != X509)
            {
                LogError("trying to set a x509 certificate while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_103: [ The options shall be saved. ]*/
                /*try to make a copy of the certificate*/
                char* temp;
                if (mallocAndStrcpy_s(&temp, value) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_104: [ If saving fails, then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("unable to mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_105: [ Otherwise IoTHubClient_LL_UploadToBlob_SetOption shall succeed and return IOTHUB_CLIENT_OK. ]*/
                    if (handleData->credentials.x509credentials.x509certificate != NULL) /*free any previous values, if any*/
                    {
                        free((void*)handleData->credentials.x509credentials.x509certificate);
                    }
                    handleData->credentials.x509credentials.x509certificate = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        /*Codes_SRS_IOTHUBCLIENT_LL_02_101: [ x509privatekey - then value is a null terminated string that contains the x509 privatekey. ]*/
        else if (strcmp(optionName, OPTION_X509_PRIVATE_KEY) == 0)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            if (handleData->authorizationScheme != X509)
            {
                LogError("trying to set a x509 privatekey while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_103: [ The options shall be saved. ]*/
                /*try to make a copy of the privatekey*/
                char* temp;
                if (mallocAndStrcpy_s(&temp, value) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_104: [ If saving fails, then IoTHubClient_LL_UploadToBlob_SetOption shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("unable to mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_105: [ Otherwise IoTHubClient_LL_UploadToBlob_SetOption shall succeed and return IOTHUB_CLIENT_OK. ]*/
                    if (handleData->credentials.x509credentials.x509privatekey != NULL) /*free any previous values, if any*/
                    {
                        free((void*)handleData->credentials.x509credentials.x509privatekey);
                    }
                    handleData->credentials.x509credentials.x509privatekey = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp("TrustedCerts", optionName) == 0)
        {
            if (value == NULL)
            {
                LogError("NULL is a not a valid value for TrustedCerts");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                char* tempCopy;
                if (mallocAndStrcpy_s(&tempCopy, value) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (handleData->certificates != NULL)
                    {
                        free(handleData->certificates);
                    }
                    handleData->certificates = tempCopy;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        /*Codes_SRS_IOTHUBCLIENT_LL_32_008: [ OPTION_HTTP_PROXY - then the value will be a pointer to HTTP_PROXY_OPTIONS structure. ]*/
        else if (strcmp(optionName, OPTION_HTTP_PROXY) == 0)
        {
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS *)value;
            
            if (proxy_options->host_address == NULL)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_32_006: [ If `host_address` is NULL, `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
                LogError("NULL host_address in proxy options");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            /* Codes_SRS_IOTHUBCLIENT_LL_32_007: [ If only one of `username` and `password` is NULL, `IoTHubClient_LL_UploadToBlob_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
            else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
                (proxy_options->username != proxy_options->password))
            {
                LogError("Only one of username and password for proxy settings was NULL");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                if (handleData->http_proxy_options.host_address != NULL)
                {
                    free((char *)handleData->http_proxy_options.host_address);
                    handleData->http_proxy_options.host_address = NULL;
                }
                if (handleData->http_proxy_options.username != NULL)
                {
                    free((char *)handleData->http_proxy_options.username);
                    handleData->http_proxy_options.username = NULL;
                }
                if (handleData->http_proxy_options.password != NULL)
                {
                    free((char *)handleData->http_proxy_options.password);
                    handleData->http_proxy_options.password = NULL;
                }
                
                handleData->http_proxy_options.port = proxy_options->port;

                if (mallocAndStrcpy_s((char **)(&handleData->http_proxy_options.host_address), proxy_options->host_address) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - handleData->http_proxy_options.host_address");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if (proxy_options->username != NULL && mallocAndStrcpy_s((char **)(&handleData->http_proxy_options.username), proxy_options->username) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - handleData->http_proxy_options.username");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if (proxy_options->password != NULL && mallocAndStrcpy_s((char **)(&handleData->http_proxy_options.password), proxy_options->password) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - handleData->http_proxy_options.password");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_102: [ If an unknown option is presented then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
    }
    return result;
}

typedef struct IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA_TAG {
    unsigned int blockID;
    int isError;
    STRING_HANDLE xml;
    const char* relativePath; // Points to a part of SasUri
    HTTPAPIEX_HANDLE httpApiExHandle;
    BUFFER_HANDLE responseToIoTHub; // ex-httpResponse
    LOCK_HANDLE lockHandle;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE blobHandle;
    STRING_HANDLE sasUri;
    HTTPAPIEX_HANDLE iotHubHttpApiExHandle;
    STRING_HANDLE correlationId;
    HTTP_HEADERS_HANDLE requestHttpHeaders;
    unsigned int httpResponse;
    char *hostname;
} IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA;

static BLOB_RESULT Blob_UploadFromSasUri_start(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle)
{
    BLOB_RESULT result;
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)(handle);

    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* blobHandle = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)(handleData->blobHandle);

    const char* SASURI = STRING_c_str(handleData->sasUri); // TODO is it really necessary ?

    /*Codes_SRS_BLOB_02_001: [ If SASURI is NULL then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
    if (SASURI == NULL)
    {
        LogError("parameter SASURI is NULL");
        result = BLOB_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_BLOB_02_017: [ Blob_UploadFromSasUri shall copy from SASURI the hostname to a new const char* ]*/
        /*Codes_SRS_BLOB_02_004: [ Blob_UploadFromSasUri shall copy from SASURI the hostname to a new const char*. ]*/
        /*to find the hostname, the following logic is applied:*/
        /*the hostname starts at the first character after "://"*/
        /*the hostname ends at the first character before the next "/" after "://"*/
        const char* hostnameBegin = strstr(SASURI, "://");
        if (hostnameBegin == NULL)
        {
            /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
            LogError("hostname cannot be determined");
            result = BLOB_INVALID_ARG;
        }
        else
        {
            hostnameBegin += 3; /*have to skip 3 characters which are "://"*/
            const char* hostnameEnd = strchr(hostnameBegin, '/');
            if (hostnameEnd == NULL)
            {
                /*Codes_SRS_BLOB_02_005: [ If the hostname cannot be determined, then Blob_UploadFromSasUri shall fail and return BLOB_INVALID_ARG. ]*/
                LogError("hostname cannot be determined");
                result = BLOB_INVALID_ARG;
            }
            else
            {
                size_t hostnameSize = hostnameEnd - hostnameBegin;
                handleData->hostname = (char*)malloc(hostnameSize + 1); /*+1 because of '\0' at the end*/
                if (handleData->hostname == NULL)
                {
                    /*Codes_SRS_BLOB_02_016: [ If the hostname copy cannot be made then then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("oom - out of memory");
                    result = BLOB_ERROR;
                }
                else
                {
                    (void)memcpy(handleData->hostname, hostnameBegin, hostnameSize);
                    handleData->hostname[hostnameSize] = '\0';

                    /*Codes_SRS_BLOB_02_006: [ Blob_UploadFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
                    /*Codes_SRS_BLOB_02_018: [ Blob_UploadFromSasUri shall create a new HTTPAPI_EX_HANDLE by calling HTTPAPIEX_Create passing the hostname. ]*/
                    handleData->httpApiExHandle = HTTPAPIEX_Create(handleData->hostname);
                    if (handleData->httpApiExHandle == NULL)
                    {
                        /*Codes_SRS_BLOB_02_007: [ If HTTPAPIEX_Create fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR. ]*/
                        LogError("unable to create a HTTPAPIEX_HANDLE");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        if ((blobHandle->certificates != NULL)&& (HTTPAPIEX_SetOption(handleData->httpApiExHandle, "TrustedCerts", blobHandle->certificates) == HTTPAPIEX_ERROR))
                        {
                            LogError("failure in setting trusted certificates");
                            result = BLOB_ERROR;
                        }
                        else
                        {

                            /*Codes_SRS_BLOB_02_008: [ Blob_UploadFromSasUri shall compute the relative path of the request from the SASURI parameter. ]*/
                            /*Codes_SRS_BLOB_02_019: [ Blob_UploadFromSasUri shall compute the base relative path of the request from the SASURI parameter. ]*/
                            handleData->relativePath = hostnameEnd; /*this is where the relative path begins in the SasUri*/

                            /*Codes_SRS_BLOB_02_028: [ Blob_UploadFromSasUri shall construct an XML string with the following content: ]*/
                            handleData->xml = STRING_construct("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BlockList>"); /*the XML "build as we go"*/
                            if (handleData->xml == NULL)
                            {
                                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                                LogError("failed to STRING_construct");
                                result = BLOB_HTTP_ERROR;
                            }
                            else
                            {
                                /* The handle is valid */
                                result = BLOB_OK;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

static BLOB_RESULT Blob_UploadFromSasUri_stop(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle)
{
    BLOB_RESULT result;
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;

    if (handleData->isError)
    {
        /*do nothing, it will be reported "as is"*/
    }
    else
    {
        /*complete the XML*/
        if (STRING_concat(handleData->xml, "</BlockList>") != 0)
        {
            /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
            LogError("failed to STRING_concat");
            result = BLOB_ERROR;
        }
        else
        {
            /*Codes_SRS_BLOB_02_029: [Blob_UploadFromSasUri shall construct a new relativePath from following string : base relativePath + "&comp=blocklist"]*/
            STRING_HANDLE newRelativePath = STRING_construct(handleData->relativePath);
            if (newRelativePath == NULL)
            {
                /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                LogError("failed to STRING_construct");
                result = BLOB_ERROR;
            }
            else
            {
                if (STRING_concat(newRelativePath, "&comp=blocklist") != 0)
                {
                    /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                    LogError("failed to STRING_concat");
                    result = BLOB_ERROR;
                }
                else
                {
                    /*Codes_SRS_BLOB_02_030: [ Blob_UploadFromSasUri shall call HTTPAPIEX_ExecuteRequest with a PUT operation, passing the new relativePath, httpStatus and httpResponse and the XML string as content. ]*/
                    const char* s = STRING_c_str(handleData->xml);
                    BUFFER_HANDLE xmlAsBuffer = BUFFER_create((const unsigned char*)s, strlen(s));
                    if (xmlAsBuffer == NULL)
                    {
                        /*Codes_SRS_BLOB_02_033: [ If any previous operation that doesn't have an explicit failure description fails then Blob_UploadFromSasUri shall fail and return BLOB_ERROR ]*/
                        LogError("failed to BUFFER_create");
                        result = BLOB_ERROR;
                    }
                    else
                    {
                        if (HTTPAPIEX_ExecuteRequest(
                            handleData->httpApiExHandle,
                            HTTPAPI_REQUEST_PUT,
                            STRING_c_str(newRelativePath),
                            NULL,
                            xmlAsBuffer,
                            &handleData->httpResponse,
                            NULL,
                            handleData->responseToIoTHub
                        ) != HTTPAPIEX_OK)
                        {
                            /*Codes_SRS_BLOB_02_031: [ If HTTPAPIEX_ExecuteRequest fails then Blob_UploadFromSasUri shall fail and return BLOB_HTTP_ERROR. ]*/
                            LogError("unable to HTTPAPIEX_ExecuteRequest");
                            result = BLOB_HTTP_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_BLOB_02_032: [ Otherwise, Blob_UploadFromSasUri shall succeed and return BLOB_OK. ]*/
                            result = BLOB_OK;
                        }
                        BUFFER_delete(xmlAsBuffer);
                    }
                }
                STRING_delete(newRelativePath);
            }
        }
    }

    return result;
}

static IOTHUB_CLIENT_RESULT LARGE_FILE_upload_blob_start(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle, const char* destinationFileName)
{
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* blobHandle = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)(handleData->blobHandle);
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_LL_02_064: [ IoTHubClient_LL_UploadToBlob shall create an HTTPAPIEX_HANDLE to the IoTHub hostname. ]*/
    handleData->iotHubHttpApiExHandle = HTTPAPIEX_Create(blobHandle->hostname);

    /*Codes_SRS_IOTHUBCLIENT_LL_02_065: [ If creating the HTTPAPIEX_HANDLE fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
    if (handleData->iotHubHttpApiExHandle == NULL)
    {
        LogError("unable to HTTPAPIEX_Create");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        if (
            (blobHandle->authorizationScheme == X509) &&

            /*transmit the x509certificate and x509privatekey*/
            /*Codes_SRS_IOTHUBCLIENT_LL_02_106: [ - x509certificate and x509privatekey saved options shall be passed on the HTTPAPIEX_SetOption ]*/
            (!(
                (HTTPAPIEX_SetOption(handleData->iotHubHttpApiExHandle, OPTION_X509_CERT, blobHandle->credentials.x509credentials.x509certificate) == HTTPAPIEX_OK) &&
                (HTTPAPIEX_SetOption(handleData->iotHubHttpApiExHandle, OPTION_X509_PRIVATE_KEY, blobHandle->credentials.x509credentials.x509privatekey) == HTTPAPIEX_OK)
            ))
            )
        {
            LogError("unable to HTTPAPIEX_SetOption for x509");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_111: [ If certificates is non-NULL then certificates shall be passed to HTTPAPIEX_SetOption with optionName TrustedCerts. ]*/
            if ((blobHandle->certificates != NULL) && (HTTPAPIEX_SetOption(handleData->iotHubHttpApiExHandle, "TrustedCerts", blobHandle->certificates) != HTTPAPIEX_OK))
            {
                LogError("unable to set TrustedCerts!");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {

                if (blobHandle->http_proxy_options.host_address != NULL)
                {
                    HTTP_PROXY_OPTIONS proxy_options;
                    proxy_options = blobHandle->http_proxy_options;

                    if (HTTPAPIEX_SetOption(handleData->iotHubHttpApiExHandle, OPTION_HTTP_PROXY, &proxy_options) != HTTPAPIEX_OK)
                    {
                        LogError("unable to set http proxy!");
                        result = IOTHUB_CLIENT_ERROR;
                    }
                    else
                    {
                        result = IOTHUB_CLIENT_OK;
                    }
                }
                else
                {
                    result = IOTHUB_CLIENT_OK;
                }

                if (result != IOTHUB_CLIENT_ERROR)
                {
                    handleData->correlationId = STRING_new();
                    if (handleData->correlationId == NULL)
                    {
                        LogError("unable to STRING_new");
                        result = IOTHUB_CLIENT_ERROR;
                    }
                    else
                    {
                        handleData->sasUri = STRING_new();
                        if (handleData->sasUri == NULL)
                        {
                            LogError("unable to STRING_new");
                            result = IOTHUB_CLIENT_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_070: [ IoTHubClient_LL_UploadToBlob shall create request HTTP headers. ]*/
                            handleData->requestHttpHeaders = HTTPHeaders_Alloc(); /*these are build by step 1 and used by step 3 too*/
                            if (handleData->requestHttpHeaders == NULL)
                            {
                                LogError("unable to HTTPHeaders_Alloc");
                                result = IOTHUB_CLIENT_ERROR;
                            }
                            else
                            {
                                /*do step 1*/
                                if (IoTHubClient_LL_UploadToBlob_step1and2(blobHandle, handleData->iotHubHttpApiExHandle, handleData->requestHttpHeaders, destinationFileName, handleData->correlationId, handleData->sasUri) != 0)
                                {
                                    LogError("error in IoTHubClient_LL_UploadToBlob_step1");
                                    result = IOTHUB_CLIENT_ERROR;
                                }
                                else
                                {
                                    /*do step 2.*/
                                    handleData->responseToIoTHub = BUFFER_new();
                                    if (handleData->responseToIoTHub == NULL)
                                    {
                                        result = IOTHUB_CLIENT_ERROR;
                                        LogError("unable to BUFFER_new");
                                    }
                                    else
                                    {
                                        if (Blob_UploadFromSasUri_start(handle) != BLOB_OK)
                                        {
                                            result = IOTHUB_CLIENT_ERROR;
                                            LogError("unable to Blob_UploadFromSasUri_start");
                                        }
                                        else
                                        {
                                            /* The handle is valid */
                                            result = IOTHUB_CLIENT_OK;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

static IOTHUB_CLIENT_RESULT LARGE_FILE_upload_blob_stop(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* blobHandleData = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)(handleData->blobHandle);

    int step2success;
    step2success = (Blob_UploadFromSasUri_stop(handle) == BLOB_OK);

    if (!step2success)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_084: [ If Blob_UploadFromSasUri fails then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("unable to Blob_UploadFromSasUri");

        /*do step 3*/ /*try*/
        /*Codes_SRS_IOTHUBCLIENT_LL_02_091: [ If step 2 fails without establishing an HTTP dialogue, then the HTTP message body shall look like: ]*/
        if (BUFFER_build(handleData->responseToIoTHub, (const unsigned char*)FILE_UPLOAD_FAILED_BODY, sizeof(FILE_UPLOAD_FAILED_BODY) / sizeof(FILE_UPLOAD_FAILED_BODY[0])) == 0)
        {
            if (IoTHubClient_LL_UploadToBlob_step3(blobHandleData, handleData->correlationId, handleData->iotHubHttpApiExHandle, handleData->requestHttpHeaders, handleData->responseToIoTHub) != 0)
            {
                LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
            }
        }
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        /*must make a json*/

        int requiredStringLength = snprintf(NULL, 0, "{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%s\"}", ((handleData->httpResponse < 300) ? "true" : "false"), handleData->httpResponse, BUFFER_u_char(handleData->responseToIoTHub));

        char * requiredString = malloc(requiredStringLength + 1);
        if (requiredString == 0)
        {
            LogError("unable to malloc");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            /*do again snprintf*/
            (void)snprintf(requiredString, requiredStringLength + 1, "{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%s\"}", ((handleData->httpResponse < 300) ? "true" : "false"), handleData->httpResponse, BUFFER_u_char(handleData->responseToIoTHub));
            BUFFER_HANDLE toBeTransmitted = BUFFER_create((const unsigned char*)requiredString, requiredStringLength);
            if (toBeTransmitted == NULL)
            {
                LogError("unable to BUFFER_create");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (IoTHubClient_LL_UploadToBlob_step3(blobHandleData, handleData->correlationId, handleData->iotHubHttpApiExHandle, handleData->requestHttpHeaders, toBeTransmitted) != 0)
                {
                    LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    result = (handleData->httpResponse < 300) ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR;
                }
                BUFFER_delete(toBeTransmitted);
            }
            free(requiredString);
        }
    }

    return result;
}

static void Destroy_LargeFileHandle(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle)
{
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;
    if (handle == NULL)
    {
        LogError("unexpected NULL argument");
    }
    else
    {
        if (handleData->hostname != NULL)
        {
            free(handleData->hostname);
        }

        if (handleData->httpApiExHandle)
        {
            HTTPAPIEX_Destroy(handleData->httpApiExHandle);
        }

        if (handleData->xml != NULL)
        {
            STRING_delete(handleData->xml);
        }

        if (handleData->responseToIoTHub != NULL)
        {
            BUFFER_delete(handleData->responseToIoTHub);
        }

        if (handleData->requestHttpHeaders != NULL)
        {
            HTTPHeaders_Free(handleData->requestHttpHeaders);
        }

        if (handleData->correlationId != NULL)
        {
            STRING_delete(handleData->correlationId);
        }

        if (handleData->iotHubHttpApiExHandle != NULL)
        {
            HTTPAPIEX_Destroy(handleData->iotHubHttpApiExHandle);
        }

        if (handleData->lockHandle != NULL)
        {
            Lock_Deinit(handleData->lockHandle);
        }

        if (handleData->sasUri != NULL)
        {
            STRING_delete(handleData->sasUri);
        }

        free(handle);
    }
}

IOTHUB_CLIENT_LARGE_FILE_HANDLE IoTHubClient_LL_LargeFileOpen_Impl(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE blobHandle, const char* destinationFileName)
{
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = NULL;
    if (destinationFileName == NULL || blobHandle == NULL)
    {
        LogError("invalid arguments : destinationFileName=%p, blobHandle=%p", destinationFileName, blobHandle);
    }
    else
    {
        handleData = calloc(1, sizeof(IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA));  /* use calloc to initialize all internal handles to NULL */
        if (handleData == NULL)
        {
            LogError("oom - malloc");
            /*return as is*/
        }
        else
        {
            handleData->isError = 0;
            handleData->blobHandle = blobHandle;

            handleData->lockHandle = Lock_Init();
            if (handleData->lockHandle == NULL)
            {
                LogError("Could not Lock_Init");
                free(handleData);
                handleData = NULL;
            }
            else
            {
                if(LARGE_FILE_upload_blob_start((IOTHUB_CLIENT_LARGE_FILE_HANDLE)handleData, destinationFileName) != IOTHUB_CLIENT_OK)
                {
                    LogError("Could not LARGE_FILE_upload_blob_start");
                    Destroy_LargeFileHandle((IOTHUB_CLIENT_LARGE_FILE_HANDLE)handleData);
                    handleData = NULL;
                }
            }
        }
    }
    return (IOTHUB_CLIENT_LARGE_FILE_HANDLE)handleData;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_LargeFileClose_Impl(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle)
{
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;

    if (handle == NULL)
    {
        LogError("unexpected NULL argument");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        /* Close file handle */
        result = LARGE_FILE_upload_blob_stop(handle);

        /* Clean resources */
        Destroy_LargeFileHandle(handle);
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_LargeFileWrite_Impl(IOTHUB_CLIENT_LARGE_FILE_HANDLE handle, const unsigned char* source, size_t size)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LARGE_FILE_HANDLE_DATA*)handle;
    if (
        (size > 0) &&
        (source == NULL)
        )
    {
        LogError("combination of source = %p and size = %zu is invalid", source, size);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else if (size > 4 * 1024 * 1024)
    {
        LogError("size too big (%zu)", size);
        result = IOTHUB_CLIENT_INVALID_SIZE;
    }
    else
    {
        // fileHandle must be valid
        if (1 == handleData->isError)
        {
            LogError("Invalid file handle");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else if (handleData->blockID >= 50000)
        {
            LogError("Too many blocks already written (max 50000)");
            result = IOTHUB_CLIENT_INVALID_SIZE;
        }
        else
        {
            /* No concurrent block writing allowed */
            if (Lock(handleData->lockHandle) != LOCK_OK)
            {
                LogError("unable to lock");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /* Create buffer */
                BUFFER_HANDLE requestBuffer = BUFFER_create(source, size);
                if (requestBuffer == NULL)
                {
                    LogError("unable to BUFFER_create");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    BLOB_RESULT uploadBlockResult;
                    uploadBlockResult = Blob_UploadNextBlock(requestBuffer,
                                                             handleData->blockID,
                                                             &(handleData->isError),
                                                             handleData->xml,
                                                             handleData->relativePath,
                                                             handleData->httpApiExHandle,
                                                             &handleData->httpResponse,
                                                             handleData->responseToIoTHub);

                    if (uploadBlockResult == BLOB_OK && handleData->isError == 0)
                    {
                        handleData->blockID++;
                        result = IOTHUB_CLIENT_OK;
                    }
                    else
                    {
                        LogError("unable to Blob_UploadNextBlock (uploadBlockResult = %i, fileHandle->isError = %i)", uploadBlockResult, handleData->isError);
                        result = IOTHUB_CLIENT_ERROR;
                    }

                    BUFFER_delete(requestBuffer);
                }

                (void)Unlock(handleData->lockHandle);
            }
        }
    }

    return result;
}

#endif /*DONT_USE_UPLOADTOBLOB*/


