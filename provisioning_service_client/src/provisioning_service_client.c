// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "azure_uhttp_c/uhttp.h"

#include "provisioning_service_client.h"
#include "provisioning_sc_models_serializer.h"
#include "provisioning_sc_shared_helpers.h"

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

//consider substructure representing SharedAccessSignature?
typedef struct PROVISIONING_SERVICE_CLIENT_TAG
{
    //Connection details
    char* provisioning_service_uri;
    char* key_name;
    char* access_key;

    //Connection data
    HTTP_CONNECTION_STATE http_state;
    char* response;

    //Connection options
    TRACING_STATUS tracing;
    HTTP_PROXY_OPTIONS* proxy_options;
    char* certificate;

} PROV_SERVICE_CLIENT;

typedef char*(*VECTOR_SERIALIZE_TO_JSON)(void*);
typedef void*(*VECTOR_DESERIALIZE_FROM_JSON)(char*);
typedef char*(*VECTOR_GET_ID)(void*);
typedef char*(*VECTOR_GET_ETAG)(void*);
typedef void(*VECTOR_DESTROY)(void*);

typedef struct HANDLE_FUNCTION_VECTOR_TAG
{
    //char* (*serializeToJson)(void*);
    //void* (*deserializeFromJson)(char*);
    //const char* (*getId)(void*);
    //const char* (*getEtag)(void*);
    //void(*destroy)(void*);
    VECTOR_SERIALIZE_TO_JSON serializeToJson;
    VECTOR_DESERIALIZE_FROM_JSON deserializeFromJson;
    VECTOR_GET_ID getId;
    VECTOR_GET_ETAG getEtag;
    VECTOR_DESTROY destroy;
} HANDLE_FUNCTION_VECTOR;

#define IOTHUBHOSTNAME "HostName"
#define IOTHUBSHAREDACESSKEYNAME "SharedAccessKeyName"
#define IOTHUBSHAREDACESSKEY "SharedAccessKey"

#define PROVISIONING_SERVICE_API_VERSION    "2017-11-15"
#define ENROLL_GROUP_PROVISION_PATH_FMT     "/enrollmentGroups/%s?api-version=%s"
#define INDV_ENROLL_PROVISION_PATH_FMT      "/enrollments/%s?api-version=%s"
#define HEADER_KEY_AUTHORIZATION            "Authorization"
#define HEADER_KEY_IF_MATCH                 "If-Match"
#define HEADER_KEY_USER_AGENT               "UserAgent"
#define HEADER_KEY_ACCEPT                   "Accept"
#define HEADER_KEY_CONTENT_TYPE             "Content-Type"
#define HEADER_VALUE_USER_AGENT             "iothub_dps_prov_client/1.0"
#define HEADER_VALUE_ACCEPT                 "application/json"
#define HEADER_VALUE_CONTENT_TYPE           "application/json; charset=utf-8"

#define DEFAULT_HTTPS_PORT          443
#define UID_LENGTH                  37
#define SAS_TOKEN_DEFAULT_LIFETIME  3600
#define EPOCH_TIME_T_VALUE          (time_t)0

static HANDLE_FUNCTION_VECTOR getVector_individualEnrollment()
{
    HANDLE_FUNCTION_VECTOR vector;
    vector.serializeToJson = (VECTOR_SERIALIZE_TO_JSON)individualEnrollment_serializeToJson;
    vector.deserializeFromJson = (VECTOR_DESERIALIZE_FROM_JSON)individualEnrollment_deserializeFromJson;
    vector.getId = (VECTOR_GET_ID)individualEnrollment_getRegistrationId;
    vector.getEtag = (VECTOR_GET_ETAG)individualEnrollment_getEtag;
    vector.destroy = (VECTOR_DESTROY)individualEnrollment_destroy;

    return vector;
}

