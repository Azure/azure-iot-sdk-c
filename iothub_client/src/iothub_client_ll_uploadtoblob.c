// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DONT_USE_UPLOADTOBLOB

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
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "iothub_client_core_ll.h"
#include "iothub_client_options.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_version.h"
#include "iothub_transport_ll.h"
#include "parson.h"
#include "internal/iothub_client_ll_uploadtoblob.h"
#include "internal/iothub_client_authorization.h"
#include "internal/blob.h"

#define API_VERSION "?api-version=2016-11-14"

static const char* const RESPONSE_BODY_FORMAT = "{\"correlationId\":\"%s\", \"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%s\"}";
static const char* const RESPONSE_BODY_SUCCESS_BOOLEAN_STRING = "true";
static const char* const RESPONSE_BODY_ERROR_BOOLEAN_STRING = "false";

#define INDEFINITE_TIME                            ((time_t)-1)

static const char* const EMPTY_STRING = "";
static const char* const HEADER_AUTHORIZATION = "Authorization";
static const char* const HEADER_APP_JSON = "application/json";

#define HTTP_STATUS_CODE_OK                 200
#define HTTP_STATUS_CODE_BAD_REQUEST        400
#define IS_HTTP_STATUS_CODE_SUCCESS(x)      ((x) >= 100 && (x) < 300)

typedef struct UPLOADTOBLOB_X509_CREDENTIALS_TAG
{
    char* x509certificate;
    char* x509privatekey;
    int* x509privatekeyType;
    char* engine;
} UPLOADTOBLOB_X509_CREDENTIALS;

typedef enum UPLOADTOBLOB_CURL_VERBOSITY_TAG
{
    UPLOADTOBLOB_CURL_VERBOSITY_UNSET,
    UPLOADTOBLOB_CURL_VERBOSITY_ON,
    UPLOADTOBLOB_CURL_VERBOSITY_OFF
} UPLOADTOBLOB_CURL_VERBOSITY;

typedef struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA_TAG
{
    const char* deviceId;
    char* hostname;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
    IOTHUB_CREDENTIAL_TYPE cred_type;
    union 
    {
        UPLOADTOBLOB_X509_CREDENTIALS x509_credentials;
        char* supplied_sas_token;
    } credentials;
    
    char* certificates;
    HTTP_PROXY_OPTIONS http_proxy_options;
    UPLOADTOBLOB_CURL_VERBOSITY curl_verbosity_level;
    size_t blob_upload_timeout_millisecs;
    const char* networkInterface;
    bool tls_renegotiation;
} IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA;

typedef struct BLOB_UPLOAD_CONTEXT_TAG
{
    const unsigned char* blobSource; /* source to upload */
    size_t blobSourceSize; /* size of the source */
    size_t remainingSizeToUpload; /* size not yet uploaded */
} BLOB_UPLOAD_CONTEXT;

typedef struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_STRUCT
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* u2bClientData;

    char* blobStorageHostname;
    char* blobStorageRelativePath;
    SINGLYLINKEDLIST_HANDLE blockIdList;

    HTTPAPIEX_HANDLE blobHttpApiHandle;
} IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT;

static int send_http_sas_request(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_client, HTTPAPIEX_HANDLE http_api_handle, const char* relative_path, HTTP_HEADERS_HANDLE request_header, BUFFER_HANDLE blobBuffer, BUFFER_HANDLE response_buff)
{
    int result;
    STRING_HANDLE uri_resource = STRING_construct_sprintf("%s/devices/%s", upload_client->hostname, upload_client->deviceId);

    if (uri_resource == NULL)
    {
        LogError("Failure constructing uri string");
        result = MU_FAILURE;
    }
    else
    {
        const char* uriResourceCharPtr = STRING_c_str(uri_resource);
        HTTPAPIEX_SAS_HANDLE http_sas_handle = HTTPAPIEX_SAS_Create_From_String(IoTHubClient_Auth_Get_DeviceKey(upload_client->authorization_module), uriResourceCharPtr, EMPTY_STRING);

        if (http_sas_handle == NULL)
        {
            LogError("unable to HTTPAPIEX_SAS_Create");
            result = MU_FAILURE;
        }
        else
        {
            unsigned int statusCode = 0;

            if (HTTPAPIEX_SAS_ExecuteRequest(
                http_sas_handle, http_api_handle, HTTPAPI_REQUEST_POST, relative_path, request_header,
                blobBuffer, &statusCode, NULL, response_buff) != HTTPAPIEX_OK)
            {
                LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                result = MU_FAILURE;
            }
            else if (!IS_HTTP_STATUS_CODE_SUCCESS(statusCode))
            {
                LogError("HTTP failed response code was %u", statusCode);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }

            HTTPAPIEX_SAS_Destroy(http_sas_handle);
        }

        STRING_delete(uri_resource);
    }

    return result;
}

