// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/socketio.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_uhttp_c/uhttp.h"

#include "azure_c_shared_utility/envvariable.h"

#include "parson.h"

static const char* HSM_HTTP_EDGE_VERSION = "2018-06-28";
static const char* HTTP_HEADER_KEY_CONTENT_TYPE = "Content-Type";
static const char* HTTP_HEADER_VAL_CONTENT_TYPE = "application/json; charset=utf-8";
static const char* HSM_EDGE_SIGN_JSON_KEY_ID = "keyId";
static const char* HSM_EDGE_SIGN_JSON_KEY_ID_VALUE = "primary";
static const char* HSM_EDGE_SIGN_JSON_ALGORITHM = "algo";
static const char* HSM_EDGE_SIGN_JSON_ALGORITHM_VALUE = "HMACSHA256";
static const char* HSM_EDGE_SIGN_JSON_DATA = "data";
static const char* HSM_EDGE_SIGN_JSON_DIGEST = "digest";
static const char* HSM_EDGE_TRUSTED_CERTIFICATE_JSON_CERTIFICATE = "certificate";

static const int HSM_HTTP_EDGE_MAXIMUM_REQUEST_TIME = 60; // 1 Minute


#include "hsm_client_http_edge.h"

typedef enum WORKLOAD_PROTOCOL_TYPE_TAG
{
    WORKLOAD_PROTOCOL_TYPE_UNSPECIFIED,
    WORKLOAD_PROTOCOL_TYPE_HTTP,
    WORKLOAD_PROTOCOL_TYPE_UNIX_DOMAIN_SOCKET
} WORKLOAD_PROTOCOL_TYPE;

static const char* ENVIRONMENT_VAR_WORKLOADURI = "IOTEDGE_WORKLOADURI";
static const char* ENVIRONMENT_VAR_MODULE_GENERATION_ID = "IOTEDGE_MODULEGENERATIONID";
static const char* ENVIRONMENT_VAR_EDGEMODULEID = "IOTEDGE_MODULEID";

typedef struct HSM_CLIENT_HTTP_EDGE
{
    WORKLOAD_PROTOCOL_TYPE workload_protocol_type;
    char* workload_hostname;
    int   workload_portnumber;
    char* edge_module_generation_id;
    char* module_id;
} HSM_CLIENT_HTTP_EDGE;

static const char http_prefix[] = "http://";
static const int http_prefix_len = sizeof(http_prefix) - 1;

static const char unix_domain_sockets_prefix[] = "unix://";
static const int unix_domain_sockets_prefix_len = sizeof(unix_domain_sockets_prefix) - 1;

static HSM_CLIENT_HTTP_EDGE_INTERFACE http_edge_interface =
{

    hsm_client_http_edge_create,
    hsm_client_http_edge_destroy,
    hsm_client_http_edge_sign_data,
    hsm_client_http_edge_get_trust_bundle
};

static WORKLOAD_PROTOCOL_TYPE get_workload_protocol_type(const char* workload_uri)
{
    WORKLOAD_PROTOCOL_TYPE workload_protocol_type;

    if (strncmp(workload_uri, http_prefix, http_prefix_len) == 0)
    {
        workload_protocol_type = WORKLOAD_PROTOCOL_TYPE_HTTP;
    }
    else if (strncmp(workload_uri, unix_domain_sockets_prefix, unix_domain_sockets_prefix_len) == 0)
    {
        workload_protocol_type = WORKLOAD_PROTOCOL_TYPE_UNIX_DOMAIN_SOCKET;
    }
    else
    {
        LogError("WorkloadUri is set to %s, which is invalid", workload_uri);
        workload_protocol_type = WORKLOAD_PROTOCOL_TYPE_UNSPECIFIED;
    }

    return workload_protocol_type;
}