static HANDLE_FUNCTION_VECTOR getVector_enrollmentGroup()
{
    HANDLE_FUNCTION_VECTOR vector;
    vector.serializeToJson = (VECTOR_SERIALIZE_TO_JSON)enrollmentGroup_serializeToJson;
    vector.deserializeFromJson = (VECTOR_DESERIALIZE_FROM_JSON)enrollmentGroup_deserializeFromJson;
    vector.getId = (VECTOR_GET_ID)enrollmentGroup_getGroupId;
    vector.getEtag = (VECTOR_GET_ETAG)enrollmentGroup_getEtag;
    vector.destroy = (VECTOR_DESTROY)enrollmentGroup_destroy;

    return vector;
}

static void on_http_connected(void* callback_ctx, HTTP_CALLBACK_REASON connect_result)
{
    if (callback_ctx != NULL)
    {
        PROV_SERVICE_CLIENT* prov_client = (PROV_SERVICE_CLIENT*)callback_ctx;
        if (connect_result == HTTP_CALLBACK_REASON_OK)
        {
            prov_client->http_state = HTTP_STATE_CONNECTED;
        }
        else
        {
            prov_client->http_state = HTTP_STATE_ERROR;
        }
    }
}

static void on_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    (void)error_result;
    if (callback_ctx != NULL)
    {
        PROV_SERVICE_CLIENT* prov_client = (PROV_SERVICE_CLIENT*)callback_ctx;
        prov_client->http_state = HTTP_STATE_ERROR;
        LogError("Failure encountered in http %d", error_result);
    }
    else
    {
        LogError("Failure encountered in http %d", error_result);
    }
}

static void on_http_reply_recv(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_len, unsigned int status_code, HTTP_HEADERS_HANDLE responseHeadersHandle)
{
    (void)responseHeadersHandle;
    (void)content_len;
    if (callback_ctx != NULL)
    {
        PROV_SERVICE_CLIENT* prov_client = (PROV_SERVICE_CLIENT*)callback_ctx;
        const char* content_str = (const char*)content;

        //if there is a json response
        if (content != NULL)
        {
            if ((prov_client->response = malloc(strlen(content_str) + 1)) == NULL)
            {
                LogError("Allocating response failed");
                prov_client->response = NULL;
            }
            else
            {
                strcpy(prov_client->response, content_str);
            }
        }
        
        //update HTTP state
        if (request_result == HTTP_CALLBACK_REASON_OK)
        {
            if (status_code >= 200 && status_code <= 299)
            {
                prov_client->http_state = HTTP_STATE_REQUEST_RECV;
            }
            else
            {
                prov_client->http_state = HTTP_STATE_ERROR;
            }
        }
        else
        {
            prov_client->http_state = HTTP_STATE_ERROR;
        }
    }
    else
    {
        LogError("Invalid callback context");
    }
}

static HTTP_HEADERS_HANDLE construct_http_headers(const PROV_SERVICE_CLIENT* prov_client, const char* etag, HTTP_CLIENT_REQUEST_TYPE request)
{
    HTTP_HEADERS_HANDLE result;
    if ((result = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("failure sending request");
    }
    else
    {
        size_t secSinceEpoch = (size_t)(difftime(get_time(NULL), EPOCH_TIME_T_VALUE) + 0);
        size_t expiryTime = secSinceEpoch + SAS_TOKEN_DEFAULT_LIFETIME;

        STRING_HANDLE sas_token = SASToken_CreateString(prov_client->access_key, prov_client->provisioning_service_uri, prov_client->key_name, expiryTime);
        if (sas_token == NULL)
        {
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else
        {
            if ((HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_USER_AGENT, HEADER_VALUE_USER_AGENT) != HTTP_HEADERS_OK) ||
                (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_ACCEPT, HEADER_VALUE_ACCEPT) != HTTP_HEADERS_OK) ||
                ((request != HTTP_CLIENT_REQUEST_DELETE) && (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_CONTENT_TYPE, HEADER_VALUE_CONTENT_TYPE) != HTTP_HEADERS_OK)) ||
                (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_AUTHORIZATION, STRING_c_str(sas_token)) != HTTP_HEADERS_OK) ||
                ((etag != NULL) && (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_IF_MATCH, etag) != HTTP_HEADERS_OK)))
            {
                LogError("failure adding header value");
                HTTPHeaders_Free(result);
                result = NULL;
            }
            STRING_delete(sas_token);
        }
    }
    return result;
}