static int send_http_request(HTTPAPIEX_HANDLE http_api_handle, const char* relative_path, HTTP_HEADERS_HANDLE request_header, BUFFER_HANDLE blobBuffer, BUFFER_HANDLE response_buff)
{
    int result;
    unsigned int statusCode = 0;
    if (HTTPAPIEX_ExecuteRequest(http_api_handle, HTTPAPI_REQUEST_POST, relative_path, request_header,
        blobBuffer, &statusCode, NULL, response_buff) != HTTPAPIEX_OK)
    {
        LogError("unable to HTTPAPIEX_ExecuteRequest");
        result = MU_FAILURE;
    }
    else if (!IS_HTTP_STATUS_CODE_SUCCESS(statusCode))
    {
        LogError("HTTP failed response code was %u", statusCode);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int parseResultFromIoTHub(const char* json_response, char** uploadCorrelationId, char** azureBlobSasUri)
{
    int result;
    JSON_Object* json_obj;
    JSON_Value* json = json_parse_string(json_response);

    if (json == NULL)
    {
        LogError("unable to json_parse_string");
        result = MU_FAILURE;

    }
    else
    {
        if ((json_obj = json_value_get_object(json)) == NULL)
        {
            LogError("unable to get json_value_get_object");
            result = MU_FAILURE;
        }
        else
        {
            const char* json_corr_id;
            const char* json_hostname;
            const char* json_container_name;
            const char* json_blob_name;
            const char* json_sas_token;
            STRING_HANDLE filename;

            if ((json_corr_id = json_object_get_string(json_obj, "correlationId")) == NULL)
            {
                LogError("unable to retrieve correlation Id from json");
                result = MU_FAILURE;
            }
            else if ((json_hostname = json_object_get_string(json_obj, "hostName")) == NULL)
            {
                LogError("unable to retrieve hostname Id from json");
                result = MU_FAILURE;
            }
            else if ((json_container_name = json_object_get_string(json_obj, "containerName")) == NULL)
            {
                LogError("unable to retrieve container name Id from json");
                result = MU_FAILURE;
            }
            else if ((json_blob_name = json_object_get_string(json_obj, "blobName")) == NULL)
            {
                LogError("unable to retrieve blob name Id from json");
                result = MU_FAILURE;
            }
            else if ((json_sas_token = json_object_get_string(json_obj, "sasToken")) == NULL)
            {
                LogError("unable to retrieve sas token from json");
                result = MU_FAILURE;
            }
            else if ((filename = URL_EncodeString(json_blob_name)) == NULL)
            {
                LogError("unable to URL encode of filename");
                result = MU_FAILURE;
            }
            else
            {
                STRING_HANDLE sas_uri;

                if ((sas_uri = STRING_new()) == NULL)
                {
                    LogError("Failed to allocate SAS URI STRING_HANDLE");
                    result = MU_FAILURE;
                }
                else
                {
                    if (STRING_sprintf(sas_uri, "https://%s/%s/%s%s", json_hostname, json_container_name, STRING_c_str(filename), json_sas_token) != 0)
                    {
                        LogError("unable to construct uri string");
                        result = MU_FAILURE;
                    }
                    else if (mallocAndStrcpy_s(uploadCorrelationId, json_corr_id) != 0)
                    {
                        LogError("unable to copy correlation Id");
                        result = MU_FAILURE;
                    }
                    else if (mallocAndStrcpy_s(azureBlobSasUri, STRING_c_str(sas_uri)) != 0)
                    {
                        LogError("unable to copy SAS URI");
                        free(*uploadCorrelationId);
                        *uploadCorrelationId = NULL;
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                    
                    STRING_delete(sas_uri);
                }

                STRING_delete(filename);
            }
        }

        json_value_free(json);
    }

    return result;
}


static HTTP_HEADERS_HANDLE createIotHubRequestHttpHeaders(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data)
{
    HTTP_HEADERS_HANDLE iotHubRequestHttpHeaders;

    if ((iotHubRequestHttpHeaders = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("Failed to allocate HTTP headers for IoT Hub connection");
    }
    else
    {
        bool isError;

        if ((HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, "Content-Type", HEADER_APP_JSON) != HTTP_HEADERS_OK) ||
            (HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, "Accept", HEADER_APP_JSON) != HTTP_HEADERS_OK) ||
            (HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, "User-Agent", "iothubclient/" IOTHUB_SDK_VERSION) != HTTP_HEADERS_OK))
        {
            LogError("unable to add HTTP headers");
            isError = true;
        }
        else
        {
            switch (upload_data->cred_type)
            {
                default:
                {
                    LogError("Internal Error: unexpected value in auth schema = %d", upload_data->cred_type);
                    isError = true;
                    break;
                }
                case IOTHUB_CREDENTIAL_TYPE_X509_ECC:
                case IOTHUB_CREDENTIAL_TYPE_X509:
                {
                    isError = false;
                    break;
                }
                case IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY:
                {
                    // This is the scenario that uses `send_http_sas_request`.
                    // Believe it or not, HTTPAPIEX_SAS_ExecuteRequest requires the headers to have
                    // an empty Authorization header so it can replace it with the authorization value. 

                    if (HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, HEADER_AUTHORIZATION, EMPTY_STRING) != HTTP_HEADERS_OK)
                    {
                        LogError("unable to add authorization header");
                        isError = true;
                    }
                    else
                    {
                        isError = false;
                    }
                    break;
                }
                case IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH:
                {
                    STRING_HANDLE uri_resource = STRING_construct_sprintf("%s/devices/%s", upload_data->hostname, upload_data->deviceId);

                    if (uri_resource == NULL)
                    {
                        LogError("Failure constructing uri string");
                        isError = true;
                    }
                    else
                    {
                        time_t curr_time;
                        if ((curr_time = get_time(NULL)) == INDEFINITE_TIME)
                        {
                            LogError("failure retrieving time");
                            isError = true;
                        }
                        else
                        {
                            uint64_t expiry = (uint64_t)(difftime(curr_time, 0) + 3600);
                            char* sas_token = IoTHubClient_Auth_Get_SasToken(upload_data->authorization_module, STRING_c_str(uri_resource), expiry, EMPTY_STRING);

                            if (sas_token == NULL)
                            {
                                LogError("unable to retrieve sas token");
                                isError = true;
                            }
                            else
                            {
                                if (HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, HEADER_AUTHORIZATION, sas_token) != HTTP_HEADERS_OK)
                                {
                                    LogError("unable to HTTPHeaders_AddHeaderNameValuePair");
                                    isError = true;
                                }
                                else
                                {
                                    isError = false;
                                }

                                free(sas_token);
                            }
                        }

                        STRING_delete(uri_resource);
                    }

                    break;
                }
                case IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN:
                {
                    if (HTTPHeaders_AddHeaderNameValuePair(iotHubRequestHttpHeaders, HEADER_AUTHORIZATION, upload_data->credentials.supplied_sas_token) != HTTP_HEADERS_OK)
                    {
                        LogError("unable to add authorization header");
                        isError = true;
                    }
                    else
                    {
                        isError = false;
                    }
                    break;
                }
            }
        }

        if (isError)
        {
            HTTPHeaders_Free(iotHubRequestHttpHeaders);
            iotHubRequestHttpHeaders = NULL;
        }
    }

    return iotHubRequestHttpHeaders;
}