// This is the string we use to connect to the edge device itself.  An example will be
// http://127.0.0.1:8080.  Note NOT "https" as that would require us to trust the edgelet's
// server certificate, which we can't because we're still bootstrapping.
static int read_and_parse_edge_uri(HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge)
{
    const char* workload_uri;
    const char* colon_begin;
    int result = 0;

    if ((workload_uri = environment_get_variable(ENVIRONMENT_VAR_WORKLOADURI)) == NULL)
    {
        LogError("Environment variable %s not specified", ENVIRONMENT_VAR_WORKLOADURI);
        result = MU_FAILURE;
    }
    else if ((hsm_client_http_edge->workload_protocol_type = get_workload_protocol_type(workload_uri)) == WORKLOAD_PROTOCOL_TYPE_UNSPECIFIED)
    {
        result = MU_FAILURE;
    }

    if (result == 0)
    {
        if (hsm_client_http_edge->workload_protocol_type == WORKLOAD_PROTOCOL_TYPE_HTTP)
        {
            if ((colon_begin = strchr(workload_uri + http_prefix_len + 1, ':')) == NULL)
            {
                LogError("WorkloadUri is set to %s, missing ':' to indicate port number", workload_uri);
                result = MU_FAILURE;
            }
            else if ((hsm_client_http_edge->workload_portnumber = atoi(colon_begin + 1)) == 0)
            {
                LogError("WorkloadUri is set to %s, port number is not legal", workload_uri);
                result = MU_FAILURE;
            }
            else if ((hsm_client_http_edge->workload_hostname = malloc(colon_begin - workload_uri)) == NULL)
            {
                LogError("Failed allocating workload_hostname");
                result = MU_FAILURE;
            }
            else
            {
                const char* hostname_start = workload_uri + http_prefix_len;
                size_t chars_to_copy = colon_begin - workload_uri - http_prefix_len;
                memcpy(hsm_client_http_edge->workload_hostname, hostname_start, chars_to_copy);
                hsm_client_http_edge->workload_hostname[chars_to_copy] = 0;
                result = 0;
            }
        }
        else if (hsm_client_http_edge->workload_protocol_type == WORKLOAD_PROTOCOL_TYPE_UNIX_DOMAIN_SOCKET)
        {
            // Make sure that there is an address set.
            if (workload_uri[unix_domain_sockets_prefix_len] == 0)
            {
                LogError("hostname does not have content after prefix");
                result = MU_FAILURE;
            }
            else if (mallocAndStrcpy_s(&hsm_client_http_edge->workload_hostname, workload_uri + unix_domain_sockets_prefix_len) != 0)
            {
                LogError("Failed copying workload hostname");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }

    return result;
}

static int initialize_http_edge_device(HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge)
{
    int result;
    const char* edge_module_generation_id;
    const char* module_id;

    if ((edge_module_generation_id = environment_get_variable(ENVIRONMENT_VAR_MODULE_GENERATION_ID)) == NULL)
    {
        LogError("Environment variable %s not specified", ENVIRONMENT_VAR_MODULE_GENERATION_ID);
        result = MU_FAILURE;
    }
    else if ((module_id = environment_get_variable(ENVIRONMENT_VAR_EDGEMODULEID)) == NULL)
    {
        LogError("Environment variable %s not specified", ENVIRONMENT_VAR_EDGEMODULEID);
        result = MU_FAILURE;
    }
    else if (read_and_parse_edge_uri(hsm_client_http_edge) != 0)
    {
        LogError("read_and_parse_edge_uri failed");
        result = MU_FAILURE;
    }
    else if (mallocAndStrcpy_s(&hsm_client_http_edge->edge_module_generation_id, edge_module_generation_id) != 0)
    {
        LogError("Failed copying edge_module_generation_id");
        result = MU_FAILURE;
    }
    else if (mallocAndStrcpy_s(&hsm_client_http_edge->module_id, module_id) != 0)
    {
        LogError("Failed copying module_id");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}


HSM_CLIENT_HANDLE hsm_client_http_edge_create()
{
    HSM_CLIENT_HTTP_EDGE* result;
    result = malloc(sizeof(HSM_CLIENT_HTTP_EDGE));
    if (result == NULL)
    {
        LogError("Failure: malloc HSM_CLIENT_HTTP_EDGE.");
    }
    else
    {
        memset(result, 0, sizeof(HSM_CLIENT_HTTP_EDGE));
        result->workload_protocol_type = WORKLOAD_PROTOCOL_TYPE_UNSPECIFIED;
        if (initialize_http_edge_device(result) != 0)
        {
            LogError("Failure initializing http edge device.");
            hsm_client_http_edge_destroy((HSM_CLIENT_HANDLE)result);
            result = NULL;
        }
    }
    return (HSM_CLIENT_HANDLE)result;
}

void hsm_client_http_edge_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge = (HSM_CLIENT_HTTP_EDGE*)handle;
        free(hsm_client_http_edge->workload_hostname);
        free(hsm_client_http_edge->edge_module_generation_id);
        free(hsm_client_http_edge->module_id);
        free(hsm_client_http_edge);
    }
}

int hsm_client_http_edge_init(void)
{
    return 0;
}

void hsm_client_http_edge_deinit(void)
{
}

const HSM_CLIENT_HTTP_EDGE_INTERFACE* hsm_client_http_edge_interface()
{
    return &http_edge_interface;
}

static BUFFER_HANDLE construct_json_signing_blob(const char* data)
{
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;
    BUFFER_HANDLE result = NULL;
    char* serialized_string = NULL;
    STRING_HANDLE data_string = NULL;
    STRING_HANDLE data_url_encoded = NULL;
    STRING_HANDLE data_base64_encoded = NULL;

    // The caller passes us the data in the form "<tokenScope>\n<expire_time>".  We need to
    // URL encode the tokenScope for HTTP transmission, but not the \n, so build up string appropriately.
    const char* carriage_return_in_data = strchr(data, '\n');
    if ((carriage_return_in_data == NULL) || (*(carriage_return_in_data+1) == 0))
    {
        LogError("No carriage return in data %s", data);
        result = NULL;
    }
    else if ((data_string = STRING_construct_n(data, carriage_return_in_data - data)) == NULL)
    {
        LogError("creating data string failed");
        result = NULL;
    }
    else if ((data_url_encoded = URL_Encode(data_string)) == NULL)
    {
        LogError("url encoding of string %s failed", data);
        result = NULL;
    }
    else if ((STRING_concat(data_url_encoded, carriage_return_in_data)) != 0)
    {
        LogError("STRING_concat failed");
        result = NULL;
    }
    else if ((data_base64_encoded = Azure_Base64_Encode_Bytes((const unsigned char*)STRING_c_str(data_url_encoded), strlen(STRING_c_str(data_url_encoded)))) == NULL)
    {
        LogError("base64 encoding of string %s failed", STRING_c_str(data_url_encoded));
        result = NULL;
    }
    else if ((root_value = json_value_init_object()) == NULL)
    {
        LogError("json_value_init_object failed");
        result = NULL;
    }
    else if ((root_object = json_value_get_object(root_value)) == NULL)
    {
        LogError("json_value_get_object failed");
        result = NULL;
    }
    else if ((json_object_set_string(root_object, HSM_EDGE_SIGN_JSON_KEY_ID, HSM_EDGE_SIGN_JSON_KEY_ID_VALUE)) != JSONSuccess)
    {
        LogError("json_object_set_string failed for keyId");
        result = NULL;
    }
    else if ((json_object_set_string(root_object, HSM_EDGE_SIGN_JSON_ALGORITHM, HSM_EDGE_SIGN_JSON_ALGORITHM_VALUE)) != JSONSuccess)
    {
        LogError("json_object_set_string failed for algorithm");
        result = NULL;
    }
    else if ((json_object_set_string(root_object, HSM_EDGE_SIGN_JSON_DATA, STRING_c_str(data_base64_encoded))) != JSONSuccess)
    {
        LogError("json_object_set_string failed for data");
        result = NULL;
    }
    else if ((serialized_string = json_serialize_to_string(root_value)) == NULL)
    {
        LogError("json_serialize_to_string failed");
        result = NULL;
    }
    else if ((result = BUFFER_create((const unsigned char*)serialized_string, strlen(serialized_string) + 1)) == NULL)
    {
        LogError("Buffer_Create failed");
        result = NULL;
    }

    json_free_serialized_string(serialized_string);
    json_object_clear(root_object);
    json_value_free(root_value);
    STRING_delete(data_base64_encoded);
    STRING_delete(data_url_encoded);
    STRING_delete(data_string);
    return result;
}

static void on_edge_hsm_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    HSM_HTTP_WORKLOAD_CONTEXT* workload_context = (HSM_HTTP_WORKLOAD_CONTEXT*)callback_ctx;
    if (workload_context == NULL)
    {
        LogError("on_edge_hsm_http_error invoked with invalid context.  reason=%d", error_result);
    }
    else
    {
        LogError("on_edge_hsm_http_error invoked.  reason=%d", error_result);
        workload_context->continue_running = false;
    }
}