static STRING_HANDLE construct_registration_path(const char* registration_id, const char* path_fmt)
{
    STRING_HANDLE result;
    STRING_HANDLE registration_encode;

    if (registration_id == NULL)
    {
        LogError("invalid registration id");
        result = NULL;
    }
    else if ((registration_encode = URL_EncodeString(registration_id)) == NULL)
    {
        LogError("Failed encode registration Id");
        result = NULL;
    }
    else
    {
        if ((result = STRING_construct_sprintf(path_fmt, STRING_c_str(registration_encode), PROVISIONING_SERVICE_API_VERSION)) == NULL)
        {
            LogError("Failed constructing provisioning path");
        }
        STRING_delete(registration_encode);
    }
    return result;
}

static HTTP_CLIENT_HANDLE connect_to_service(PROV_SERVICE_CLIENT* prov_client)
{
    HTTP_CLIENT_HANDLE result;

    // Create uhttp
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy;
    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = prov_client->provisioning_service_uri;
    tls_io_config.port = DEFAULT_HTTPS_PORT;

     // Setup proxy
     if (prov_client->proxy_options != NULL)
     {
         memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
         http_proxy.hostname = prov_client->provisioning_service_uri;
         http_proxy.port = DEFAULT_HTTPS_PORT;
         http_proxy.proxy_hostname = prov_client->proxy_options->host_address;
         http_proxy.proxy_port = prov_client->proxy_options->port;
         http_proxy.username = prov_client->proxy_options->username;
         http_proxy.password = prov_client->proxy_options->password;

         tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
         tls_io_config.underlying_io_parameters = &http_proxy;
     }

    const IO_INTERFACE_DESCRIPTION* interface_desc = platform_get_default_tlsio();
    if (interface_desc == NULL)
    {
        LogError("platform default tlsio is NULL");
        result = NULL;
    }
    else if ((result = uhttp_client_create(interface_desc, &tls_io_config, on_http_error, prov_client)) == NULL)
    {
        LogError("Failed creating http object");
    }
    else if (prov_client->certificate != NULL && uhttp_client_set_trusted_cert(result, prov_client->certificate) != HTTP_CLIENT_OK)
    {
         LogError("Failed setting trusted cert");
         uhttp_client_destroy(result);
         result = NULL;
     }
    else if (prov_client->tracing == TRACING_STATUS_ON && uhttp_client_set_trace(result, true, true) != HTTP_CLIENT_OK)
    {
        LogError("Failed setting trace");
        uhttp_client_destroy(result);
        result = NULL;
    }
    else if (uhttp_client_open(result, prov_client->provisioning_service_uri, DEFAULT_HTTPS_PORT, on_http_connected, prov_client) != HTTP_CLIENT_OK)
    {
        LogError("Failed opening http url %s", prov_client->provisioning_service_uri);
        uhttp_client_destroy(result);
        result = NULL;
    }
    return result;
}

static int rest_call(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, HTTP_CLIENT_REQUEST_TYPE operation, const char* registration_path, HTTP_HEADERS_HANDLE request_headers, const char* content)
{
    int result;
    size_t content_len;
    HTTP_CLIENT_HANDLE http_client;

    if (content == NULL)
    {
        content_len = 0;
    }
    else
    {
        content_len = strlen(content);
    }

    http_client = connect_to_service(prov_client);
    if (http_client == NULL)
    {
        LogError("Failed connecting to service");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
        do
        {
            uhttp_client_dowork(http_client);
            if (prov_client->http_state == HTTP_STATE_CONNECTED)
            {
                if (uhttp_client_execute_request(http_client, operation, registration_path, request_headers, (unsigned char*)content, content_len, on_http_reply_recv, prov_client) != HTTP_CLIENT_OK)
                {
                    LogError("Failure executing http request");
                    prov_client->http_state = HTTP_STATE_ERROR;
                    result = __FAILURE__;
                }
                else
                {
                    prov_client->http_state = HTTP_STATE_REQUEST_SENT;
                }
            }
            else if (prov_client->http_state == HTTP_STATE_REQUEST_RECV)
            {
                prov_client->http_state = HTTP_STATE_COMPLETE;
            }
            else if (prov_client->http_state == HTTP_STATE_ERROR)
            {
                result = __FAILURE__;
                LogError("HTTP error");
            }
        } while (prov_client->http_state != HTTP_STATE_COMPLETE && prov_client->http_state != HTTP_STATE_ERROR);

        uhttp_client_close(http_client, NULL, NULL);
        uhttp_client_destroy(http_client);
    }

    prov_client->http_state = HTTP_STATE_DISCONNECTED;
    return result;
}