static HTTPAPIEX_HANDLE createIotHubHttpApiExHandle(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* u2bLLClientData)
{
    HTTPAPIEX_HANDLE iotHubHttpApiExHandle = HTTPAPIEX_Create(u2bLLClientData->hostname);

    if (iotHubHttpApiExHandle == NULL)
    {
        LogError("unable to HTTPAPIEX_Create");
    }
    else if (u2bLLClientData->blob_upload_timeout_millisecs != 0 &&
             HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_HTTP_TIMEOUT, &u2bLLClientData->blob_upload_timeout_millisecs) != HTTPAPIEX_OK)
    {
        LogError("unable to set blob transfer timeout");
        HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
        iotHubHttpApiExHandle = NULL;
    }
    else
    {
        if (u2bLLClientData->curl_verbosity_level != UPLOADTOBLOB_CURL_VERBOSITY_UNSET)
        {
            size_t curl_verbose = (u2bLLClientData->curl_verbosity_level == UPLOADTOBLOB_CURL_VERBOSITY_ON);
            (void)HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_CURL_VERBOSE, &curl_verbose);
        }

        if ((u2bLLClientData->networkInterface) != NULL &&
            (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_CURL_INTERFACE, u2bLLClientData->networkInterface) != HTTPAPIEX_OK))
        {
            LogError("unable to set networkInteface!");
            HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
            iotHubHttpApiExHandle = NULL;
        }
        else
        {
            /*transmit the x509certificate and x509privatekey*/
            if ((u2bLLClientData->cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || u2bLLClientData->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC) &&
                    (((u2bLLClientData->tls_renegotiation == true) && (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_SET_TLS_RENEGOTIATION, &u2bLLClientData->tls_renegotiation) != HTTPAPIEX_OK)) ||
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_X509_CERT, u2bLLClientData->credentials.x509_credentials.x509certificate) != HTTPAPIEX_OK) ||
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_X509_PRIVATE_KEY, u2bLLClientData->credentials.x509_credentials.x509privatekey) != HTTPAPIEX_OK) ||
                    ((u2bLLClientData->credentials.x509_credentials.x509privatekeyType != NULL) && (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_OPENSSL_PRIVATE_KEY_TYPE, u2bLLClientData->credentials.x509_credentials.x509privatekeyType) != HTTPAPIEX_OK)) ||
                    ((u2bLLClientData->credentials.x509_credentials.engine != NULL) && (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_OPENSSL_ENGINE, u2bLLClientData->credentials.x509_credentials.engine) != HTTPAPIEX_OK))))
            {
                LogError("unable to HTTPAPIEX_SetOption for x509 certificate");
                HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
                iotHubHttpApiExHandle = NULL;
            }
            else if ((u2bLLClientData->certificates != NULL) &&
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_TRUSTED_CERT, u2bLLClientData->certificates) != HTTPAPIEX_OK))
            {
                LogError("unable to set TrustedCerts!");
                HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
                iotHubHttpApiExHandle = NULL;
            }
            else if ((u2bLLClientData->http_proxy_options.host_address != NULL) &&
                    (HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_HTTP_PROXY, &(u2bLLClientData->http_proxy_options)) != HTTPAPIEX_OK))
            {
                LogError("unable to set http proxy!");
                HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
                iotHubHttpApiExHandle = NULL;
            }
            // TODO: proxy username and password can be set through SetOption, but are not used (it was the previous behavior as well). Bug?
        }
    }

    return iotHubHttpApiExHandle;
}

