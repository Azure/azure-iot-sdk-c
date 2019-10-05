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
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/urlencode.h"

#include "azure_uhttp_c/uhttp.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/platform.h"

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
#define DEFAULT_HTTPS_PORT          443

/*Codes_SRS_IOTHUBCLIENT_LL_02_085: [ IoTHubClient_LL_UploadToBlob shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters: ]*/
#define FILE_UPLOAD_FAILED_BODY "{ \"isSuccess\":false, \"statusCode\":-1,\"statusDescription\" : \"client not able to connect with the server\" }"
#define FILE_UPLOAD_ABORTED_BODY "{ \"isSuccess\":false, \"statusCode\":-1,\"statusDescription\" : \"file upload aborted\" }"
#define INDEFINITE_TIME                            ((time_t)-1)
#define MAX_CLOSE_DOWORK_COUNT                  10

static const char* const EMPTY_STRING = "";
static const char* const HEADER_AUTHORIZATION = "Authorization";
static const char* const HEADER_APP_JSON = "application/json";

typedef struct UPLOADTOBLOB_X509_CREDENTIALS_TAG
{
    char* x509certificate;
    char* x509privatekey;
} UPLOADTOBLOB_X509_CREDENTIALS;

typedef enum UPOADTOBLOB_CURL_VERBOSITY_TAG
{
    UPOADTOBLOB_CURL_VERBOSITY_UNSET,
    UPOADTOBLOB_CURL_VERBOSITY_ON,
    UPOADTOBLOB_CURL_VERBOSITY_OFF
} UPOADTOBLOB_CURL_VERBOSITY;

typedef enum HTTP_CONNECTION_STATE_TAG
{
    HTTP_STATE_DISCONNECTED,
    HTTP_STATE_CONNECTING,
    HTTP_STATE_CONNECTED,
    HTTP_STATE_REQUEST_SENT,
    HTTP_STATE_REQUEST_RECV,
    HTTP_STATE_COMPLETE,
    HTTP_STATE_ERROR
} HTTP_CONNECTION_STATE;

typedef struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA_TAG
{
    const char* deviceId;
    char* hostname;
    HTTP_CONNECTION_STATE http_state;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
    IOTHUB_CREDENTIAL_TYPE cred_type;
    union
    {
        UPLOADTOBLOB_X509_CREDENTIALS x509_credentials;
        char* supplied_sas_token;
    } credentials;

    char* certificates;
    HTTP_PROXY_OPTIONS http_proxy_options;
    UPOADTOBLOB_CURL_VERBOSITY curl_verbosity_level;
    size_t blob_upload_timeout_secs;
    TICK_COUNTER_HANDLE tick_counter;
    BUFFER_HANDLE response_data;
    unsigned int status_code;
}IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA;

typedef struct BLOB_UPLOAD_CONTEXT_TAG
{
    const unsigned char* blobSource; /* source to upload */
    size_t blobSourceSize; /* size of the source */
    size_t remainingSizeToUpload; /* size not yet uploaded */
} BLOB_UPLOAD_CONTEXT;

static int construct_sas_token(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, const char* resource)
{
    int result;
    time_t curr_time;
    if ((curr_time = get_time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failure getting time");
        result = MU_FAILURE;
    }
    else
    {
        size_t expiry = (size_t)(difftime(curr_time, 0) + 3600);
        if (upload_data->credentials.supplied_sas_token != NULL)
        {
            free(upload_data->credentials.supplied_sas_token);
        }
        if ((upload_data->credentials.supplied_sas_token = IoTHubClient_Auth_Get_SasToken(upload_data->authorization_module, resource, expiry, NULL)) == NULL)
        {
            LogError("Failure constructing supplied sas token");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static void on_http_connected(void* callback_ctx, HTTP_CALLBACK_REASON connect_result)
{
    if (callback_ctx != NULL)
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)callback_ctx;
        if (connect_result == HTTP_CALLBACK_REASON_OK)
        {
            upload_data->http_state = HTTP_STATE_CONNECTED;
        }
        else
        {
            upload_data->http_state = HTTP_STATE_ERROR;
        }
    }
}

static void on_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    if (callback_ctx != NULL)
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)callback_ctx;
        upload_data->http_state = HTTP_STATE_ERROR;
        LogError("Failure encountered in http");
    }
    else
    {
        LogError("Failure encountered in http %d", error_result);
    }
}

