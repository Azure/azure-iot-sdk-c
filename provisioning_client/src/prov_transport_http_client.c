// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/base64.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/internal/prov_transport_private.h"
#include "azure_uhttp_c/uhttp.h"
#include "parson.h"

#define HTTP_PORT_NUM                   443
#define HTTP_STATUS_CODE_OK             200
#define HTTP_STATUS_CODE_OK_MAX         226
#define HTTP_STATUS_CODE_UNAUTHORIZED   401

static const char* PROV_REGISTRATION_URI_FMT = "/%s/registrations/%s/register?api-version=%s";
static const char* PROV_OP_STATUS_URI_FMT = "/%s/registrations/%s/operations/%s?api-version=%s";

static const char* HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* HEADER_USER_AGENT = "UserAgent";
static const char* HEADER_ACCEPT = "Accept";
static const char* HEADER_CONTENT_TYPE = "Content-Type";
static const char* HEADER_CONNECTION = "Connection";
static const char* USER_AGENT_VALUE = "prov_device_client/1.0";
static const char* ACCEPT_VALUE = "application/json";
static const char* CONTENT_TYPE_VALUE = "application/json; charset=utf-8";
static const char* KEEP_ALIVE_VALUE = "keep-alive";

static const char* TPM_SECURITY_INFO = "{\"registrationId\":\"%s\",\"tpm\":{\"endorsementKey\":\"%s\", \"storageRootKey\":\"%s\"}}";
static const char* RIOT_SECURITY_INFO = "{ \"registrationId\":\"%s\" }";

DEFINE_ENUM_STRINGS(HTTP_CALLBACK_REASON, HTTP_CALLBACK_REASON_VALUES);

typedef enum PROV_TRANSPORT_STATE_TAG
{
    TRANSPORT_CLIENT_STATE_IDLE,

    TRANSPORT_CLIENT_STATE_REG_SEND,
    TRANSPORT_CLIENT_STATE_REG_SENT,
    TRANSPORT_CLIENT_STATE_REG_RECV,

    TRANSPORT_CLIENT_STATE_STATUS_SENT,
    TRANSPORT_CLIENT_STATE_STATUS_RECV,

    TRANSPORT_CLIENT_STATE_ERROR
} PROV_TRANSPORT_STATE;

typedef struct PROV_TRANSPORT_HTTP_INFO_TAG
{
    PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK register_data_cb;
    void* user_ctx;
    PROV_DEVICE_TRANSPORT_STATUS_CALLBACK status_cb;
    void* status_ctx;
    PROV_TRANSPORT_CHALLENGE_CALLBACK challenge_cb;
    void* challenge_ctx;
    PROV_TRANSPORT_JSON_PARSE json_parse_cb;
    void* json_ctx;

    char* hostname;
    char* proxy_host;
    int proxy_port;
    char* username;
    char* password;

    char* x509_cert;
    char* private_key;

    char* certificate;

    BUFFER_HANDLE ek;
    BUFFER_HANDLE srk;
    char* registration_id;
    char* scope_id;
    char* sas_token;

    BUFFER_HANDLE nonce;

    char* operation_id;

    char* api_version;

    char* payload_data;
    unsigned int http_status_code;

    bool http_connected;
    bool log_trace;
    PROV_TRANSPORT_STATE transport_state;
    TRANSPORT_HSM_TYPE hsm_type;

    HTTP_CLIENT_HANDLE http_client;
} PROV_TRANSPORT_HTTP_INFO;

static void on_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    (void)error_result;
    if (callback_ctx != NULL)
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)callback_ctx;
        http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
        LogError("http error encountered: %s", ENUM_TO_STRING(HTTP_CALLBACK_REASON, error_result));
    }
    else
    {
        LogError("callback_ctx NULL.  http error encountered: %s", ENUM_TO_STRING(HTTP_CALLBACK_REASON, error_result));
    }
}