static int prov_sc_create_or_update_record(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, void** handle_ptr, HANDLE_FUNCTION_VECTOR vector, const char* path_format)
{
    int result = 0;
    void* handle;

    if (prov_client == NULL)
    {
        LogError("Invalid Provisioning Client Handle");
        result = __FAILURE__;
    }
    else if ((handle_ptr == NULL) || ((handle = *handle_ptr) == NULL))
    {
        LogError("Invalid handle");
        result = __FAILURE__;
    }
    else
    {
        char* content;
        if ((content = vector.serializeToJson(handle)) == NULL)
        {
            LogError("Failure serializing enrollment");
            result = __FAILURE__;
        }
        else
        {
            STRING_HANDLE registration_path;
            if ((registration_path = construct_registration_path(vector.getId(handle), path_format)) == NULL)
            {
                LogError("Failed constructing provisioning path");
                result = __FAILURE__;
            }
            else
            {
                HTTP_HEADERS_HANDLE request_headers;
                if ((request_headers = construct_http_headers(prov_client, vector.getEtag(handle), HTTP_CLIENT_REQUEST_PUT)) == NULL)
                {
                    LogError("Failure creating registration json content");
                    result = __FAILURE__;
                }
                else
                {
                    result = rest_call(prov_client, HTTP_CLIENT_REQUEST_PUT, STRING_c_str(registration_path), request_headers, content);

                    if (result == 0)
                    {
                        INDIVIDUAL_ENROLLMENT_HANDLE new_handle;
                        if ((new_handle = vector.deserializeFromJson(prov_client->response)) == NULL)
                        {
                            LogError("Failure constructing new enrollment structure from json response");
                            result = __FAILURE__;
                        }

                        //Free the user submitted enrollment, and replace the pointer reference to a new enrollment from the provisioning service
                        vector.destroy(handle);
                        *handle_ptr = new_handle;
                    }
                    else
                    {
                        LogError("Rest call failed");
                    }

                    free(prov_client->response);
                    prov_client->response = NULL;
                }
                HTTPHeaders_Free(request_headers);
            }
            STRING_delete(registration_path);
        }
        free(content);
    }

    return result;
}

static int prov_sc_delete_record_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const char* etag, const char* path_format)
{
    int result = 0;

    if (prov_client == NULL)
    {
        LogError("Invalid Provisioning Client Handle");
        result = __FAILURE__;
    }
    else if (id == NULL)
    {
        LogError("Invalid Id");
        result = __FAILURE__;
    }
    else
    {
        STRING_HANDLE registration_path;
        if ((registration_path = construct_registration_path(id, path_format)) == NULL)
        {
            LogError("Failed constructing provisioning path");
            result = __FAILURE__;
        }
        else
        {
            HTTP_HEADERS_HANDLE request_headers;
            if ((request_headers = construct_http_headers(prov_client, etag, HTTP_CLIENT_REQUEST_DELETE)) == NULL)
            {
                LogError("Failure creating registration json content");
                result = __FAILURE__;
            }
            else
            {
                result = rest_call(prov_client, HTTP_CLIENT_REQUEST_DELETE, STRING_c_str(registration_path), request_headers, NULL);
                free(prov_client->response);
                prov_client->response = NULL;
            }
            HTTPHeaders_Free(request_headers);
        }
        STRING_delete(registration_path);
    }

    return result;
}