static int IoTHubClient_LL_UploadToBlob_GetBlobCredentialsFromIoTHub(
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, const char* destinationFileName, char** correlationId, char** sasUri)
{
    int result;
    STRING_HANDLE relativePath = STRING_construct_sprintf("/devices/%s/files/%s", upload_data->deviceId, API_VERSION);

    if (relativePath == NULL)
    {
        LogError("Failure constructing string");
        result = MU_FAILURE;
    }
    else
    {
        STRING_HANDLE blobName = STRING_construct_sprintf("{ \"blobName\": \"%s\" }", destinationFileName);

        if (blobName == NULL)
        {
            LogError("Failure constructing string");
            result = MU_FAILURE;
        }
        else
        {
            // Keep STRING_length separate in an separate call to avoid messing up with unit tests.
            // For details, see function parameter evaluation order (C99 6.5.2.2p10).
            size_t blobNameLength = STRING_length(blobName);
            BUFFER_HANDLE blobBuffer = BUFFER_create(
                (const unsigned char *)STRING_c_str(blobName), blobNameLength);

            if (blobBuffer == NULL)
            {
                LogError("unable to create BUFFER");
                result = MU_FAILURE;
            }
            else
            {
                BUFFER_HANDLE responseContent;

                if ((responseContent = BUFFER_new()) == NULL)
                {
                    result = MU_FAILURE;
                    LogError("unable to BUFFER_new");
                }
                else
                {
                    HTTPAPIEX_HANDLE iotHubHttpApiExHandle = createIotHubHttpApiExHandle(upload_data);

                    if (iotHubHttpApiExHandle == NULL)
                    {
                        LogError("Failed to create the HTTPAPIEX_HANDLE for Azure IoT Hub");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        HTTP_HEADERS_HANDLE iotHubRequestHttpHeaders;

                        if ((iotHubRequestHttpHeaders = createIotHubRequestHttpHeaders(upload_data)) == NULL)
                        {
                            LogError("Failed to allocate HTTP headers for IoT Hub connection");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            bool wasIoTHubRequestSuccess = false;

                            switch (upload_data->cred_type)
                            {
                                default:
                                {
                                    LogError("Internal Error: unexpected value in auth schema = %d", upload_data->cred_type);
                                    break;
                                }
                                case IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN:
                                case IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH:
                                case IOTHUB_CREDENTIAL_TYPE_X509_ECC:
                                case IOTHUB_CREDENTIAL_TYPE_X509:
                                {
                                    if (send_http_request(iotHubHttpApiExHandle, STRING_c_str(relativePath), iotHubRequestHttpHeaders, blobBuffer, responseContent) != 0)
                                    {
                                        LogError("unable to HTTPAPIEX_ExecuteRequest");
                                    }
                                    else
                                    {
                                        wasIoTHubRequestSuccess = true;
                                    }
                                    break;
                                }
                                case IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY:
                                {
                                    if (send_http_sas_request(upload_data, iotHubHttpApiExHandle, STRING_c_str(relativePath), iotHubRequestHttpHeaders, blobBuffer, responseContent) != 0)
                                    {
                                        LogError("unable to HTTPAPIEX_SAS_ExecuteRequest");
                                    }
                                    else
                                    {
                                        wasIoTHubRequestSuccess = true;
                                    }

                                    break;
                                }
                            }

                            if (wasIoTHubRequestSuccess)
                            {
                                const unsigned char* responseContent_u_char = BUFFER_u_char(responseContent);
                                size_t responseContent_length = BUFFER_length(responseContent);
                                STRING_HANDLE responseAsString = STRING_from_byte_array(responseContent_u_char, responseContent_length);
                                if (responseAsString == NULL)
                                {
                                    LogError("unable to get the response as string");
                                    result = MU_FAILURE;
                                }
                                else
                                {
                                    if (parseResultFromIoTHub(STRING_c_str(responseAsString), correlationId, sasUri) != 0)
                                    {
                                        LogError("unable to parse json result");
                                        result = MU_FAILURE;
                                    }
                                    else
                                    {
                                        result = 0;
                                    }

                                    STRING_delete(responseAsString);
                                }
                            }
                            else
                            {
                                result = MU_FAILURE;
                            }

                            HTTPHeaders_Free(iotHubRequestHttpHeaders);
                        }

                        HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
                    }

                    BUFFER_delete(responseContent);
                }

                BUFFER_delete(blobBuffer);
            }

            STRING_delete(blobName);
        }

        STRING_delete(relativePath);
    }

    return result;
}

static int IoTHubClient_LL_UploadToBlob_NotifyIoTHubOfUploadCompletion(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, BUFFER_HANDLE messageBody)
{
    int result;
    STRING_HANDLE relativePathNotification = STRING_construct_sprintf("/devices/%s/files/notifications/%s", upload_data->deviceId, API_VERSION);

    if (relativePathNotification == NULL)
    {
        LogError("Failure constructing relative path string");
        result = MU_FAILURE;
    }
    else
    {
        HTTPAPIEX_HANDLE iotHubHttpApiExHandle = createIotHubHttpApiExHandle(upload_data);

        if (iotHubHttpApiExHandle == NULL)
        {
            LogError("Failed to create the HTTPAPIEX_HANDLE for Azure IoT Hub");
            result = MU_FAILURE;
        }
        else
        {
            HTTP_HEADERS_HANDLE iotHubRequestHttpHeaders;

            if ((iotHubRequestHttpHeaders = createIotHubRequestHttpHeaders(upload_data)) == NULL)
            {
                LogError("Failed to allocate HTTP headers for IoT Hub connection");
                result = MU_FAILURE;
            }
            else
            {
                switch (upload_data->cred_type)
                {
                    default:
                    {
                        LogError("internal error: unknown authorization Scheme");
                        result = MU_FAILURE;
                        break;
                    }
                    case IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN:
                    case IOTHUB_CREDENTIAL_TYPE_X509:
                    case IOTHUB_CREDENTIAL_TYPE_X509_ECC:
                    case IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH:
                    {
                        if (send_http_request(iotHubHttpApiExHandle, STRING_c_str(relativePathNotification), iotHubRequestHttpHeaders, messageBody, NULL) != 0)
                        {
                            LogError("unable to execute send_http_request");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            result = 0;
                        }
                        break;
                    }
                    case IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY:
                    {
                        if (send_http_sas_request(upload_data, iotHubHttpApiExHandle, STRING_c_str(relativePathNotification), iotHubRequestHttpHeaders, messageBody, NULL) != 0)
                        {
                            LogError("unable to execute send_http_sas_request");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            result = 0;
                        }
                        break;
                    }
                }

                HTTPHeaders_Free(iotHubRequestHttpHeaders);
            }

            HTTPAPIEX_Destroy(iotHubHttpApiExHandle);
        }

        STRING_delete(relativePathNotification);
    }

    return result;
}

static IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT* createUploadToBlobContextInstance(void)
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT* result;

    if ((result = malloc(sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT))) == NULL)
    {
        LogError("Failed allocating IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT");
    }
    else
    {
        (void)memset(result, 0, sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT));

        if ((result->blockIdList = singlylinkedlist_create()) == NULL)
        {
            LogError("Failed allocating list for blockIds");
            free(result);
            result = NULL;
        }
    }

    return result;
}

static void destroyUploadToBlobContextInstance(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT* uploadContext)
{
    if (uploadContext != NULL)
    {
        HTTPAPIEX_Destroy(uploadContext->blobHttpApiHandle);

        if (uploadContext->blockIdList != NULL)
        {
            Blob_ClearBlockIdList(uploadContext->blockIdList);
            singlylinkedlist_destroy(uploadContext->blockIdList);
        }

        free(uploadContext->blobStorageHostname);
        free(uploadContext->blobStorageRelativePath);
        free(uploadContext);
    }
}

static int parseAzureBlobSasUri(const char* sasUri, char** blobStorageHostname, char** blobStorageRelativePath)
{
    int result;
    const char* hostnameBegin;
    char* hostname = NULL;
    char* relativePath = NULL;
    size_t sasUriLength = strlen(sasUri);
    
    /*to find the hostname, the following logic is applied:*/
    /*the hostname starts at the first character after "://"*/
    /*the hostname ends at the first character before the next "/" after "://"*/
    if ((hostnameBegin = strstr(sasUri, "://")) == NULL)
    {
        LogError("hostname cannot be determined");
        result = MU_FAILURE;
    }
    else
    {
        hostnameBegin += 3; /*have to skip 3 characters which are "://"*/
        const char* relativePathBegin = strchr(hostnameBegin, '/');

        if (relativePathBegin == NULL)
        {
            LogError("relative path cannot be determined");
            result = MU_FAILURE;
        }
        else
        {
            size_t hostnameSize = relativePathBegin - hostnameBegin;

            if ((hostname = malloc(hostnameSize + 1)) == NULL)
            {
                LogError("failed to allocate memory for blob storage hostname");
                result = MU_FAILURE;
            }
            else
            {
                (void)memcpy(hostname, hostnameBegin, hostnameSize);
                hostname[hostnameSize] = '\0';
                size_t relativePathSize = sasUriLength - (relativePathBegin - sasUri);

                if ((relativePath = malloc(relativePathSize + 1)) == NULL)
                {
                    LogError("failed to allocate memory for blob storage relative path");
                    free(hostname);
                    result = MU_FAILURE;
                }
                else
                {
                    (void)memcpy(relativePath, relativePathBegin, relativePathSize);
                    relativePath[relativePathSize] = '\0';
                    *blobStorageHostname = hostname;
                    *blobStorageRelativePath = relativePath;
                    result = 0;
                }
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE IoTHubClient_LL_UploadToBlob_Create(const IOTHUB_CLIENT_CONFIG* config, IOTHUB_AUTHORIZATION_HANDLE auth_handle)
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data;
    if (auth_handle == NULL || config == NULL)
    {
        LogError("Invalid arguments auth_handle: %p, config: %p", auth_handle, config);
        upload_data = NULL;
    }
    else
    {
        upload_data = malloc(sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA));
        if (upload_data == NULL)
        {
            LogError("Failed malloc allocation");
            /*return as is*/
        }
        else
        {
            memset(upload_data, 0, sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA));

            upload_data->authorization_module = auth_handle;

            size_t iotHubNameLength = strlen(config->iotHubName);
            size_t iotHubSuffixLength = strlen(config->iotHubSuffix);
            upload_data->hostname = malloc(iotHubNameLength + 1 + iotHubSuffixLength + 1); /*first +1 is because "." the second +1 is because \0*/
            if (upload_data->hostname == NULL)
            {
                LogError("Failed malloc allocation");
                free(upload_data);
                upload_data = NULL;
            }
            else if ((upload_data->deviceId = IoTHubClient_Auth_Get_DeviceId(upload_data->authorization_module)) == NULL)
            {
                LogError("Failed retrieving device ID");
                free(upload_data->hostname);
                free(upload_data);
                upload_data = NULL;
            }
            else
            {
                char* insert_pos = (char*)upload_data->hostname;
                (void)memcpy((char*)insert_pos, config->iotHubName, iotHubNameLength);
                insert_pos += iotHubNameLength;
                *insert_pos = '.';
                insert_pos += 1;
                (void)memcpy(insert_pos, config->iotHubSuffix, iotHubSuffixLength); /*+1 will copy the \0 too*/
                insert_pos += iotHubSuffixLength;
                *insert_pos = '\0';

                upload_data->cred_type = IoTHubClient_Auth_Get_Credential_Type(upload_data->authorization_module);
                // If the credential type is unknown then it means that we are using x509 because the certs need to get
                // passed down later in the process.
                if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_UNKNOWN || upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509)
                {
                    upload_data->cred_type = IOTHUB_CREDENTIAL_TYPE_X509;
                    upload_data->credentials.x509_credentials.x509certificate = NULL;
                    upload_data->credentials.x509_credentials.x509privatekey = NULL;
                    upload_data->credentials.x509_credentials.x509privatekeyType = NULL;
                    upload_data->credentials.x509_credentials.engine = NULL;
                }
                else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
                {
                    if (IoTHubClient_Auth_Get_x509_info(upload_data->authorization_module, &upload_data->credentials.x509_credentials.x509certificate, &upload_data->credentials.x509_credentials.x509privatekey) != 0)
                    {
                        LogError("Failed getting x509 certificate information");
                        free(upload_data->hostname);
                        free(upload_data);
                        upload_data = NULL;
                    }
                }
                else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
                {
                    upload_data->credentials.supplied_sas_token = IoTHubClient_Auth_Get_SasToken(upload_data->authorization_module, NULL, 0, EMPTY_STRING);
                    if (upload_data->credentials.supplied_sas_token == NULL)
                    {
                        LogError("Failed retrieving supplied sas token");
                        free(upload_data->hostname);
                        free(upload_data);
                        upload_data = NULL;
                    }
                }
            }
        }
    }

    return (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)upload_data;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_InitializeUpload(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, char** uploadCorrelationId, char** azureBlobSasUri)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || destinationFileName == NULL || uploadCorrelationId == NULL || azureBlobSasUri == NULL)
    {
        LogError("invalid argument detected handle=%p destinationFileName=%p uploadCorrelationId=%p azureBlobSasUri=%p",
            handle, destinationFileName, uploadCorrelationId, azureBlobSasUri);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        if (IoTHubClient_LL_UploadToBlob_GetBlobCredentialsFromIoTHub(
            upload_data, destinationFileName, uploadCorrelationId, azureBlobSasUri) != 0)
        {
            LogError("error in IoTHubClient_LL_UploadToBlob_GetBlobCredentialsFromIoTHub");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE IoTHubClient_LL_UploadToBlob_CreateContext(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* azureBlobSasUri)
{
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT* result;

    if (handle == NULL || azureBlobSasUri == NULL)
    {
        LogError("invalid argument detected handle=%p azureBlobSasUri=%p", handle, azureBlobSasUri);
        result = NULL;
    }
    else if ((result = createUploadToBlobContextInstance()) == NULL)
    {
        LogError("Failed creating Upload to Blob context");
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        if (parseAzureBlobSasUri(azureBlobSasUri, &result->blobStorageHostname, &result->blobStorageRelativePath) != 0)
        {
            LogError("failed parsing Blob Storage SAS URI");
            destroyUploadToBlobContextInstance(result);
            result = NULL;
        }
        else
        {
            result->u2bClientData = upload_data;

            result->blobHttpApiHandle = Blob_CreateHttpConnection(
                result->blobStorageHostname,
                result->u2bClientData->certificates,
                &(result->u2bClientData->http_proxy_options),
                result->u2bClientData->networkInterface,
                result->u2bClientData->blob_upload_timeout_millisecs);

            if (result->blobHttpApiHandle == NULL)
            {
                LogError("Failed creating HTTP connection to Azure Blob");
                destroyUploadToBlobContextInstance(result);
                result = NULL;
            }
        }
    }

    return result;
}

void IoTHubClient_LL_UploadToBlob_DestroyContext(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext)
{
    destroyUploadToBlobContextInstance(uploadContext);
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_PutBlock(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext, uint32_t blockNumber, const uint8_t* dataPtr, size_t dataSize)
{
    IOTHUB_CLIENT_RESULT result;

    if (uploadContext == NULL || dataPtr == NULL || dataSize == 0)
    {
        LogError("invalid argument uploadContext=%p, dataPtr=%p, dataSize=%zu", uploadContext, dataPtr, dataSize);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        BUFFER_HANDLE blockData;

        if ((blockData = BUFFER_create(dataPtr, dataSize)) == NULL)
        {
            LogError("Failed allocating buffer for Blob block data");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            unsigned int httpResponseStatus;

            if (Blob_PutBlock(
                uploadContext->blobHttpApiHandle,
                uploadContext->blobStorageRelativePath,
                blockNumber, blockData,
                uploadContext->blockIdList,
                &httpResponseStatus, NULL) != BLOB_OK)
            {
                LogError("Failed uploading Blob block data");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }

            BUFFER_delete(blockData);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_PutBlockList(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE uploadContext)
{
    IOTHUB_CLIENT_RESULT result;

    if (uploadContext == NULL)
    {
        LogError("invalid argument uploadContext=%p", uploadContext);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        unsigned int putBlockListHttpStatus = 0;

        // Do not PUT BLOCK LIST if result is not success (isSuccess == false)
        // Otherwise we could corrupt a blob with a partial update.
        if (Blob_PutBlockList(
                uploadContext->blobHttpApiHandle,
                uploadContext->blobStorageRelativePath,
                uploadContext->blockIdList, &putBlockListHttpStatus, NULL) != BLOB_OK)
        {
            LogError("Failed sending block ID list to Blob Storage (%u)", putBlockListHttpStatus);
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_NotifyCompletion(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* uploadCorrelationId, bool isSuccess, int responseCode, const char* responseMessage)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || uploadCorrelationId == NULL)
    {
        LogError("invalid argument detected handle=%p uploadCorrelationId=%p", handle, uploadCorrelationId);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        STRING_HANDLE response = STRING_construct_sprintf(RESPONSE_BODY_FORMAT,
                                                    uploadCorrelationId,
                                                    isSuccess ? RESPONSE_BODY_SUCCESS_BOOLEAN_STRING : RESPONSE_BODY_ERROR_BOOLEAN_STRING,
                                                    responseCode,
                                                    responseMessage);

        if(response == NULL)
        {
            LogError("STRING_construct_sprintf failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            size_t response_length = STRING_length(response);
            BUFFER_HANDLE responseToIoTHub = BUFFER_new();

            if (responseToIoTHub == NULL)
            {
                LogError("BUFFER_new failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (BUFFER_build(responseToIoTHub, (const unsigned char*)STRING_c_str(response), response_length) == 0)
                {
                    if (IoTHubClient_LL_UploadToBlob_NotifyIoTHubOfUploadCompletion(upload_data, responseToIoTHub) != 0)
                    {
                        LogError("IoTHubClient_LL_UploadToBlob_NotifyIoTHubOfUploadCompletion failed");
                        result = IOTHUB_CLIENT_ERROR;
                    }
                    else
                    {
                        result = IOTHUB_CLIENT_OK;
                    }
                }
                else
                {
                    LogError("Unable to BUFFER_build, can't perform IoTHubClient_LL_UploadToBlob_NotifyIoTHubOfUploadCompletion");
                    result = IOTHUB_CLIENT_ERROR;
                }

                BUFFER_delete(responseToIoTHub);
            }

            STRING_delete(response);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_UploadMultipleBlocks(IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE azureStorageClientHandle, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context)
{
    IOTHUB_CLIENT_RESULT result;

    if (azureStorageClientHandle == NULL || getDataCallbackEx == NULL)
    {
        LogError("invalid argument detected azureStorageClientHandle=%p getDataCallbackEx=%p", azureStorageClientHandle, getDataCallbackEx);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        unsigned int blockID = 0;
        bool isError;
        unsigned char const * blockDataPtr = NULL;
        size_t blockDataSize = 0;
        IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT getDataReturnValue;

        do
        {
            getDataReturnValue = getDataCallbackEx(FILE_UPLOAD_OK, &blockDataPtr, &blockDataSize, context);

            if (getDataReturnValue == IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT)
            {
                LogInfo("Upload to blob has been aborted by the user");
                isError = true;
                break;
            }
            else if (blockDataPtr == NULL || blockDataSize == 0)
            {
                // This is how the user indicates that there is no more data to be uploaded,
                // and the function can end with success result.
                isError = false;
                break;
            }
            else
            {
                if (blockDataSize > BLOCK_SIZE)
                {
                    LogError("tried to upload block of size %lu, max allowed size is %d", (unsigned long)blockDataSize, BLOCK_SIZE);
                    isError = true;
                    break;
                }
                else if (blockID >= MAX_BLOCK_COUNT)
                {
                    LogError("unable to upload more than %lu blocks in one blob", (unsigned long)MAX_BLOCK_COUNT);
                    isError = true;
                    break;
                }
                else if (IoTHubClient_LL_UploadToBlob_PutBlock(azureStorageClientHandle, blockID, blockDataPtr, blockDataSize) != IOTHUB_CLIENT_OK)
                {
                    LogError("failed uploading block to blob");
                    isError = true;
                    break;
                }

                blockID++;
            }
        }
        while(true);

        if (isError)
        {
            (void)getDataCallbackEx(FILE_UPLOAD_ERROR, NULL, NULL, context);
            result = IOTHUB_CLIENT_ERROR;
        }
        // Checking if blockID is greater than 0 guarantees that PUT BLOCK LIST will only
        // be attempted if at least one block has indeed been uploaded to Azure Storage.
        // This behavior follows the behavior of the original implementation of
        // Upload-to-Blob in azure-iot-sdk-c.
        else if (blockID > 0 && IoTHubClient_LL_UploadToBlob_PutBlockList(azureStorageClientHandle) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed to perform Azure Blob Put Block List operation");
            (void)getDataCallbackEx(FILE_UPLOAD_ERROR, NULL, NULL, context);
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            (void)getDataCallbackEx(FILE_UPLOAD_OK, NULL, NULL, context);
            result = IOTHUB_CLIENT_OK;
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
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
        {
            free(upload_data->credentials.x509_credentials.x509certificate);
            free(upload_data->credentials.x509_credentials.x509privatekey);
            free(upload_data->credentials.x509_credentials.x509privatekeyType);
            free(upload_data->credentials.x509_credentials.engine);
        }
        else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
        {
            free(upload_data->credentials.supplied_sas_token);
        }

        free((void*)upload_data->hostname);
        if (upload_data->certificates != NULL)
        {
            free(upload_data->certificates);
        }
        if (upload_data->http_proxy_options.host_address != NULL)
        {
            free((char *)upload_data->http_proxy_options.host_address);
        }
        if (upload_data->http_proxy_options.username != NULL)
        {
            free((char *)upload_data->http_proxy_options.username);
        }
        if (upload_data->http_proxy_options.password != NULL)
        {
            free((char *)upload_data->http_proxy_options.password);
        }
        if (upload_data->networkInterface != NULL)
        {
            free((char*)upload_data->networkInterface);
        }
        free(upload_data);
    }
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_SetOption(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* optionName, const void* value)
{
    IOTHUB_CLIENT_RESULT result;
    if (handle == NULL)
    {
        LogError("invalid argument detected: IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle=%p, const char* optionName=%s, const void* value=%p", handle, optionName, value);
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        if (strcmp(optionName, OPTION_X509_CERT) == 0)
        {
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
            {
                LogError("trying to set a x509 certificate while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                /*try to make a copy of the certificate*/
                char* temp;
                if (mallocAndStrcpy_s(&temp, value) != 0)
                {
                    LogError("unable to mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (upload_data->credentials.x509_credentials.x509certificate != NULL) /*free any previous values, if any*/
                    {
                        free(upload_data->credentials.x509_credentials.x509certificate);
                    }
                    upload_data->credentials.x509_credentials.x509certificate = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(optionName, OPTION_X509_PRIVATE_KEY) == 0)
        {
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
            {
                LogError("trying to set a x509 privatekey while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                /*try to make a copy of the privatekey*/
                char* temp;
                if (mallocAndStrcpy_s(&temp, value) != 0)
                {
                    LogError("unable to mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (upload_data->credentials.x509_credentials.x509privatekey != NULL) /*free any previous values, if any*/
                    {
                        free(upload_data->credentials.x509_credentials.x509privatekey);
                    }
                    upload_data->credentials.x509_credentials.x509privatekey = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(OPTION_OPENSSL_PRIVATE_KEY_TYPE, optionName) == 0)
        {
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
            {
                LogError("trying to set a x509 private key type while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else if (value == NULL)
            {
                LogError("NULL is a not a valid value for x509 private key type");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                int* temp = malloc(sizeof(int));
                if (temp == NULL)
                {
                    LogError("failure in malloc");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    *temp = *(int*)value;
                    if (upload_data->credentials.x509_credentials.x509privatekeyType != NULL)
                    {
                        free(upload_data->credentials.x509_credentials.x509privatekeyType);
                    }
                    upload_data->credentials.x509_credentials.x509privatekeyType = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(OPTION_OPENSSL_ENGINE, optionName) == 0)
        {
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
            {
                LogError("trying to set an openssl engine while the authentication scheme is not x509");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                char* temp;
                if (mallocAndStrcpy_s(&temp, value) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    if (upload_data->credentials.x509_credentials.engine != NULL)
                    {
                        free(upload_data->credentials.x509_credentials.engine);
                    }
                    upload_data->credentials.x509_credentials.engine = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(OPTION_TRUSTED_CERT, optionName) == 0)
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
                    if (upload_data->certificates != NULL)
                    {
                        free(upload_data->certificates);
                    }
                    upload_data->certificates = tempCopy;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(optionName, OPTION_HTTP_PROXY) == 0)
        {
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS *)value;

            if (proxy_options->host_address == NULL)
            {
                LogError("NULL host_address in proxy options");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
                (proxy_options->username != proxy_options->password))
            {
                LogError("Only one of username and password for proxy settings was NULL");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                if (upload_data->http_proxy_options.host_address != NULL)
                {
                    free((char *)upload_data->http_proxy_options.host_address);
                    upload_data->http_proxy_options.host_address = NULL;
                }
                if (upload_data->http_proxy_options.username != NULL)
                {
                    free((char *)upload_data->http_proxy_options.username);
                    upload_data->http_proxy_options.username = NULL;
                }
                if (upload_data->http_proxy_options.password != NULL)
                {
                    free((char *)upload_data->http_proxy_options.password);
                    upload_data->http_proxy_options.password = NULL;
                }

                upload_data->http_proxy_options.port = proxy_options->port;

                if (mallocAndStrcpy_s((char **)(&upload_data->http_proxy_options.host_address), proxy_options->host_address) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - upload_data->http_proxy_options.host_address");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if (proxy_options->username != NULL && mallocAndStrcpy_s((char **)(&upload_data->http_proxy_options.username), proxy_options->username) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - upload_data->http_proxy_options.username");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else if (proxy_options->password != NULL && mallocAndStrcpy_s((char **)(&upload_data->http_proxy_options.password), proxy_options->password) != 0)
                {
                    LogError("failure in mallocAndStrcpy_s - upload_data->http_proxy_options.password");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(optionName, OPTION_CURL_VERBOSE) == 0)
        {
            upload_data->curl_verbosity_level = ((*(bool*)value) == 0) ? UPLOADTOBLOB_CURL_VERBOSITY_OFF : UPLOADTOBLOB_CURL_VERBOSITY_ON;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(optionName, OPTION_BLOB_UPLOAD_TIMEOUT_SECS) == 0)
        {
            upload_data->blob_upload_timeout_millisecs = 1000 * (*(size_t*)value);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(optionName, OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB) == 0)
        {
            if (value == NULL)
            {
                LogError("NULL is not a valid value for networkInterface option");
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
                    if (upload_data->networkInterface != NULL)
                    {
                        free((char*)upload_data->networkInterface);
                    }
                    upload_data->networkInterface = tempCopy;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else if (strcmp(optionName, OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION) == 0)
        {
            upload_data->tls_renegotiation = *((bool*)(value));
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
    }
    return result;
}

#endif /*DONT_USE_UPLOADTOBLOB*/