static void on_http_reply_recv(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_len, unsigned int status_code, HTTP_HEADERS_HANDLE responseHeadersHandle)
{
    (void)responseHeadersHandle;
    if (callback_ctx != NULL)
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)callback_ctx;
        http_info->http_status_code = status_code;
        if (request_result != HTTP_CALLBACK_REASON_OK)
        {
            LogError("Failure http reply %s", ENUM_TO_STRING(HTTP_CALLBACK_REASON, request_result));
            http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
        }
        else if ((status_code >= HTTP_STATUS_CODE_OK && status_code <= HTTP_STATUS_CODE_OK_MAX) || status_code == HTTP_STATUS_CODE_UNAUTHORIZED)
        {
            if (content != NULL && content_len > 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_038: [ prov_transport_http_dowork shall free the payload_data ] */
                if (http_info->payload_data != NULL)
                {
                    free(http_info->payload_data);
                }
                http_info->payload_data = malloc(content_len + 1);
                if (http_info->payload_data == NULL)
                {
                    LogError("Failure sending http request");
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                }
                else
                {
                    memset(http_info->payload_data, 0, content_len + 1);
                    memcpy(http_info->payload_data, content, content_len);
                    if (http_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SENT)
                    {
                        http_info->transport_state = TRANSPORT_CLIENT_STATE_REG_RECV;
                    }
                    else
                    {
                        http_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_RECV;
                    }
                }
            }
            else
            {
                if (http_info->transport_state == TRANSPORT_CLIENT_STATE_REG_SENT)
                {
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_REG_RECV;
                }
                else
                {
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_RECV;
                }
            }
        }
        else
        {
            LogError("Failure status code sent from the server: %u", status_code);
            http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
        }
    }
    else
    {
        LogError("Null callback_ctx specified");
    }
}

static void on_http_connected(void* callback_ctx, HTTP_CALLBACK_REASON open_result)
{
    if (callback_ctx != NULL)
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)callback_ctx;
        if (open_result == HTTP_CALLBACK_REASON_OK)
        {
            if (http_info->status_cb != NULL)
            {
                http_info->status_cb(PROV_DEVICE_TRANSPORT_STATUS_CONNECTED, http_info->status_ctx);
            }
            http_info->http_connected = true;
        }
        else
        {
            LogError("Http connection failed to connect");
            http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
            http_info->http_connected = false;
        }
    }
    else
    {
        LogError("Null callback_ctx specified");
    }
}