static int prov_sc_get_record(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, void** handle_ptr, HANDLE_FUNCTION_VECTOR vector, const char* path_format)
{
    int result = 0;

    if (prov_client == NULL)
    {
        LogError("Invalid Provisioning Client Handle");
        result = __FAILURE__;
    }
    else if (id == NULL)
    {
        LogError("Invalid id");
        result = __FAILURE__;
    }
    else if (handle_ptr == NULL)
    {
        LogError("Invalid handle");
        result = __FAILURE__;
    }
    else
    {
        STRING_HANDLE registration_path;
        if ((registration_path = construct_registration_path(id, path_format)) == NULL)
        {
            LogError("Failed constructing provisioning path");
            result = __FAILURE__;
        }
        else
        {
            HTTP_HEADERS_HANDLE request_headers;
            if ((request_headers = construct_http_headers(prov_client, NULL, HTTP_CLIENT_REQUEST_GET)) == NULL)
            {
                LogError("Failure creating registration json content");
                result = __FAILURE__;
            }
            else
            {
                result = rest_call(prov_client, HTTP_CLIENT_REQUEST_GET, STRING_c_str(registration_path), request_headers, NULL);

                if (result == 0)
                {
                    void* handle;
                    if ((handle = vector.deserializeFromJson(prov_client->response)) == NULL)
                    {
                        LogError("Failure constructing new enrollment structure from json response");
                        result = __FAILURE__;
                    }
                    *handle_ptr = handle;
                }

                free(prov_client->response);
                prov_client->response = NULL;
            }
            HTTPHeaders_Free(request_headers);
        }
        STRING_delete(registration_path);
    }

    return result;
}

//Exposed functions below

void prov_sc_destroy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client)
{
    if (prov_client != NULL)
    {
        free(prov_client->provisioning_service_uri);
        free(prov_client->key_name);
        free(prov_client->access_key);
        free(prov_client->response);
        free(prov_client->certificate);
        free(prov_client);
    }
}

PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_create_from_connection_string(const char* conn_string)
{
    PROV_SERVICE_CLIENT* result;

    if (conn_string == NULL)
    {
        LogError("Input parameter is NULL: conn_string");
        result = NULL;
    }
    else
    {
        STRING_HANDLE cs_string;
        if ((cs_string = STRING_construct(conn_string)) == NULL)
        {
            LogError("STRING_construct failed");
            result = NULL;
        }
        else
        {
            MAP_HANDLE connection_string_values_map;
            if ((connection_string_values_map = connectionstringparser_parse(cs_string)) == NULL)
            {
                LogError("Tokenizing conn_string failed");
                result = NULL;
            }
            else
            {
                const char* hostname = NULL;
                const char* key_name = NULL;
                const char* key = NULL;

                if ((hostname = Map_GetValueFromKey(connection_string_values_map, IOTHUBHOSTNAME)) == NULL)
                {
                    LogError("Couldn't find %s in conn_string", IOTHUBHOSTNAME);
                    result = NULL;
                }
                else if ((key_name = Map_GetValueFromKey(connection_string_values_map, IOTHUBSHAREDACESSKEYNAME)) == NULL)
                {
                    LogError("Couldn't find %s in conn_string", IOTHUBSHAREDACESSKEYNAME);
                    result = NULL;
                }
                else if ((key = Map_GetValueFromKey(connection_string_values_map, IOTHUBSHAREDACESSKEY)) == NULL)
                {
                    LogError("Couldn't find %s in conn_string", IOTHUBSHAREDACESSKEY);
                    result = NULL;
                }
                if (hostname == NULL || key_name == NULL || key == NULL)
                {
                    LogError("invalid parameter hostname: %p, key_name: %p, key: %p", hostname, key_name, key);
                    result = NULL;
                }
                else if ((result = malloc(sizeof(PROV_SERVICE_CLIENT))) == NULL)
                {
                    LogError("Allocation of provisioning service client failed");
                    result = NULL;
                }
                else
                {
                    memset(result, 0, sizeof(PROV_SERVICE_CLIENT));
                    if (mallocAndStrcpy_s(&result->provisioning_service_uri, hostname) != 0)
                    {
                        LogError("Failure allocating of provisioning service uri");
                        prov_sc_destroy(result);
                        result = NULL;
                    }
                    else if (mallocAndStrcpy_s(&result->key_name, key_name) != 0)
                    {
                        LogError("Failure allocating of keyname");
                        prov_sc_destroy(result);
                        result = NULL;
                    }
                    else if (mallocAndStrcpy_s(&result->access_key, key) != 0)
                    {
                        LogError("Failure allocating of access key");
                        prov_sc_destroy(result);
                        result = NULL;
                    }
                    else
                    {
                        result->tracing = TRACING_STATUS_OFF;
                    }
                }
                Map_Destroy(connection_string_values_map);
            }
        }
        STRING_delete(cs_string);
    }
    return result;
}