static void on_http_reply_recv(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_len, unsigned int status_code, HTTP_HEADERS_HANDLE response_header)
{
    (void)response_header;
    if (callback_ctx != NULL)
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)callback_ctx;

        //update HTTP state
        if (request_result == HTTP_CALLBACK_REASON_OK)
        {
            // Store the http Status code
            upload_data->status_code = status_code;
            if (status_code >= 200 && status_code <= 299)
            {
                upload_data->http_state = HTTP_STATE_REQUEST_RECV;
            }
            else
            {
                upload_data->http_state = HTTP_STATE_ERROR;
                LogError("Failure encountered status code: %d\n%.*s", (int)status_code, (int)content_len, content);
            }

            // if there is a json response save it to process later
            if (content != NULL  && upload_data->http_state != HTTP_STATE_ERROR)
            {
                // Clear the buffer
                (void)BUFFER_unbuild(upload_data->response_data);
                if (BUFFER_build(upload_data->response_data, content, content_len) != 0)
                {
                    upload_data->http_state = HTTP_STATE_ERROR;
                }
            }
        }
        else
        {
            upload_data->http_state = HTTP_STATE_ERROR;
        }
    }
    else
    {
        LogError("Invalid callback context");
    }
}

static HTTP_CLIENT_HANDLE create_http_client(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data)
{
    HTTP_CLIENT_HANDLE result;
    // Create uhttp
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy;
    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = upload_data->hostname;
    tls_io_config.port = DEFAULT_HTTPS_PORT;

    // Setup proxy
    if (upload_data->http_proxy_options.host_address != NULL)
    {
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
        http_proxy.hostname = upload_data->hostname;
        http_proxy.port = DEFAULT_HTTPS_PORT;
        http_proxy.proxy_hostname = upload_data->http_proxy_options.host_address;
        http_proxy.proxy_port = upload_data->http_proxy_options.port;
        http_proxy.username = upload_data->http_proxy_options.username;
        http_proxy.password = upload_data->http_proxy_options.password;

        tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
        tls_io_config.underlying_io_parameters = &http_proxy;
    }

    const IO_INTERFACE_DESCRIPTION* interface_desc = platform_get_default_tlsio();
    if (interface_desc == NULL)
    {
        LogError("platform default tlsio is NULL");
        result = NULL;
    }
    else if ((result = uhttp_client_create(interface_desc, &tls_io_config, on_http_error, upload_data)) == NULL)
    {
        LogError("Failed creating http object");
    }
    else if (upload_data->certificates != NULL && uhttp_client_set_trusted_cert(result, upload_data->certificates) != HTTP_CLIENT_OK)
    {
        LogError("Failed setting trusted cert");
        uhttp_client_destroy(result);
        result = NULL;
    }
    else if ((upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC) &&
        ((IoTHubClient_Auth_Set_xio_Certificate(upload_data->authorization_module, uhttp_client_get_underlying_xio(result))) != 0)
        )
    {
        LogError("unable to uhttp_client_set_X509_cert for x509 certificate");
        uhttp_client_destroy(result);
        result = NULL;
    }
    else if (upload_data->curl_verbosity_level != UPOADTOBLOB_CURL_VERBOSITY_UNSET && uhttp_client_set_trace(result, true, true) != HTTP_CLIENT_OK)
    {
        LogError("Failed setting trace");
        uhttp_client_destroy(result);
        result = NULL;
    }
    else if (uhttp_client_open(result, upload_data->hostname, DEFAULT_HTTPS_PORT, on_http_connected, upload_data) != HTTP_CLIENT_OK)
    {
        LogError("Failed opening http url %s", upload_data->hostname);
        uhttp_client_destroy(result);
        result = NULL;
    }
    else
    {

    }
    return result;
}

static void on_http_close_callback(void* callback_ctx)
{
    if (callback_ctx == NULL)
    {
        LogError("NULL callback_ctx specified in close callback");
    }
    else
    {
        int* close_value = (int*)callback_ctx;
        *close_value = 1;
    }
}