static HTTP_HEADERS_HANDLE construct_http_headers(const char* sas_token)
{
    HTTP_HEADERS_HANDLE result;
    if ((result = HTTPHeaders_Alloc()) == NULL)
    {
        LogError("failure sending request");
    }
    else
    {
        if ((HTTPHeaders_AddHeaderNameValuePair(result, HEADER_USER_AGENT, USER_AGENT_VALUE) != HTTP_HEADERS_OK) ||
            (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_ACCEPT, ACCEPT_VALUE) != HTTP_HEADERS_OK) ||
            (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_CONNECTION, KEEP_ALIVE_VALUE) != HTTP_HEADERS_OK) ||
            (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_CONTENT_TYPE, CONTENT_TYPE_VALUE) != HTTP_HEADERS_OK))
        {
            LogError("failure adding header value");
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else
        {
            if (sas_token != NULL)
            {
                if (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_AUTHORIZATION, sas_token) != HTTP_HEADERS_OK)
                {
                    LogError("failure adding sas_token header value");
                    HTTPHeaders_Free(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

static char* construct_json_data(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    char* result;
    if (http_info->hsm_type == TRANSPORT_HSM_TYPE_TPM)
    {
        STRING_HANDLE encoded_srk = NULL;
        STRING_HANDLE encoded_ek;
        if ((encoded_ek = Base64_Encoder(http_info->ek)) == NULL)
        {
            LogError("Failure encoding ek");
            result = NULL;
        }
        else if ((encoded_srk = Base64_Encoder(http_info->srk)) == NULL)
        {
            LogError("Failure encoding srk");
            result = NULL;
        }
        else
        {
            size_t json_length = strlen(TPM_SECURITY_INFO) + strlen(http_info->registration_id) + STRING_length(encoded_ek) + STRING_length(encoded_srk);
            result = (char*)malloc(json_length);
            if (result == NULL)
            {
                LogError("Failure allocating json information");
                result = NULL;
            }
            else if (sprintf(result, TPM_SECURITY_INFO, http_info->registration_id, STRING_c_str(encoded_ek), STRING_c_str(encoded_srk)) == 0)
            {
                LogError("Failure constructing json information");
                free(result);
                result = NULL;
            }
            STRING_delete(encoded_srk);
            STRING_delete(encoded_ek);
        }
    }
    else
    {
        size_t json_length = strlen(RIOT_SECURITY_INFO) + strlen(http_info->registration_id);
        result = (char*)malloc(json_length);
        if (result == NULL)
        {
            LogError("Failure allocating json information");
            result = NULL;
        }
        else if (sprintf(result, RIOT_SECURITY_INFO, http_info->registration_id) == 0)
        {
            LogError("Failure constructing json information");
            free(result);
            result = NULL;
        }
    }
    return result;
}

static char* construct_url_path(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    char* result;
    size_t path_len;
    STRING_HANDLE encoded_scope;

    STRING_HANDLE encoded_reg_id = URL_EncodeString(http_info->registration_id);
    if (encoded_reg_id == NULL)
    {
        LogError("Failure encoding reg id");
        result = NULL;
    }
    else if ((encoded_scope = URL_EncodeString(http_info->scope_id)) == NULL)
    {
        LogError("Failure encoding string value");
        STRING_delete(encoded_reg_id);
        result = NULL;
    }
    else
    {
        if (http_info->operation_id == NULL)
        {
            path_len = strlen(PROV_REGISTRATION_URI_FMT) + STRING_length(encoded_scope) + STRING_length(encoded_reg_id) + strlen(http_info->api_version);
            if ((result = malloc(path_len + 1)) == NULL)
            {
                LogError("Failure allocating url path");
            }
            else
            {
                if (sprintf(result, PROV_REGISTRATION_URI_FMT, STRING_c_str(encoded_scope), STRING_c_str(encoded_reg_id), http_info->api_version) == 0)
                {
                    LogError("Failure constructing url path");
                    free(result);
                    result = NULL;
                }
            }
        }
        else
        {
            STRING_HANDLE encoded_op_id = URL_EncodeString(http_info->operation_id);
            if (encoded_op_id == NULL)
            {
                LogError("Failure encoding operation id");
                result = NULL;
            }
            else
            {
                path_len = strlen(PROV_OP_STATUS_URI_FMT) + STRING_length(encoded_scope) + STRING_length(encoded_reg_id) + STRING_length(encoded_op_id) + strlen(http_info->api_version);
                if ((result = malloc(path_len + 1)) == NULL)
                {
                    LogError("failure allocating retrieval path");
                }
                else
                {
                    if (sprintf(result, PROV_OP_STATUS_URI_FMT, STRING_c_str(encoded_scope), STRING_c_str(encoded_reg_id), STRING_c_str(encoded_op_id), http_info->api_version) == 0)
                    {
                        LogError("failure allocating retrieval path");
                        free(result);
                        result = NULL;
                    }
                }
                STRING_delete(encoded_op_id);
            }
        }
        STRING_delete(encoded_scope);
        STRING_delete(encoded_reg_id);
    }
    return result;
}

static int send_registration_info(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    int result;
    char* uri_path;
    char* json_data;
    HTTP_HEADERS_HANDLE http_headers;

    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_018: [ prov_transport_http_register_device shall construct the http headers for anonymous communication to the service. ] */
    if ((http_headers = construct_http_headers(NULL)) == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_021: [ If any error is encountered prov_transport_http_register_device shall return a non-zero value. ] */
        LogError("failure constructing http headers");
        result = __FAILURE__;
    }
    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_019: [ prov_transport_http_register_device shall construct the path to the service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/register-me?api-version=<api_version> ] */
    else if ((uri_path = construct_url_path(http_info)) == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_021: [ If any error is encountered prov_transport_http_register_device shall return a non-zero value. ] */
        LogError("Failure constructing uri path");
        HTTPHeaders_Free(http_headers);
        result = __FAILURE__;
    }
    else if ((json_data = construct_json_data(http_info)) == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_021: [ If any error is encountered prov_transport_http_register_device shall return a non-zero value. ] */
        LogError("Failure constructing uri path");
        HTTPHeaders_Free(http_headers);
        free(uri_path);
        result = __FAILURE__;
    }
    else
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_020: [ prov_transport_http_register_device shall send the request using the http client. ] */
        if (uhttp_client_execute_request(http_info->http_client, HTTP_CLIENT_REQUEST_PUT, uri_path, http_headers, (const unsigned char*)json_data, strlen(json_data), on_http_reply_recv, http_info) != HTTP_CLIENT_OK)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_021: [ If any error is encountered prov_transport_http_register_device shall return a non-zero value. ] */
            LogError("Failure sending http request");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
        free(uri_path);
        free(json_data);
        HTTPHeaders_Free(http_headers);
    }
    return result;
}

static int send_challenge_response(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    int result;
    HTTP_HEADERS_HANDLE http_headers;
    char* uri_path;
    char* json_data;

    if ((http_headers = construct_http_headers(http_info->sas_token)) == NULL)
    {
        LogError("failure constructing http headers");
        result = __FAILURE__;
    }
    else if ((uri_path = construct_url_path(http_info)) == NULL)
    {
        LogError("Failure constructing uri path");
        HTTPHeaders_Free(http_headers);
        result = __FAILURE__;
    }
    else if ((json_data = construct_json_data(http_info)) == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_021: [ If any error is encountered prov_transport_http_register_device shall return a non-zero value. ] */
        LogError("Failure constructing uri path");
        HTTPHeaders_Free(http_headers);
        free(uri_path);
        result = __FAILURE__;
    }
    else
    {
        if (uhttp_client_execute_request(http_info->http_client, HTTP_CLIENT_REQUEST_PUT, uri_path, http_headers, (const unsigned char*)json_data, strlen(json_data), on_http_reply_recv, http_info) != HTTP_CLIENT_OK)
        {
            LogError("Failure sending http request");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
        free(json_data);
        free(uri_path);
        HTTPHeaders_Free(http_headers);
    }
    return result;
}

static void free_json_parse_info(PROV_JSON_INFO* parse_info)
{
    switch (parse_info->prov_status)
    {
        case PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED:
            BUFFER_delete(parse_info->authorization_key);
            free(parse_info->key_name);
            break;
        case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED:
            BUFFER_delete(parse_info->authorization_key);
            free(parse_info->iothub_uri);
            free(parse_info->device_id);
            break;
        case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING:
            free(parse_info->operation_id);
            break;
        default:
            break;
    }
    free(parse_info);
}

static void free_allocated_data(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    free(http_info->hostname);
    free(http_info->registration_id);
    free(http_info->api_version);
    free(http_info->scope_id);
    free(http_info->certificate);
    free(http_info->proxy_host);
    free(http_info->x509_cert);
    free(http_info->private_key);
    free(http_info->username);
    free(http_info->password);
    free(http_info->payload_data);
    free(http_info->sas_token);
    free(http_info->operation_id);
    BUFFER_delete(http_info->ek);
    BUFFER_delete(http_info->srk);
    BUFFER_delete(http_info->nonce);
    free(http_info);
}

static int create_connection(PROV_TRANSPORT_HTTP_INFO* http_info)
{
    int result;
    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_009: [ prov_transport_http_open shall create the http client adding any proxy and certificate information that is presented. ] */
    if (http_info->http_client == NULL)
    {
        TLSIO_CONFIG tls_io_config;
        memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
        tls_io_config.hostname = http_info->hostname;
        tls_io_config.port = HTTP_PORT_NUM;

        // Setup proxy
        if (http_info->proxy_host != NULL)
        {
            HTTP_PROXY_IO_CONFIG http_proxy;

            memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
            http_proxy.hostname = http_info->hostname;
            http_proxy.port = HTTP_PORT_NUM;
            http_proxy.proxy_hostname = http_info->proxy_host;
            http_proxy.proxy_port = http_info->proxy_port;
            http_proxy.username = http_info->username;
            http_proxy.password = http_info->password;

            tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
            tls_io_config.underlying_io_parameters = &http_proxy;
        }
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_009: [ prov_transport_http_open shall create the http client adding any proxy and certificate information that is presented. ] */
        const IO_INTERFACE_DESCRIPTION* interface_desc;

        if ((interface_desc = platform_get_default_tlsio()) == NULL)
        {
            LogError("failgetting tls interface description");
            result = __FAILURE__;
        }
        else if ((http_info->http_client = uhttp_client_create(interface_desc, &tls_io_config, on_http_error, http_info)) == NULL)
        {
            LogError("failed to create http client");
            result = __FAILURE__;
        }
        else
        {
            (void)uhttp_client_set_trace(http_info->http_client, http_info->log_trace, true);

            if (http_info->certificate != NULL && uhttp_client_set_trusted_cert(http_info->http_client, http_info->certificate) != HTTP_CLIENT_OK)
            {
                LogError("fail to set the trusted certificate in the http client");
                uhttp_client_destroy(http_info->http_client);
                http_info->http_client = NULL;
                result = __FAILURE__;
            }
            else
            {
                if (http_info->hsm_type == TRANSPORT_HSM_TYPE_X509)
                {
                    if (http_info->x509_cert == NULL || http_info->private_key == NULL)
                    {
                        LogError("x509 certificate information was not properly set");
                        result = __FAILURE__;
                    }
                    else if (uhttp_client_set_X509_cert(http_info->http_client, true, http_info->x509_cert, http_info->private_key) != HTTP_CLIENT_OK)
                    {
                        LogError("failed to set x509 certificate information");
                        result = __FAILURE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    else
    {
        result = 0;
    }

    if (result == 0)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_010: [ prov_transport_http_open shall opening the http communications with the service. ] */
        if (uhttp_client_open(http_info->http_client, http_info->hostname, HTTP_PORT_NUM, on_http_connected, http_info) != HTTP_CLIENT_OK)
        {
            LogError("failed to open http client");
            uhttp_client_destroy(http_info->http_client);
            http_info->http_client = NULL;
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

PROV_DEVICE_TRANSPORT_HANDLE prov_transport_http_create(const char* uri, TRANSPORT_HSM_TYPE type, const char* scope_id, const char* api_version)
{
    PROV_TRANSPORT_HTTP_INFO* result;
    if (uri == NULL || scope_id == NULL || api_version == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_001: [ If uri, scope_id, registration_id or api_version are NULL prov_transport_http_create shall return NULL. ] */
        LogError("Invalid parameter specified uri: %p, scope_id: %p, api_version: %p", uri, scope_id, api_version);
        result = NULL;
    }
    else
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_002: [ prov_transport_http_create shall allocate the memory for the PROV_DEVICE_TRANSPORT_HANDLE variables. ] */
        result = malloc(sizeof(PROV_TRANSPORT_HTTP_INFO));
        if (result == NULL)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_003: [ If any error is encountered prov_transport_http_create shall return NULL. ] */
            LogError("Unable to allocate PROV_TRANSPORT_HTTP_INFO");
        }
        else
        {
            memset(result, 0, sizeof(PROV_TRANSPORT_HTTP_INFO));
            if (mallocAndStrcpy_s(&result->hostname, uri) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_003: [ If any error is encountered prov_transport_http_create shall return NULL. ] */
                LogError("Failure allocating hostname");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->api_version, api_version) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_003: [ If any error is encountered prov_transport_http_create shall return NULL. ] */
                LogError("Failure allocating api_version");
                free_allocated_data(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->scope_id, scope_id) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_003: [ If any error is encountered prov_transport_http_create shall return NULL. ] */
                LogError("Failure allocating scope Id");
                free_allocated_data(result);
                result = NULL;
            }
            else
            {
                result->hsm_type = type;
            }
        }
    }
    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_004: [ On success prov_transport_http_create shall return a PROV_DEVICE_TRANSPORT_HANDLE. ] */
    return result;
}

void prov_transport_http_destroy(PROV_DEVICE_TRANSPORT_HANDLE handle)
{
    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_007: [ If the argument handle is NULL, prov_transport_http_destroy shall do nothing ] */
    if (handle != NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_005: [ prov_transport_http_destroy shall free all resources associated with the PROV_DEVICE_TRANSPORT_HANDLE handle ] */
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        free_allocated_data(http_info);
    }
}

int prov_transport_http_open(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* registration_id, BUFFER_HANDLE ek, BUFFER_HANDLE srk, PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK data_callback, void* user_ctx, PROV_DEVICE_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx)
{
    int result;
    PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
    if (http_info == NULL || data_callback == NULL || status_cb == NULL || registration_id == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_008: [ If the argument handle, data_callback, or status_cb are NULL, prov_transport_http_open shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, data_callback: %p, status_cb: %p, registration_id: %p", handle, data_callback, status_cb, registration_id);
        result = __FAILURE__;
    }
    else if (http_info->hsm_type == TRANSPORT_HSM_TYPE_TPM && (ek == NULL || srk == NULL))
    {
        LogError("Invalid parameter specified ek: %p, srk: %p", ek, srk);
        result = __FAILURE__;
    }
    else if (ek != NULL && (http_info->ek = BUFFER_clone(ek)) == NULL)
    {
        LogError("Unable to allocate endorsement key");
        result = __FAILURE__;
    }
    else if (srk != NULL && (http_info->srk = BUFFER_clone(srk)) == NULL)
    {
        LogError("Unable to allocate storage root key");
        BUFFER_delete(http_info->ek);
        http_info->ek = NULL;
        result = __FAILURE__;
    }
    else if (mallocAndStrcpy_s(&http_info->registration_id, registration_id) != 0)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_003: [ If any error is encountered prov_transport_http_create shall return NULL. ] */
        LogError("failure constructing registration Id");
        BUFFER_delete(http_info->ek);
        http_info->ek = NULL;
        BUFFER_delete(http_info->srk);
        http_info->srk = NULL;
        result = __FAILURE__;
    }
    else
    {
        http_info->register_data_cb = data_callback;
        http_info->user_ctx = user_ctx;
        http_info->status_cb = status_cb;
        http_info->status_ctx = status_ctx;

        if (create_connection(http_info) != 0)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_013: [ If an error is encountered prov_transport_http_open shall return a non-zero value. ] */
            LogError("Failure creating http connection");
            if (http_info->ek != NULL)
            {
                BUFFER_delete(http_info->ek);
                http_info->ek = NULL;
            }
            if (http_info->srk != NULL)
            {
                BUFFER_delete(http_info->srk);
                http_info->srk = NULL;
            }
            http_info->register_data_cb = NULL;
            http_info->user_ctx = NULL;
            http_info->status_cb = NULL;
            http_info->status_ctx = NULL;
            free(http_info->registration_id);
            http_info->registration_id = NULL;
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_012: [ If successful prov_transport_http_open shall return 0. ] */
    return result;
}

int prov_transport_http_close(PROV_DEVICE_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_014: [ If the argument handle is NULL, prov_transport_http_close shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_015: [ prov_transport_http_close shall attempt to close the http communication with the service. ] */
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        if (http_info->http_client != NULL)
        {
            BUFFER_delete(http_info->ek);
            http_info->ek = NULL;
            BUFFER_delete(http_info->srk);
            http_info->srk = NULL;
            free(http_info->registration_id);
            http_info->registration_id = NULL;

            uhttp_client_close(http_info->http_client, NULL, NULL);
            uhttp_client_dowork(http_info->http_client);

            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_006: [ prov_transport_http_close shall call the uhttp_client_destroy function function associated with the http_client ] */
            uhttp_client_destroy(http_info->http_client);
            http_info->http_client = NULL;
        }
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_016: [ On success prov_transport_http_close shall return 0. ] */
        result = 0;
    }
    return result;
}

int prov_transport_http_register_device(PROV_DEVICE_TRANSPORT_HANDLE handle, PROV_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, PROV_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx)
{
    int result;
    PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
    if (http_info == NULL || json_parse_cb == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_017: [ If the argument handle or json_data is NULL, prov_transport_http_register_device shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, json_parse_cb: %p", handle, json_parse_cb);
        result = __FAILURE__;
    }
    else if (http_info->hsm_type == TRANSPORT_HSM_TYPE_TPM && reg_challenge_cb == NULL)
    {
        LogError("registration challenge callback must be set using tpm");
        result = __FAILURE__;
    }
    else if (http_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
    {
        LogError("Provisioning is in an error state, close the connection and try again.");
        result = __FAILURE__;
    }
    else
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_022: [ On success prov_transport_http_register_device shall return 0. ] */
        http_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SEND;
        http_info->challenge_cb = reg_challenge_cb;
        http_info->challenge_ctx = user_ctx;
        http_info->json_parse_cb = json_parse_cb;
        http_info->json_ctx = json_ctx;

        result = 0;
    }
    return result;
}

int prov_transport_http_get_operation_status(PROV_DEVICE_TRANSPORT_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_029: [ If the argument handle, or operation_id is NULL, prov_transport_http_get_operation_status shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        HTTP_HEADERS_HANDLE http_headers;
        char* uri_path;

        if (http_info->operation_id == NULL)
        {
            LogError("operation_id was not previously set in the challenge method");
            result = __FAILURE__;
        }
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_030: [ prov_transport_http_get_operation_status shall construct the http headers with SAS_TOKEN as Authorization header. ] */
        else if ((http_headers = construct_http_headers(http_info->sas_token)) == NULL)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_033: [ If any error is encountered prov_transport_http_get_operation_status shall return a non-zero value. ] */
            LogError("failure constructing http headers");
            result = __FAILURE__;
        }
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_031: [ prov_transport_http_get_operation_status shall construct the path to the service in the following format: /<scope_id>/registrations/<url_encoded_registration_id>/operations<url_encoded_operation_id>?api-version=<api_version> ] */
        else if ((uri_path = construct_url_path(http_info)) == NULL)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_033: [ If any error is encountered prov_transport_http_get_operation_status shall return a non-zero value. ] */
            LogError("Failure constructing uri path");
            result = __FAILURE__;
        }
        else
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_032: [ prov_transport_http_get_operation_status shall send the request using the http client. ] */
            if (uhttp_client_execute_request(http_info->http_client, HTTP_CLIENT_REQUEST_GET, uri_path, http_headers, NULL, 0, on_http_reply_recv, http_info) != HTTP_CLIENT_OK)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_033: [ If any error is encountered prov_transport_http_get_operation_status shall return a non-zero value. ] */
                LogError("Failure sending data to server");
                result = __FAILURE__;
            }
            else
            {
                http_info->transport_state = TRANSPORT_CLIENT_STATE_STATUS_SENT;
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_034: [ On success prov_transport_http_get_operation_status shall return 0. ] */
                result = 0;
            }
            HTTPHeaders_Free(http_headers);
            free(uri_path);
        }
    }
    return result;
}

void prov_transport_http_dowork(PROV_DEVICE_TRANSPORT_HANDLE handle)
{
    if (handle != NULL)
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_036: [ prov_transport_http_dowork shall call the underlying http client do work method. ] */
        uhttp_client_dowork(http_info->http_client);

        if (http_info->http_connected || http_info->transport_state == TRANSPORT_CLIENT_STATE_ERROR)
        {
            switch (http_info->transport_state)
            {
            case TRANSPORT_CLIENT_STATE_REG_SEND:
                if (send_registration_info(http_info) != 0)
                {
                    LogError("NULL sas_token provided in challenge callback.");
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                }
                else
                {
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SENT;
                }
                break;
            case TRANSPORT_CLIENT_STATE_STATUS_RECV:
            case TRANSPORT_CLIENT_STATE_REG_RECV:
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_037: [ If the prov_transport_http_dowork state is Message received prov_transport_http_dowork shall call the calling process registration_data callback ] */
                PROV_JSON_INFO* parse_info = http_info->json_parse_cb(http_info->payload_data, http_info->json_ctx);
                if (parse_info == NULL)
                {
                    LogError("Unable to process registration reply.");
                    http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                }
                else
                {
                    switch (parse_info->prov_status)
                    {
                        case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED:
                            http_info->register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_OK, parse_info->authorization_key, parse_info->iothub_uri, parse_info->device_id, http_info->user_ctx);
                            http_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                            break;

                        case PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED:
                        {
                            const unsigned char* nonce = BUFFER_u_char(parse_info->authorization_key);
                            size_t nonce_len = BUFFER_length(parse_info->authorization_key);

                            http_info->sas_token = http_info->challenge_cb(nonce, nonce_len, parse_info->key_name, http_info->user_ctx);
                            if (http_info->sas_token == NULL)
                            {
                                LogError("NULL sas_token provided in challenge callback.");
                                http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            }
                            else
                            {
                                send_challenge_response(http_info);
                                http_info->transport_state = TRANSPORT_CLIENT_STATE_REG_SENT;
                            }
                            break;
                        }

                        case PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING:
                            if (parse_info->operation_id == NULL)
                            {
                                LogError("Failure operation Id invalid");
                                http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            }
                            else if (http_info->operation_id == NULL && mallocAndStrcpy_s(&http_info->operation_id, parse_info->operation_id) != 0)
                            {
                                LogError("Failure copying operation Id");
                                http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            }
                            else
                            {
                                if (http_info->status_cb != NULL)
                                {
                                    http_info->status_cb(parse_info->prov_status, http_info->status_ctx);
                                }
                                http_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                            }
                            break;

                        default:
                        case PROV_DEVICE_TRANSPORT_STATUS_ERROR:
                            LogError("Unable to process status reply");
                            http_info->transport_state = TRANSPORT_CLIENT_STATE_ERROR;
                            break;
                    }
                    free_json_parse_info(parse_info);
                }
                break;
            }

            case TRANSPORT_CLIENT_STATE_ERROR:
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_039: [ If the state is Error, prov_transport_http_dowork shall call the registration_data callback with PROV_DEVICE_TRANSPORT_RESULT_ERROR and NULL payload_data. ] */
                http_info->register_data_cb(PROV_DEVICE_TRANSPORT_RESULT_ERROR, NULL, NULL, NULL, http_info->user_ctx);
                http_info->transport_state = TRANSPORT_CLIENT_STATE_IDLE;
                break;

            case TRANSPORT_CLIENT_STATE_REG_SENT:
            case TRANSPORT_CLIENT_STATE_STATUS_SENT:
            case TRANSPORT_CLIENT_STATE_IDLE:
            default:
                break;
            }
        }
    }
}

int prov_transport_http_set_trace(PROV_DEVICE_TRANSPORT_HANDLE handle, bool trace_on)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_040: [ If the argument handle, is NULL, prov_transport_http_set_trace shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p", handle);
        result = __FAILURE__;
    }
    else
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        http_info->log_trace = trace_on;
        if (http_info->http_client != NULL)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_041: [ If the http client is not NULL, prov_transport_http_set_trace shall set the http client log trace function with the specified trace_on flag. ] */
            (void)uhttp_client_set_trace(http_info->http_client, http_info->log_trace, http_info->log_trace);
        }
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_042: [ On success prov_transport_http_set_trace shall return zero. ] */
        result = 0;
    }
    return result;
}