static void on_edge_hsm_http_recv(void* callback_ctx, HTTP_CALLBACK_REASON request_result, const unsigned char* content, size_t content_length, unsigned int status_code, HTTP_HEADERS_HANDLE response_headers)
{
    (void)response_headers;
    HSM_HTTP_WORKLOAD_CONTEXT* workload_context = (HSM_HTTP_WORKLOAD_CONTEXT*)callback_ctx;
    if (workload_context == NULL)
    {
        LogError("on_edge_hsm_http_recv invoked with invalid context");
    }
    else
    {
        if (request_result != HTTP_CALLBACK_REASON_OK)
        {
            LogError("on_edge_hsm_http_recv request result = %d", request_result);
        }
        else if (status_code >= 300)
        {
            LogError("executing HTTP request fails, status=%d, response_buffer=%s", status_code, content ? (const char*)content : "unspecified");
        }
        else if (content == NULL)
        {
            LogError("executing HTTP request fails, content not set");
        }
        else if ((workload_context->http_response = BUFFER_create(content, content_length)) == NULL)
        {
            LogError("failed copying response buffer");
        }
        else
        {
            ; // success
        }

        workload_context->continue_running = false;
    }
}

static void on_edge_hsm_http_connected(void* callback_ctx, HTTP_CALLBACK_REASON connect_result)
{
    if (connect_result != HTTP_CALLBACK_REASON_OK)
    {
        HSM_HTTP_WORKLOAD_CONTEXT* workload_context = (HSM_HTTP_WORKLOAD_CONTEXT*)callback_ctx;
        if (callback_ctx == NULL)
        {
            LogError("on_http_connected reports error %d but no context", connect_result);
        }
        else
        {
            LogError("on_http_connected reports error %d", connect_result);
            workload_context->continue_running = false;
        }
    }
}