static void close_http_client(HTTP_CLIENT_HANDLE http_client)
{
    int close_value = 0;
    uhttp_client_close(http_client, on_http_close_callback, &close_value);
    for (size_t index = 0; index < MAX_CLOSE_DOWORK_COUNT && close_value == 0; index++)
    {
        uhttp_client_dowork(http_client);
    }
}

static int send_http_request(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_client, HTTP_CLIENT_HANDLE http_client, const char* relative_path, HTTP_HEADERS_HANDLE request_header, STRING_HANDLE blob_data)
{
    int result;
    const unsigned char* data = (const unsigned char*)STRING_c_str(blob_data);
    size_t data_len = STRING_length(blob_data);
    if (uhttp_client_execute_request(http_client, HTTP_CLIENT_REQUEST_POST, relative_path, request_header, data, data_len, on_http_reply_recv, upload_client) != HTTP_CLIENT_OK)
    {
        LogError("Failure executing http request");
        upload_client->http_state = HTTP_STATE_ERROR;
        result = MU_FAILURE;
    }
    else
    {
        upload_client->http_state = HTTP_STATE_REQUEST_SENT;
        result = 0;
        do
        {
            uhttp_client_dowork(http_client);
            if (upload_client->http_state == HTTP_STATE_REQUEST_RECV)
            {
                upload_client->http_state = HTTP_STATE_COMPLETE;
                result = 0;
            }
            else if (upload_client->http_state == HTTP_STATE_ERROR)
            {
                result = MU_FAILURE;
                LogError("Failure encountered executing http request");
            }
        } while (upload_client->http_state != HTTP_STATE_COMPLETE && upload_client->http_state != HTTP_STATE_ERROR);

        size_t len = BUFFER_length(upload_client->response_data);
        if (result != 0 || len == 0)
        {
            result = MU_FAILURE;
            LogError("Failure recieved from data");
        }
    }
    return result;
}