static int prov_transport_http_x509_cert(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key)
{
    int result;
    if (handle == NULL || certificate == NULL || private_key == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_043: [ If the argument handle, private_key or certificate is NULL, prov_transport_http_x509_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p, private_key: %p", handle, certificate, private_key);
        result = __FAILURE__;
    }
    else
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        if (http_info->x509_cert != NULL)
        {
            free(http_info->x509_cert);
        }
        if (http_info->private_key != NULL)
        {
            free(http_info->private_key);
        }

        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_045: [ prov_transport_http_x509_cert shall store the certificate and private_key for use when the http client connects. ] */
        if (mallocAndStrcpy_s(&http_info->x509_cert, certificate) != 0)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_044: [ If any error is encountered prov_transport_http_x509_cert shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else if (mallocAndStrcpy_s(&http_info->private_key, private_key) != 0)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_044: [ If any error is encountered prov_transport_http_x509_cert shall return a non-zero value. ] */
            free(http_info->x509_cert);
            http_info->x509_cert = NULL;
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_046: [ On success prov_transport_http_set_trace shall return zero. ] */
            result = 0;
        }
    }
    return result;
}

static int prov_transport_http_set_trusted_cert(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* certificate)
{
    int result;
    if (handle == NULL || certificate == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_047: [ If the argument handle, certificate is NULL, prov_transport_http_set_trusted_cert shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, certificate: %p", handle, certificate);
        result = __FAILURE__;
    }
    else
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        if (http_info->certificate != NULL)
        {
            free(http_info->certificate);
        }

        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_049: [ prov_transport_http_set_trusted_cert shall store the certificate for use when the http client connects. ] */
        if (mallocAndStrcpy_s(&http_info->certificate, certificate) != 0)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_048: [ If any error is encountered prov_transport_http_set_trusted_cert shall return a non-zero value. ] */
            result = __FAILURE__;
            LogError("failure allocating certificate");
        }
        else
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_050: [ On success prov_transport_http_set_trusted_cert shall return zero. ] */
            result = 0;
        }
    }
    return result;
}