static int send_request_to_edge_workload(HTTP_CLIENT_HANDLE http_handle, HTTP_HEADERS_HANDLE http_headers_handle, const char* uri_path, BUFFER_HANDLE json_to_send, HSM_HTTP_WORKLOAD_CONTEXT* workload_context)
{
    const unsigned char* json_to_send_str = BUFFER_u_char(json_to_send);
    time_t start_request_time = get_time(NULL);
    bool timed_out = false;
    HTTP_CLIENT_RESULT http_client_result;

    size_t json_to_send_str_len;
    HTTP_CLIENT_REQUEST_TYPE http_request_type;

    if (json_to_send_str != NULL)
    {
        json_to_send_str_len = strlen((const char*)json_to_send_str);
        http_request_type = HTTP_CLIENT_REQUEST_POST;
    }
    else
    {
        json_to_send_str_len = 0;
        http_request_type = HTTP_CLIENT_REQUEST_GET;
    }

    if ((http_client_result = uhttp_client_execute_request(http_handle, http_request_type, uri_path, http_headers_handle, json_to_send_str, json_to_send_str_len, on_edge_hsm_http_recv, workload_context)) != HTTP_CLIENT_OK)
    {
        LogError("uhttp_client_execute_request failed, result=%d", http_client_result);
    }
    else
    {
        do
        {
            uhttp_client_dowork(http_handle);
            timed_out = difftime(get_time(NULL), start_request_time) > HSM_HTTP_EDGE_MAXIMUM_REQUEST_TIME;
        } while ((workload_context->continue_running == true) && (timed_out == false));
    }

    return (workload_context->http_response != NULL) ? 0 : MU_FAILURE;
}