static HTTP_HEADERS_HANDLE construct_http_header(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data)
{
    HTTP_HEADERS_HANDLE result;
    if ((result = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("Unable to allocate HTTPHeaders");
    }
    else
    {
        if (HTTPHeaders_AddHeaderNameValuePair(result, "Content-Type", HEADER_APP_JSON) != HTTP_HEADERS_OK)
        {
            LogError("Failure setting http header Content-Type");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else if (HTTPHeaders_AddHeaderNameValuePair(result, "Accept", HEADER_APP_JSON) != HTTP_HEADERS_OK)
        {
            LogError("Failure setting http header Accept");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else if (HTTPHeaders_AddHeaderNameValuePair(result, "User-Agent", "iothubclient/" IOTHUB_SDK_VERSION) != HTTP_HEADERS_OK)
        {
            LogError("Failure setting http header User-Agent");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else if ((upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC) &&
            (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_AUTHORIZATION, EMPTY_STRING) != HTTP_HEADERS_OK))
        {
            LogError("Failure setting http header Authorization");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else if ((upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY) &&
            (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_AUTHORIZATION, upload_data->credentials.supplied_sas_token) != HTTP_HEADERS_OK))
        {
            LogError("Failure setting http header Authorization");
            HTTPHeaders_Free(result);
            result = NULL;
        }
    }
    return result;
}

static int parse_result_json(const char* json_response, STRING_HANDLE correlation_id, STRING_HANDLE sas_uri)
{
    int result;

    JSON_Object* json_obj;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_081: [ Otherwise, IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall use parson to extract and save the following information from the response buffer: correlationID and SasUri. ]*/
    JSON_Value* json = json_parse_string(json_response);
    if (json == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
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
            /*Codes_SRS_IOTHUBCLIENT_LL_32_008: [ The returned file name shall be URL encoded before passing back to the cloud. ]*/
            else if ((filename = URL_EncodeString(json_blob_name)) == NULL)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_32_009: [ If URL_EncodeString fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                LogError("unable to URL encode of filename");
                result = MU_FAILURE;
            }
            else
            {
                if (STRING_sprintf(sas_uri, "https://%s/%s/%s%s", json_hostname, json_container_name, STRING_c_str(filename), json_sas_token) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("unable to construct uri string");
                    result = MU_FAILURE;
                }
                else if (STRING_copy(correlation_id, json_corr_id) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("unable to copy correlation Id");
                    result = MU_FAILURE;
                    STRING_empty(sas_uri);
                }
                else
                {
                    result = 0;
                }
                STRING_delete(filename);
            }
        }
        json_value_free(json);
    }
    return result;
}

/*returns 0 when correlationId, sasUri contain data*/
static int IoTHubClient_LL_UploadToBlob_step1and2(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, HTTP_CLIENT_HANDLE http_client, HTTP_HEADERS_HANDLE request_header, const char* destinationFileName, STRING_HANDLE correlationId, STRING_HANDLE sasUri)
{
    int result;
    STRING_HANDLE relativePath;
    STRING_HANDLE blobName;

    /*Codes_SRS_IOTHUBCLIENT_LL_02_066: [ IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall create an HTTP relative path formed from "/devices/" + deviceId + "/files/" + "?api-version=API_VERSION". ]*/
    if ((relativePath = STRING_construct_sprintf("/devices/%s/files/%s", upload_data->deviceId, API_VERSION)) == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_067: [ If creating the relativePath fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("Failure constructing string");
        result = MU_FAILURE;
    }
    /*Codes_SRS_IOTHUBCLIENT_LL_32_001: [ IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall create a JSON string formed from "{ \"blobName\": \" + destinationFileName + "\" }" */
    else if ((blobName = STRING_construct_sprintf("{ \"blobName\": \"%s\" }", destinationFileName)) == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_32_002: [ If creating the JSON string fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("Failure constructing string");
        STRING_delete(relativePath);
        result = MU_FAILURE;
    }
    else
    {
        // set the result to error by default
        switch (upload_data->cred_type)
        {
            default:
            {
                /*wasIoTHubRequestSuccess takes care of the return value*/
                LogError("Internal Error: unexpected value in auth schema = %d", upload_data->cred_type);
                result = MU_FAILURE;
                break;
            }
            case IOTHUB_CREDENTIAL_TYPE_X509_ECC:
            case IOTHUB_CREDENTIAL_TYPE_X509:
            case IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY:
            case IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH:
            case IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN:
            {
                if (send_http_request(upload_data, http_client, STRING_c_str(relativePath), request_header, blobName) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_076: [ If HTTPAPIEX_ExecuteRequest call fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    result = MU_FAILURE;
                    LogError("unable to HTTPAPIEX_ExecuteRequest");
                }
                else
                {
                    result = 0;
                }
                break;
            }
        }
        if (result == 0)
        {
            const unsigned char*responseContent_u_char = BUFFER_u_char(upload_data->response_data);
            size_t responseContent_length = BUFFER_length(upload_data->response_data);
            STRING_HANDLE responseAsString = STRING_from_byte_array(responseContent_u_char, responseContent_length);
            if (responseAsString == NULL)
            {
                result = MU_FAILURE;
                LogError("unable to get the response as string");
            }
            else
            {
                if (parse_result_json(STRING_c_str(responseAsString), correlationId, sasUri) != 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_082: [ If extracting and saving the correlationId or SasUri fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
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
        STRING_delete(blobName);
        STRING_delete(relativePath);
    }
    return result;
}

/*returns 0 when the IoTHub has been informed about the file upload status*/
static int IoTHubClient_LL_UploadToBlob_step3(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, const char* correlationId, HTTP_CLIENT_HANDLE http_client, HTTP_HEADERS_HANDLE request_header, STRING_HANDLE messageBody)
{
    int result;
    /*here is step 3. depending on the outcome of step 2 it needs to inform IoTHub about the file upload status*/
    /*if step 1 failed, there's nothing that step 3 needs to report.*/
    /*this POST "tries" to happen*/

    /*Codes_SRS_IOTHUBCLIENT_LL_02_085: [ IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall use the same authorization as step 1. to prepare and perform a HTTP request with the following parameters: ]*/
    STRING_HANDLE relativePathNotification = STRING_construct_sprintf("/devices/%s/files/notifications/%s%s", upload_data->deviceId, correlationId, API_VERSION);
    if (relativePathNotification == NULL)
    {
        result = MU_FAILURE;
        LogError("Failure constructing string");
    }
    else if (uhttp_client_open(http_client, upload_data->hostname, DEFAULT_HTTPS_PORT, NULL, NULL) != HTTP_CLIENT_OK)
    {
        LogError("Failed opening http url %s", upload_data->hostname);
        STRING_delete(relativePathNotification);
        result = BLOB_HTTP_ERROR;
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_086: [ If performing the HTTP request fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
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
                if (send_http_request(upload_data, http_client, STRING_c_str(relativePathNotification), request_header, messageBody) != 0)
                {
                    LogError("unable to execute HTTPAPIEX_ExecuteRequest");
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
                // New scope for sas token
                STRING_HANDLE uriResource = STRING_construct_sprintf("%s/devices/%s/files/notifications", upload_data->hostname, upload_data->deviceId);
                if (uriResource == NULL)
                {
                    LogError("Failure constructing string");
                    result = MU_FAILURE;
                }
                else
                {
                    HTTP_HEADERS_RESULT hdr_result;
                    if (construct_sas_token(upload_data, STRING_c_str(uriResource)) != 0)
                    {
                        LogError("Failure constructing string");
                        result = MU_FAILURE;
                    }
                    else if ((hdr_result = HTTPHeaders_ReplaceHeaderNameValuePair(request_header, HEADER_AUTHORIZATION, upload_data->credentials.supplied_sas_token)) != HTTP_HEADERS_OK)
                    {
                        LogError("Failure Replacing auth header");
                        result = MU_FAILURE;
                    }
                    else if (send_http_request(upload_data, http_client, STRING_c_str(relativePathNotification), request_header, messageBody) != 0)
                    {
                        LogError("unable to execute HTTPAPIEX_ExecuteRequest");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                    STRING_delete(uriResource);
                }
                break;
            }
        }
        STRING_delete(relativePathNotification);
    }
    return result;
}

// this callback splits the source data into blocks to be fed to IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)_Impl
static IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT FileUpload_GetData_Callback(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context)
{
    BLOB_UPLOAD_CONTEXT* uploadContext = (BLOB_UPLOAD_CONTEXT*) context;

    if (data == NULL || size == NULL)
    {
        // This is the last call, nothing to do
    }
    else if (result != FILE_UPLOAD_OK)
    {
        // Last call failed
        *data = NULL;
        *size = 0;
    }
    else if (uploadContext->remainingSizeToUpload == 0)
    {
        // Everything has been uploaded
        *data = NULL;
        *size = 0;
    }
    else
    {
        // Upload next block
        size_t thisBlockSize = (uploadContext->remainingSizeToUpload > BLOCK_SIZE) ? BLOCK_SIZE : uploadContext->remainingSizeToUpload;
        *data = (unsigned char*)uploadContext->blobSource + (uploadContext->blobSourceSize - uploadContext->remainingSizeToUpload);
        *size = thisBlockSize;
        uploadContext->remainingSizeToUpload -= thisBlockSize;
    }

    return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK;
}

static int initiate_blob_upload(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, const char* dest_filename, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context)
{
    int result;
    // Codes_SRS_IOTHUBCLIENT_LL_02_064: [ IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall create an HTTPAPIEX_HANDLE to the IoTHub hostname. ]
    HTTP_CLIENT_HANDLE http_client = create_http_client(upload_data);
    // Codes_SRS_IOTHUBCLIENT_LL_02_065: [ If creating the HTTPAPIEX_HANDLE fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]
    if (http_client == NULL)
    {
        LogError("unable to HTTPAPIEX_Create");
        result = IOTHUB_CLIENT_ERROR;
    }
    else
    {
        STRING_HANDLE sas_uri;
        STRING_HANDLE correlation_id;
        HTTP_HEADERS_HANDLE request_header;
        if ((correlation_id = STRING_new()) == NULL)
        {
            LogError("unable to STRING_new");
            result = IOTHUB_CLIENT_ERROR;
        }
        else if ((sas_uri = STRING_new()) == NULL)
        {
            LogError("unable to create sas uri");
            result = IOTHUB_CLIENT_ERROR;
            STRING_delete(correlation_id);
        }
        else if ((request_header = construct_http_header(upload_data)) == NULL)
        {
            LogError("unable to HTTPHeaders_Alloc");
            STRING_delete(correlation_id);
            STRING_delete(sas_uri);
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            // do step 1
            if (IoTHubClient_LL_UploadToBlob_step1and2(upload_data, http_client, request_header, dest_filename, correlation_id, sas_uri) != 0)
            {
                LogError("error in IoTHubClient_LL_UploadToBlob_step1");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                BUFFER_HANDLE hub_response;
                STRING_HANDLE hub_content;

                close_http_client(http_client);

                if ((hub_response = BUFFER_new() ) == NULL)
                {
                    LogError("Failure creating response buffer");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    // Codes_SRS_IOTHUBCLIENT_LL_02_083: [ IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall call Blob_UploadFromSasUri and capture the HTTP return code and HTTP body. ]
                    BLOB_RESULT uploadMultipleBlocksResult = Blob_UploadMultipleBlocksFromSasUri(STRING_c_str(sas_uri), upload_data->certificates, &upload_data->http_proxy_options, getDataCallbackEx, context, &upload_data->status_code, hub_response);
                    if (uploadMultipleBlocksResult == BLOB_ABORTED)
                    {
                        // Codes_SRS_IOTHUBCLIENT_LL_99_008: [ If step 2 is aborted by the client, then the HTTP message body shall look like:  ]
                        LogInfo("Blob_UploadFromSasUri aborted file upload");

                        if ((hub_content = STRING_construct(FILE_UPLOAD_ABORTED_BODY)) == NULL)
                        {
                            if (IoTHubClient_LL_UploadToBlob_step3(upload_data, STRING_c_str(correlation_id), http_client, request_header, hub_content) != 0)
                            {
                                LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                                result = IOTHUB_CLIENT_ERROR;
                            }
                            else
                            {
                                // Codes_SRS_IOTHUBCLIENT_LL_99_009: [ If step 2 is aborted by the client and if step 3 succeeds, then `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` shall return `IOTHUB_CLIENT_OK`. ]
                                result = IOTHUB_CLIENT_OK;
                            }
                            STRING_delete(hub_content);
                        }
                        else
                        {
                            LogError("Unable to BUFFER_build, can't perform IoTHubClient_LL_UploadToBlob_step3");
                            result = IOTHUB_CLIENT_ERROR;
                        }
                    }
                    else if (uploadMultipleBlocksResult != BLOB_OK)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_084: [ If Blob_UploadFromSasUri fails then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                        LogError("unable to Blob_UploadFromSasUri");

                        STRING_HANDLE content_data;
                        /*do step 3*/ /*try*/
                        // Codes_SRS_IOTHUBCLIENT_LL_02_091: [ If step 2 fails without establishing an HTTP dialogue, then the HTTP message body shall look like: ]
                        if ((content_data = STRING_construct(FILE_UPLOAD_FAILED_BODY)) != NULL)
                        {
                            if (IoTHubClient_LL_UploadToBlob_step3(upload_data, STRING_c_str(correlation_id), http_client, request_header, content_data) != 0)
                            {
                                LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                            }
                            STRING_delete(content_data);
                        }
                        result = IOTHUB_CLIENT_ERROR;
                    }
                    else
                    {
                        // must make a json
                        unsigned char* blob_resp = BUFFER_u_char(hub_response);
                        size_t resp_len = BUFFER_length(hub_response);
                        STRING_HANDLE req_string;

                        if (blob_resp == NULL)
                        {
                            req_string = STRING_construct_sprintf("{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":""}", ((upload_data->status_code < 300) ? "true" : "false"), upload_data->status_code, resp_len, blob_resp);
                        }
                        else
                        {
                            req_string = STRING_construct_sprintf("{\"isSuccess\":%s, \"statusCode\":%d, \"statusDescription\":\"%.*s\"}", ((upload_data->status_code < 300) ? "true" : "false"), upload_data->status_code, resp_len, blob_resp);
                        }
                        if (req_string == NULL)
                        {
                            LogError("Failure constructing string");
                            result = IOTHUB_CLIENT_ERROR;
                        }
                        else
                        {
                            // do again snprintf

                            if (IoTHubClient_LL_UploadToBlob_step3(upload_data, STRING_c_str(correlation_id), http_client, request_header, req_string) != 0)
                            {
                                LogError("IoTHubClient_LL_UploadToBlob_step3 failed");
                                result = IOTHUB_CLIENT_ERROR;
                            }
                            else
                            {
                                result = (upload_data->status_code < 300) ? IOTHUB_CLIENT_OK : IOTHUB_CLIENT_ERROR;
                            }
                            STRING_delete(req_string);
                        }
                    }
                    BUFFER_delete(hub_response);
                }
            }
            STRING_delete(sas_uri);
            STRING_delete(correlation_id);
            HTTPHeaders_Free(request_header);
        }
        uhttp_client_destroy(http_client);
    }

    // Codes_SRS_IOTHUBCLIENT_LL_99_003: [ If `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` return `IOTHUB_CLIENT_OK`, it shall call `getDataCallbackEx` with `result` set to `FILE_UPLOAD_OK`, and `data` and `size` set to NULL. ]
    // Codes_SRS_IOTHUBCLIENT_LL_99_004: [ If `IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex)` does not return `IOTHUB_CLIENT_OK`, it shall call `getDataCallbackEx` with `result` set to `FILE_UPLOAD_ERROR`, and `data` and `size` set to NULL. ]
    (void)getDataCallbackEx(result == IOTHUB_CLIENT_OK ? FILE_UPLOAD_OK : FILE_UPLOAD_ERROR, NULL, NULL, context);

    return result;
}

/*static HTTPAPIEX_RESULT set_transfer_timeout(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data, HTTPAPIEX_HANDLE iotHubHttpApiExHandle)
{
    HTTPAPIEX_RESULT result;
    if (upload_data->blob_upload_timeout_secs != 0)
    {
        // Convert the timeout to milliseconds for curl
        long http_timeout = (long)upload_data->blob_upload_timeout_secs * 1000;
        result = HTTPAPIEX_SetOption(iotHubHttpApiExHandle, OPTION_HTTP_TIMEOUT, &http_timeout);
    }
    else
    {
        result = HTTPAPIEX_OK;
    }
    return result;
}*/

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob_Impl(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, const unsigned char* source, size_t size)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || destinationFileName == NULL)
    {
        LogError("Invalid parameter handle:%p destinationFileName:%p", handle, destinationFileName);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    /*Codes_SRS_IOTHUBCLIENT_LL_02_063: [ If source is NULL and size is greater than 0 then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    else if (source == NULL && size > 0)
    {
        LogError("Invalid source and size combination: source=%p size=%lu", source, (unsigned long)size);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_99_001: [ `IoTHubClient_LL_UploadToBlob` shall create a struct containing the `source`, the `size`, and the remaining size to upload.]*/
        BLOB_UPLOAD_CONTEXT context;
        context.blobSource = source;
        context.blobSourceSize = size;
        context.remainingSizeToUpload = size;

        /*Codes_SRS_IOTHUBCLIENT_LL_99_002: [ `IoTHubClient_LL_UploadToBlob` shall call `IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl` with `FileUpload_GetData_Callback` as `getDataCallbackEx` and pass the struct created at step SRS_IOTHUBCLIENT_LL_99_001 as `context` ]*/
        //result = IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl(handle, destinationFileName, FileUpload_GetData_Callback, &context);
        if (initiate_blob_upload((IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle, destinationFileName, FileUpload_GetData_Callback, &context) != 0)
        {
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadMultipleBlocksToBlob_Impl(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE handle, const char* destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX getDataCallbackEx, void* context)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_LL_02_061: [ If handle is NULL then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_02_062: [ If destinationFileName is NULL then IoTHubClient_LL_UploadMultipleBlocksToBlob(Ex) shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    if (handle == NULL || destinationFileName == NULL || getDataCallbackEx == NULL)
    {
        LogError("invalid argument detected handle=%p destinationFileName=%p getDataCallbackEx=%p", handle, destinationFileName, getDataCallbackEx);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;
        if (initiate_blob_upload(upload_data, destinationFileName, getDataCallbackEx, context) != 0)
        {
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
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
        if ((upload_data = malloc(sizeof(IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA))) == NULL)
        {
            LogError("Failed malloc allocation");
            // return as is
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
            else if ((upload_data->tick_counter = tickcounter_create()) == NULL)
            {
                LogError("Failed retrieving device ID");
                free(upload_data->hostname);
                free(upload_data);
                upload_data = NULL;
            }
            else if ((upload_data->response_data = BUFFER_new()) == NULL)
            {
                LogError("Failed retrieving device ID");
                tickcounter_destroy(upload_data->tick_counter);
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
                }
                else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
                {
                    if (IoTHubClient_Auth_Get_x509_info(upload_data->authorization_module, &upload_data->credentials.x509_credentials.x509certificate, &upload_data->credentials.x509_credentials.x509privatekey) != 0)
                    {
                        LogError("Failed getting x509 certificate information");
                        free(upload_data->hostname);
                        BUFFER_delete(upload_data->response_data);
                        tickcounter_destroy(upload_data->tick_counter);
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
                        BUFFER_delete(upload_data->response_data);
                        tickcounter_destroy(upload_data->tick_counter);
                        free(upload_data);
                        upload_data = NULL;
                    }
                }
                else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
                {
                    STRING_HANDLE uri_resource = STRING_construct_sprintf("%s/devices/%s", upload_data->hostname, upload_data->deviceId);
                    if (uri_resource == NULL)
                    {
                        LogError("Failure constructing supplied sas token");
                        free(upload_data->hostname);
                        BUFFER_delete(upload_data->response_data);
                        tickcounter_destroy(upload_data->tick_counter);
                        free(upload_data);
                        upload_data = NULL;
                    }
                    else
                    {
                        if (construct_sas_token(upload_data, STRING_c_str(uri_resource)) != 0)
                        {
                            LogError("Failure generating sas token");
                            free(upload_data->hostname);
                            BUFFER_delete(upload_data->response_data);
                            tickcounter_destroy(upload_data->tick_counter);
                            free(upload_data);
                            upload_data = NULL;
                        }
                        STRING_delete(uri_resource);
                    }
                }
            }
        }
    }
    return (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE)upload_data;
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
        }
        else if (upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN || upload_data->cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY)
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
        if (upload_data->response_data != NULL)
        {
            BUFFER_delete(upload_data->response_data);
        }
        tickcounter_destroy(upload_data->tick_counter);
        free(upload_data);
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
        IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA* upload_data = (IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA*)handle;

        /*Codes_SRS_IOTHUBCLIENT_LL_02_100: [ x509certificate - then value then is a null terminated string that contains the x509 certificate. ]*/
        if (strcmp(optionName, OPTION_X509_CERT) == 0)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
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
                    if (upload_data->credentials.x509_credentials.x509certificate != NULL) /*free any previous values, if any*/
                    {
                        free(upload_data->credentials.x509_credentials.x509certificate);
                    }
                    upload_data->credentials.x509_credentials.x509certificate = temp;
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        /*Codes_SRS_IOTHUBCLIENT_LL_02_101: [ x509privatekey - then value is a null terminated string that contains the x509 privatekey. ]*/
        else if (strcmp(optionName, OPTION_X509_PRIVATE_KEY) == 0)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_109: [ If the authentication scheme is NOT x509 then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            if (upload_data->cred_type != IOTHUB_CREDENTIAL_TYPE_X509)
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
                    if (upload_data->credentials.x509_credentials.x509privatekey != NULL) /*free any previous values, if any*/
                    {
                        free(upload_data->credentials.x509_credentials.x509privatekey);
                    }
                    upload_data->credentials.x509_credentials.x509privatekey = temp;
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
            upload_data->curl_verbosity_level = ((*(bool*)value) == 0) ? UPOADTOBLOB_CURL_VERBOSITY_OFF : UPOADTOBLOB_CURL_VERBOSITY_ON;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(optionName, OPTION_BLOB_UPLOAD_TIMEOUT_SECS) == 0)
        {
            upload_data->blob_upload_timeout_secs = *(size_t*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_102: [ If an unknown option is presented then IoTHubClient_LL_UploadToBlob_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
    }
    return result;
}

#endif /*DONT_USE_UPLOADTOBLOB*/