static int prov_transport_http_set_proxy(PROV_DEVICE_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_options)
{
    int result;
    if (handle == NULL || proxy_options == NULL)
    {
        /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_051: [ If the argument handle or proxy_options is NULL, prov_transport_http_set_proxy shall return a non-zero value. ] */
        LogError("Invalid parameter specified handle: %p, proxy_options: %p", handle, proxy_options);
        result = __FAILURE__;
    }
    else
    {
        PROV_TRANSPORT_HTTP_INFO* http_info = (PROV_TRANSPORT_HTTP_INFO*)handle;
        if (proxy_options->host_address == NULL)
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_055: [ If proxy_options host_name is NULL prov_transport_http_set_proxy shall return a non-zero value. ] */
            LogError("NULL host_address in proxy options");
            result = __FAILURE__;
        }
        else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
            (proxy_options->username != proxy_options->password))
        {
            /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_053: [ If proxy_options username is NULL and proxy_options::password is not NULL prov_transport_http_set_proxy shall return a non-zero value. ] */
            LogError("Only one of username and password for proxy settings was NULL");
            result = __FAILURE__;
        }
        else
        {
            if (http_info->proxy_host != NULL)
            {
                free(http_info->proxy_host);
            }
            if (http_info->username != NULL)
            {
                free(http_info->username);
                http_info->username = NULL;
            }
            if (http_info->password != NULL)
            {
                free(http_info->password);
                http_info->password = NULL;
            }

            http_info->proxy_port = proxy_options->port;
            if (mallocAndStrcpy_s(&http_info->proxy_host, proxy_options->host_address) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_052: [ If any error is encountered prov_transport_http_set_proxy shall return a non-zero value. ] */
                LogError("Failure setting proxy_host name");
                result = __FAILURE__;
            }
            else if (proxy_options->username != NULL && mallocAndStrcpy_s(&http_info->username, proxy_options->username) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_052: [ If any error is encountered prov_transport_http_set_proxy shall return a non-zero value. ] */
                free(http_info->proxy_host);
                http_info->proxy_host = NULL;
                LogError("Failure setting proxy username");
                result = __FAILURE__;
            }
            else if (proxy_options->password != NULL && mallocAndStrcpy_s(&http_info->password, proxy_options->password) != 0)
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_052: [ If any error is encountered prov_transport_http_set_proxy shall return a non-zero value. ] */
                LogError("Failure setting proxy password");
                free(http_info->proxy_host);
                http_info->proxy_host = NULL;
                free(http_info->username);
                http_info->username = NULL;
                result = __FAILURE__;
            }
            else
            {
                /* Codes_PROV_TRANSPORT_HTTP_CLIENT_07_054: [ On success prov_transport_http_set_proxy shall return zero. ] */
                result = 0;
            }
        }
    }
    return result;
}

static PROV_DEVICE_TRANSPORT_PROVIDER prov_http_func =
{
    prov_transport_http_create,
    prov_transport_http_destroy,
    prov_transport_http_open,
    prov_transport_http_close,
    prov_transport_http_register_device,
    prov_transport_http_get_operation_status,
    prov_transport_http_dowork,
    prov_transport_http_set_trace,
    prov_transport_http_x509_cert,
    prov_transport_http_set_trusted_cert,
    prov_transport_http_set_proxy

};

const PROV_DEVICE_TRANSPORT_PROVIDER* Prov_Device_HTTP_Protocol(void)
{
    return &prov_http_func;
}
