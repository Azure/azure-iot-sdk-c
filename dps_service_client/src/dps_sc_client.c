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

#include "azure_uhttp_c/uhttp.h"


#include "dps_sc_client.h"

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
typedef struct DPS_SC_TAG
{
    char* dps_uri;
    char* key_name;
    char* access_key;

    HTTP_CONNECTION_STATE http_state;

} DPS_SC;

#define UNREFERENCED_PARAMETER(x) x

#define IOTHUBHOSTNAME "HostName"
#define IOTHUBSHAREDACESSKEYNAME "SharedAccessKeyName"
#define IOTHUBSHAREDACESSKEY "SharedAccessKey"

#define DEFAULT_HTTPS_PORT          443
#define UID_LENGTH                  37
#define SAS_TOKEN_DEFAULT_LIFETIME  3600
#define EPOCH_TIME_T_VALUE          (time_t)0

#define UNREFERENCED_PARAMETER(x) x

static const char* CREATE_ENROLLMENT_CONTENT_FMT = "{\"id\":\"%s\",\"desiredIotHub\":\"%s\",\"deviceId\":\"%s\",\"identityAttestationMechanism\":\"%s\",\"identityAttestationEndorsementKey\":\"%s\",\"initialDeviceTwin\":\"\",\"enableEntry\":\"%s\"}";
static const char* ENROLL_GROUP_PROVISION_PATH_FMT = "/enrollmentGroups/%s?api-version=%s";
static const char* ENROLL_PROVISION_PATH_FMT = "/enrollments/%s?api-version=%s";
static const char* HEADER_KEY_AUTHORIZATION = "Authorization";
static const char* DPS_API_VERSION = "2017-08-31-preview";

static size_t string_length(const char* value)
{
    size_t result;
    if (value != NULL)
    {
        result = strlen(value);
    }
    else
    {
        result = 0;
    }
    return result;
}

static const char* retrieve_json_string(const char* value)
{
    const char* result;
    if (value != NULL)
    {
        result = value;
    }
    else
    {
        result = "";
    }
    return result;
}

static void on_http_connected(void* callback_ctx, HTTP_CALLBACK_REASON connect_result)
{
    if (callback_ctx != NULL)
    {
        DPS_SC* dps_client = (DPS_SC*)callback_ctx;
        if (connect_result == HTTP_CALLBACK_REASON_OK)
        {
            dps_client->http_state = HTTP_STATE_CONNECTED;
        }
        else
        {
            dps_client->http_state = HTTP_STATE_ERROR;
        }
    }
}

static void on_http_error(void* callback_ctx, HTTP_CALLBACK_REASON error_result)
{
    (void)error_result;
    if (callback_ctx != NULL)
    {
        DPS_SC* dps_client = (DPS_SC*)callback_ctx;
        dps_client->http_state = HTTP_STATE_ERROR;
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
    (void)content;
    (void)content_len;
    if (callback_ctx != NULL)
    {
        DPS_SC* dps_client = (DPS_SC*)callback_ctx;
        if (request_result == HTTP_CALLBACK_REASON_OK)
        {
            if (status_code >= 200 && status_code <= 299)
            {
                dps_client->http_state = HTTP_STATE_REQUEST_RECV;
            }
            else
            {
                dps_client->http_state = HTTP_STATE_ERROR;
            }
        }
        else
        {
            dps_client->http_state = HTTP_STATE_ERROR;
        }
    }
    else
    {
        LogError("Invalid callback context");
    }
}

static HTTP_HEADERS_HANDLE construct_http_headers(const DPS_SC* dps_client)
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

        STRING_HANDLE sas_token = SASToken_CreateString(dps_client->access_key, dps_client->dps_uri, dps_client->key_name, expiryTime);
        if (sas_token == NULL)
        {
            HTTPHeaders_Free(result);
            result = NULL;
        }
        else
        {
            if ((HTTPHeaders_AddHeaderNameValuePair(result, "UserAgent", "iothub_dps_prov_client/1.0") != HTTP_HEADERS_OK) ||
                (HTTPHeaders_AddHeaderNameValuePair(result, "Accept", "application/json") != HTTP_HEADERS_OK) ||
                (HTTPHeaders_AddHeaderNameValuePair(result, "Content-Type", "application/json; charset=utf-8") != HTTP_HEADERS_OK) ||
                (HTTPHeaders_AddHeaderNameValuePair(result, HEADER_KEY_AUTHORIZATION, STRING_c_str(sas_token)) != HTTP_HEADERS_OK))
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

    if ((registration_encode = URL_EncodeString(registration_id)) == NULL)
    {
        LogError("Failed encode registration Id");
        result = NULL;
    }
    else
    {
        if ((result = STRING_construct_sprintf(path_fmt, STRING_c_str(registration_encode), DPS_API_VERSION)) == NULL)
        {
            LogError("Failed constructing provisioning path");
        }
        STRING_delete(registration_encode);
    }
    return result;
}

static HTTP_CLIENT_HANDLE connect_to_service(DPS_SC* dps_client)
{
    HTTP_CLIENT_HANDLE result;

    // Create uhttp
    TLSIO_CONFIG tls_io_config;
    //HTTP_PROXY_IO_CONFIG http_proxy;
    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = dps_client->dps_uri;
    tls_io_config.port = DEFAULT_HTTPS_PORT;

    // // Setup proxy
    // if (dps_client->proxy_hostname != NULL)
    // {
    //     memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
    //     http_proxy.hostname = dps_client->dps_uri;
    //     http_proxy.port = 443;
    //     http_proxy.proxy_hostname = dps_client->dps_uri;
    //     http_proxy.proxy_port = DEFAULT_HTTPS_PORT;

    //     tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
    //     tls_io_config.underlying_io_parameters = &http_proxy;
    // }

    const IO_INTERFACE_DESCRIPTION* interface_desc = platform_get_default_tlsio();
    if (interface_desc == NULL)
    {
        LogError("platform default tlsio is NULL");
        result = NULL;
    }
    else if ((result = uhttp_client_create(interface_desc, &tls_io_config, on_http_error, dps_client)) == NULL)
    {
        LogError("Failed creating http object");
    }
    // else if (dps_client->certificates && uhttp_client_set_trusted_cert(result, dps_client->certificates) != HTTP_CLIENT_OK)
    // {
    //     LogError("Failed setting trusted cert");
    //     uhttp_client_destroy(result);
    //     result = NULL;
    // }
    else if (uhttp_client_open(result, dps_client->dps_uri, DEFAULT_HTTPS_PORT, on_http_connected, dps_client) != HTTP_CLIENT_OK)
    {
        LogError("Failed opening http url %s", dps_client->dps_uri);
        uhttp_client_destroy(result);
        result = NULL;
    }
    else
    {
        (void)uhttp_client_set_trace(result, true, true);
    }
    return result;
}

DPS_SC_HANDLE dps_sc_create_from_connection_string(const char* conn_string)
{
    DPS_SC* result;

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
                }
                if (hostname == NULL || key_name == NULL || key == NULL)
                {
                    LogError("invalid parameter hostname: %p, key_name: %p, key: %p", hostname, key_name, key);
                    result = NULL;
                }
                else if ((result = malloc(sizeof(DPS_SC))) == NULL)
                {
                    LogError("Allocation of dps service client failed");
                }
                else
                {
                    memset(result, 0, sizeof(DPS_SC));
                    if (mallocAndStrcpy_s(&result->dps_uri, hostname) != 0)
                    {
                        LogError("Failure allocating of dps uri");
                        free(result);
                    }
                    else if (mallocAndStrcpy_s(&result->key_name, key_name) != 0)
                    {
                        LogError("Failure allocating of keyname");
                        free(result);
                    }
                    else if (mallocAndStrcpy_s(&result->access_key, key) != 0)
                    {
                        LogError("Failure allocating of access key");
                        free(result);
                    }
                }
            }
        }
    }
    return result;
}