void prov_sc_set_trace(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, TRACING_STATUS status)
{
    if (prov_client != NULL)
    {
        prov_client->tracing = status;
    }
}

int prov_sc_set_certificate(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* certificate)
{
    int result = 0;

    if (prov_client == NULL)
    {
        LogError("Invalid prov_client");
        result = __FAILURE__;
    }
    else if (certificate == NULL)
    {
        free(prov_client->certificate);
        prov_client->certificate = NULL;
    }
    else if (mallocAndStrcpy_overwrite(&prov_client->certificate, (char*)certificate) != 0)
    {
        LogError("Failed allocating memory for certificate");
        result = __FAILURE__;
    }

    return result;
}

int prov_sc_set_proxy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, HTTP_PROXY_OPTIONS* proxy_options)
{
    int result = 0;

    if (prov_client == NULL)
    {
        LogError("Invalid prov_client");
        result = __FAILURE__;
    }
    else if (proxy_options == NULL)
    {
        LogError("Invalid proxy options");
        result = __FAILURE__;
    }
    else
    {
        if (proxy_options->host_address == NULL)
        {
            LogError("Null host address in proxy options");
            result = __FAILURE__;
        }
        else if (((proxy_options->username == NULL) || (proxy_options->password == NULL))
            && (proxy_options->username != proxy_options->password))
        {
            LogError("Only one of username and password for proxy settings was NULL");
            result = __FAILURE__;
        }
        else
        {
            prov_client->proxy_options = proxy_options;
        }
    }

    return result;
}

int prov_sc_create_or_update_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr)
{
    return prov_sc_create_or_update_record(prov_client, (void**)enrollment_ptr, getVector_individualEnrollment(), INDV_ENROLL_PROVISION_PATH_FMT);
}

int prov_sc_delete_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, INDIVIDUAL_ENROLLMENT_HANDLE enrollment)
{
    return prov_sc_delete_record_by_param(prov_client, individualEnrollment_getRegistrationId(enrollment), individualEnrollment_getEtag(enrollment), INDV_ENROLL_PROVISION_PATH_FMT);
}

int prov_sc_delete_individual_enrollment_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* reg_id, const char* etag)
{
    return prov_sc_delete_record_by_param(prov_client, reg_id, etag, INDV_ENROLL_PROVISION_PATH_FMT);
}

int prov_sc_get_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* reg_id, INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr)
{
    return prov_sc_get_record(prov_client, reg_id, (void**)enrollment_ptr, getVector_individualEnrollment(), INDV_ENROLL_PROVISION_PATH_FMT);
}

int prov_sc_create_or_update_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE* enrollment_ptr)
{
    return prov_sc_create_or_update_record(prov_client, (void**)enrollment_ptr, getVector_enrollmentGroup(), ENROLL_GROUP_PROVISION_PATH_FMT);
}

int prov_sc_delete_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE enrollment)
{
    return prov_sc_delete_record_by_param(prov_client, enrollmentGroup_getGroupId(enrollment), enrollmentGroup_getEtag(enrollment), ENROLL_GROUP_PROVISION_PATH_FMT);
}

int prov_sc_delete_enrollment_group_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, const char* etag)
{
    return prov_sc_delete_record_by_param(prov_client, group_id, etag, ENROLL_GROUP_PROVISION_PATH_FMT);
}

int prov_sc_get_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, ENROLLMENT_GROUP_HANDLE* enrollment_ptr)
{
    return prov_sc_get_record(prov_client, group_id, (void**)enrollment_ptr, getVector_enrollmentGroup(), ENROLL_GROUP_PROVISION_PATH_FMT);
}