static BUFFER_HANDLE send_http_workload_request(HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge, const char* uri_path, BUFFER_HANDLE json_to_send)
{
    int result;
    HTTP_CLIENT_HANDLE http_handle = NULL;
    HTTP_HEADERS_HANDLE http_headers_handle = NULL;

    HTTP_CLIENT_RESULT http_open_result;
    HTTP_HEADERS_RESULT http_headers_result;

    SOCKETIO_CONFIG config;
    config.accepted_socket = NULL;
    config.hostname = hsm_client_http_edge->workload_hostname;
    config.port = hsm_client_http_edge->workload_portnumber;

    HSM_HTTP_WORKLOAD_CONTEXT workload_context;
    workload_context.continue_running = true;
    workload_context.http_response = NULL;

    if ((http_handle = uhttp_client_create(socketio_get_interface_description(), &config, on_edge_hsm_http_error, &workload_context)) == NULL)
    {
        LogError("uhttp_client_create failed");
        result = MU_FAILURE;
    }
    else if ((hsm_client_http_edge->workload_protocol_type == WORKLOAD_PROTOCOL_TYPE_UNIX_DOMAIN_SOCKET) &&
             (uhttp_client_set_option(http_handle, OPTION_ADDRESS_TYPE, OPTION_ADDRESS_TYPE_DOMAIN_SOCKET) != 0))
    {
        LogError("setting unix domain socket option failed");
        result = MU_FAILURE;
    }
    else if ((http_open_result = uhttp_client_open(http_handle, hsm_client_http_edge->workload_hostname, hsm_client_http_edge->workload_portnumber, on_edge_hsm_http_connected, &workload_context) != HTTP_CLIENT_OK) != HTTP_CLIENT_OK)
    {
        LogError("uhttp_client_open failed, err=%d", http_open_result);
        result = MU_FAILURE;
    }
    else if ((json_to_send != NULL) && ((http_headers_handle = HTTPHeaders_Alloc()) == NULL))
    {
        LogError("HTTPAPIEX_Create failed");
        result = MU_FAILURE;
    }
    else if ((json_to_send != NULL) && ((http_headers_result = HTTPHeaders_AddHeaderNameValuePair(http_headers_handle, HTTP_HEADER_KEY_CONTENT_TYPE, HTTP_HEADER_VAL_CONTENT_TYPE)) != HTTP_HEADERS_OK))
    {
        LogError("HTTPHeaders_AddHeaderNameValuePair failed, error=%d", http_headers_result);
        result = MU_FAILURE;
    }
    else if (send_request_to_edge_workload(http_handle, http_headers_handle, uri_path, json_to_send, &workload_context) != 0)
    {
        LogError("send_request_to_edge_workload failed");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    HTTPHeaders_Free(http_headers_handle);
    uhttp_client_close(http_handle, NULL, NULL);
    uhttp_client_destroy(http_handle);

    if (result != 0)
    {
        BUFFER_delete(workload_context.http_response);
        workload_context.http_response = NULL;
    }

    return workload_context.http_response;
}


static int parse_json_signing_response(BUFFER_HANDLE http_response, unsigned char** signed_value, size_t* signed_len)
{
    int result;
    const char* http_response_str;
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;
    const char* digest;

    if ((http_response_str = (const char*)BUFFER_u_char(http_response)) == NULL)
    {
        LogError("BUFFER_u_char reading http_response");
        result = MU_FAILURE;
    }
    else if ((root_value = json_parse_string(http_response_str)) == NULL)
    {
        LogError("json_parse_string failed");
        result = MU_FAILURE;
    }
    else if ((root_object = json_value_get_object(root_value)) == NULL)
    {
        LogError("json_value_get_object failed");
        result = MU_FAILURE;
    }
    else if ((digest = json_object_dotget_string(root_object, HSM_EDGE_SIGN_JSON_DIGEST)) == NULL)
    {
        LogError("json_value_get_object failed to get %s", HSM_EDGE_SIGN_JSON_DIGEST);
        result = MU_FAILURE;
    }
    else if (mallocAndStrcpy_s((char**)signed_value, digest) != 0)
    {
        LogError("Allocating signed_value failed");
        result = MU_FAILURE;
    }
    else
    {
        *signed_len = strlen(digest);
        result = 0;
    }

    json_object_clear(root_object);
    json_value_free(root_value);
    return result;
}

int hsm_client_http_edge_sign_data(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len)
{
    int result;
    BUFFER_HANDLE json_to_send = NULL;
    BUFFER_HANDLE http_response = NULL;
    STRING_HANDLE uri_path = NULL;

    if (handle == NULL || data == NULL || data_len == 0 || signed_value == NULL || signed_len == NULL)
    {
        LogError("Invalid handle value specified handle: %p, data: %p, data_len: %zu, signed_value: %p, signed_len: %p", handle, data, data_len, signed_value, signed_len);
        result = MU_FAILURE;
    }
    else
    {
        HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge = (HSM_CLIENT_HTTP_EDGE*)handle;

        if ((uri_path = STRING_construct_sprintf("/modules/%s/genid/%s/sign?api-version=%s", hsm_client_http_edge->module_id, hsm_client_http_edge->edge_module_generation_id, HSM_HTTP_EDGE_VERSION)) == NULL)
        {
            LogError("STRING_construct_sprintf failed");
            result = MU_FAILURE;
        }
        else if ((json_to_send = construct_json_signing_blob((const char*)data)) == NULL)
        {
            LogError("construct_json_signing_blob failed");
            result = MU_FAILURE;
        }
        else if ((http_response = send_http_workload_request(hsm_client_http_edge, STRING_c_str(uri_path), json_to_send)) == NULL)
        {
            LogError("send_http_workload_request failed");
            result = MU_FAILURE;
        }
        else if (parse_json_signing_response(http_response, signed_value, signed_len) != 0)
        {
            LogError("parse_json_signing_response failed");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }

    BUFFER_delete(json_to_send);
    BUFFER_delete(http_response);
    STRING_delete(uri_path);
    return result;
}

static char* parse_json_certificate_response(BUFFER_HANDLE http_response)
{
    char* trusted_certificates;
    const char* trusted_certificates_from_json;
    const char* http_response_str;
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;

    if ((http_response_str = (const char*)BUFFER_u_char(http_response)) == NULL)
    {
        LogError("BUFFER_u_char reading http_response");
        trusted_certificates = NULL;
    }
    else if ((root_value = json_parse_string(http_response_str)) == NULL)
    {
        LogError("json_parse_string failed");
        trusted_certificates = NULL;
    }
    else if ((root_object = json_value_get_object(root_value)) == NULL)
    {
        LogError("json_value_get_object failed");
        trusted_certificates = NULL;
    }
    else if ((trusted_certificates_from_json = json_object_dotget_string(root_object, HSM_EDGE_TRUSTED_CERTIFICATE_JSON_CERTIFICATE)) == NULL)
    {
        LogError("json_value_get_object failed to get %s", HSM_EDGE_TRUSTED_CERTIFICATE_JSON_CERTIFICATE);
        trusted_certificates = NULL;
    }
    else if (mallocAndStrcpy_s((char**)&trusted_certificates, trusted_certificates_from_json) != 0)
    {
        LogError("Allocating signed_value failed");
        trusted_certificates  = NULL;
    }

    json_object_clear(root_object);
    json_value_free(root_value);
    return trusted_certificates;
}

char* hsm_client_http_edge_get_trust_bundle(HSM_CLIENT_HANDLE handle)
{
    char* trusted_certificates;
    BUFFER_HANDLE http_response = NULL;
    STRING_HANDLE uri_path = NULL;

    if (handle == NULL)
    {
        LogError("Invalid handle value specified handle: %p", handle);
        trusted_certificates = NULL;
    }
    else
    {
        HSM_CLIENT_HTTP_EDGE* hsm_client_http_edge = (HSM_CLIENT_HTTP_EDGE*)handle;

        if ((uri_path = STRING_construct_sprintf("/trust-bundle?api-version=%s", HSM_HTTP_EDGE_VERSION)) == NULL)
        {
            LogError("STRING_construct_sprintf failed");
            trusted_certificates = NULL;
        }
        else if ((http_response = send_http_workload_request(hsm_client_http_edge, STRING_c_str(uri_path), NULL)) == NULL)
        {
            LogError("send_http_workload_request failed");
            trusted_certificates = NULL;
        }
        else if ((trusted_certificates = parse_json_certificate_response(http_response)) == NULL)
        {
            LogError("parse_json_certificate_response failed");
        }
    }

    STRING_delete(uri_path);
    BUFFER_delete(http_response);
    return trusted_certificates;
}