void dps_sc_destroy(DPS_SC_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle->dps_uri);
        free(handle->key_name);
        free(handle->access_key);
        free(handle);
    }
}

int dps_sc_create_or_update_enrollment(DPS_SC_HANDLE dps_client, const char* id, const ENROLLMENT* enrollment)
{
    UNREFERENCED_PARAMETER(id);

    //1. establish path
    //2. construct headers
    //3. make PUT call

    int result;
    HTTP_CLIENT_HANDLE http_client;

    http_client = connect_to_service(dps_client);
    if (http_client == NULL)
    {
        LogError("Failed connecting to service");
        result = __LINE__;
    }
    else
    {
        STRING_HANDLE registration_path;
        if ((registration_path = construct_registration_path(enrollment->registration_id, ENROLL_PROVISION_PATH_FMT)) == NULL)
        {
            LogError("Failed constructing provisioning path");
            result = __LINE__;
        }
        else
        {
            HTTP_HEADERS_HANDLE request_headers;
            result = 0;
            do
            {
                uhttp_client_dowork(http_client);
                if (dps_client->http_state == HTTP_STATE_CONNECTED)
                {
                    char* content = enrollment_toJson(enrollment);
                    if (content == NULL)
                    {
                        LogError("Failure creating registration json content");
                        dps_client->http_state = HTTP_STATE_ERROR;
                        result = __LINE__;
                    }
                    else if ((request_headers = construct_http_headers(dps_client)) == NULL)
                    {
                        LogError("Failure creating registration json content");
                        dps_client->http_state = HTTP_STATE_ERROR;
                        result = __LINE__;
                    }
                    else
                    {
                        if (uhttp_client_execute_request(http_client, HTTP_CLIENT_REQUEST_DELETE, STRING_c_str(registration_path), request_headers, NULL, 0, on_http_reply_recv, &dps_client) != HTTP_CLIENT_OK)
                        {
                            LogError("Failure executing http request");
                            dps_client->http_state = HTTP_STATE_ERROR;
                            result = __LINE__;
                        }
                        else
                        {
                            dps_client->http_state = HTTP_STATE_REQUEST_SENT;
                        }
                        HTTPHeaders_Free(request_headers);
                    }
                }
                else if (dps_client->http_state == HTTP_STATE_ERROR)
                {
                    result = __LINE__;
                }
            } while (dps_client->http_state != HTTP_STATE_COMPLETE && dps_client->http_state != HTTP_STATE_ERROR);
        }
    }
    return result;
}



















int dps_sc_delete_enrollment(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_enrollment(DPS_SC_HANDLE handle, const char* id, ENROLLMENT* enrollment)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment);
    return 0;
}

int dps_sc_delete_device_registration_status(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_device_registration_status(DPS_SC_HANDLE handle, const char* id, DEVICE_REGISTRATION_STATUS* reg_status)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(reg_status);
    return 0;
}

int dps_sc_create_or_update_enrollment_group(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT_GROUP* enrollment_group)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment_group);
    return 0;
}

int dps_sc_delete_enrollment_group(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_enrollment_group(DPS_SC_HANDLE handle, const char* id, ENROLLMENT_GROUP* enrollment_group)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment_group);
    return 0;
}
