// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/base64.h"

#include "azure_hub_modules/uhttp.h"

#include "dps_service_client.h"

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

typedef enum REGISTRATION_TYPE_TAG
{
    TYPE_ENROLL_GROUP,
    TYPE_ENROLL
} REGISTRATION_TYPE;

typedef struct DPS_SERVICE_CLIENT_TAG
{
    char* dps_uri;
    char* key_name;
    char* access_key;

    char* proxy_hostname;
    char* certificates;

    const char* certificate;
    const char* device_id;
    BUFFER_HANDLE endorsement_key;

    REGISTRATION_TYPE reg_type;
    HTTP_CONNECTION_STATE http_state;
} DPS_SERVICE_CLIENT;

#define DEFAULT_HTTPS_PORT          443
#define UID_LENGTH                  37
#define SAS_TOKEN_DEFAULT_LIFETIME  3600
#define EPOCH_TIME_T_VALUE          (time_t)0

static const char* ATTESTATION_VALUE_TPM = "TPM";
static const char* ATTESTATION_VALUE_X509 = "X509";

static const char* CREATE_ENROLLMENT_CONTENT_FMT = "{\"id\":\"%s\",\"desiredIotHub\":\"%s\",\"deviceId\":\"%s\",\"identityAttestationMechanism\":\"%s\",\"identityAttestationEndorsementKey\":\"%s\",\"initialDeviceTwin\":\"\",\"enableEntry\":\"%s\"}";

static const char* ENROLL_GROUP_PROVISION_CONTENT_FMT = "{ \"enrollmentGroupId\": \"%s\", \"attestation\": { \"tpm\": null, \"x509\": { \"clientCertificates\": null, \"signingCertificates\": {\"primary\": { \"certificate\": \"%s\", \"info\": {} } }, \"secondary\": null } }, \"initialTwinState\": null }";
static const char* ENROLL_PROVISION_CONTENT_FMT = "{ \"registrationId\": \"%s\", \"deviceId\": \"%s\", \"attestation\": { \"tpm\": { \"endorsementKey\": \"%s\" } }, \"initialTwinState\": null }";
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
        DPS_SERVICE_CLIENT* dps_client = (DPS_SERVICE_CLIENT*)callback_ctx;
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
        DPS_SERVICE_CLIENT* dps_client = (DPS_SERVICE_CLIENT*)callback_ctx;
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
        DPS_SERVICE_CLIENT* dps_client = (DPS_SERVICE_CLIENT*)callback_ctx;
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

static STRING_HANDLE construct_registration_path(const DPS_SERVICE_CLIENT* dps_client, const char* registration_id)
{
    STRING_HANDLE result;
    STRING_HANDLE registration_encode;
    STRING_HANDLE double_encode;

    if ((registration_encode = URL_EncodeString(registration_id)) == NULL)
    {
        LogError("Failed encode registration Id");
        result = NULL;
    }
    else if ((double_encode = URL_Encode(registration_encode)) == NULL)
    {
        LogError("Failed double encoding registration Id");
        result = NULL;
    }
    else
    {
        const char* path_fmt;
        if (dps_client->reg_type == TYPE_ENROLL_GROUP)
        {
            path_fmt = ENROLL_GROUP_PROVISION_PATH_FMT;
        }
        else
        {
            path_fmt = ENROLL_PROVISION_PATH_FMT;
        }
        if ((result = STRING_construct_sprintf(path_fmt, STRING_c_str(double_encode), DPS_API_VERSION)) == NULL)
        {
            LogError("Failed constructing provisioning path");
        }
        STRING_delete(registration_encode);
        STRING_delete(double_encode);
    }
    return result;
}

static HTTP_HEADERS_HANDLE construct_http_headers(const DPS_SERVICE_CLIENT* dps_client)
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

static char* construct_enrollment_content(const DPS_SERVICE_CLIENT* dps_client, const ENROLLMENT_INFO* enrollment_info)
{
    char* result;
    const char* attestation_type;
    size_t enrollment_content_len;

    if (dps_client->reg_type == TYPE_ENROLL_GROUP)
    {
        result = NULL;
    }
    else
    {
        if (enrollment_info->type == SECURE_DEVICE_TYPE_TPM)
        {
            attestation_type = ATTESTATION_VALUE_TPM;
        }
        else
        {
            attestation_type = ATTESTATION_VALUE_X509;
        }
        enrollment_content_len = strlen(CREATE_ENROLLMENT_CONTENT_FMT) + string_length(enrollment_info->registration_id) + string_length(enrollment_info->desired_iothub) + string_length(enrollment_info->device_id) + string_length(attestation_type) + string_length(enrollment_info->attestation_value) + 5;
        result = malloc(enrollment_content_len + 1);
        if (result == NULL)
        {
            LogError("failure allocating enrollment content");
        }
        else
        {
            if (sprintf(result, CREATE_ENROLLMENT_CONTENT_FMT, enrollment_info->registration_id, retrieve_json_string(enrollment_info->desired_iothub), retrieve_json_string(enrollment_info->device_id), attestation_type, enrollment_info->attestation_value, enrollment_info->enable_entry ? "true" : "false") == 0)
            {
                LogError("Failure constructing enrollment content value");
                free(result);
                result = NULL;
            }
        }
    }
    return result;
}

static char* construct_registration_content(const DPS_SERVICE_CLIENT* dps_client, const char* registration_id)
{
    char* result;
    size_t enrollment_content_len;

    if (dps_client->reg_type == TYPE_ENROLL_GROUP)
    {
        enrollment_content_len = strlen(ENROLL_GROUP_PROVISION_CONTENT_FMT) + strlen(registration_id) + strlen(dps_client->certificate);
        result = malloc(enrollment_content_len + 1);
        if (result != NULL)
        {
            if (sprintf(result, ENROLL_GROUP_PROVISION_CONTENT_FMT, registration_id, dps_client->certificate) == 0)
            {
                LogError("Failure constructing provision content value");
                free(result);
                result = NULL;
            }
        }
        else
        {
            LogError("Failure constructing provision content value");
            result = NULL;
        }
    }
    else
    {
        STRING_HANDLE encoded_ek;
        if ((encoded_ek = Base64_Encoder(dps_client->endorsement_key)) == NULL)
        {
            LogError("Failure encoding ek");
            result = NULL;
        }
        else
        {
            enrollment_content_len = strlen(ENROLL_PROVISION_CONTENT_FMT) + STRING_length(encoded_ek) + strlen(registration_id) + strlen(dps_client->device_id);
            result = malloc(enrollment_content_len + 1);
            if (result != NULL)
            {
                if (sprintf(result, ENROLL_PROVISION_CONTENT_FMT, registration_id, dps_client->device_id, STRING_c_str(encoded_ek)) == 0)
                {
                    LogError("Failure constructing provision content value");
                    free(result);
                    result = NULL;
                }
            }
            else
            {
                LogError("Failure constructing provision content value");
                result = NULL;
            }
            STRING_delete(encoded_ek);
        }
    }
    return result;
}

static HTTP_CLIENT_HANDLE connect_to_service(DPS_SERVICE_CLIENT* dps_client)
{
    HTTP_CLIENT_HANDLE result;

    // Create uhttp
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy;
    memset(&tls_io_config, 0, sizeof(TLSIO_CONFIG));
    tls_io_config.hostname = dps_client->dps_uri;
    tls_io_config.port = DEFAULT_HTTPS_PORT;

    // Setup proxy
    if (dps_client->proxy_hostname != NULL)
    {
        memset(&http_proxy, 0, sizeof(HTTP_PROXY_IO_CONFIG));
        http_proxy.hostname = dps_client->dps_uri;
        http_proxy.port = 443;
        http_proxy.proxy_hostname = dps_client->dps_uri;
        http_proxy.proxy_port = DEFAULT_HTTPS_PORT;

        tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();
        tls_io_config.underlying_io_parameters = &http_proxy;
    }

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
    else if (dps_client->certificates && uhttp_client_set_trusted_cert(result, dps_client->certificates) != HTTP_CLIENT_OK)
    {
        LogError("Failed setting trusted cert");
        uhttp_client_destroy(result);
        result = NULL;
    }
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

static int send_delete_to_dps(DPS_SERVICE_CLIENT* dps_client, const char* registration_id)
{
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
        if ((registration_path = construct_registration_path(dps_client, registration_id)) == NULL)
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
                    if ((request_headers = construct_http_headers(dps_client)) == NULL)
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
                else if (dps_client->http_state == HTTP_STATE_REQUEST_RECV)
                {
                    dps_client->http_state = HTTP_STATE_COMPLETE;
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

static int send_enrollment_to_dps(DPS_SERVICE_CLIENT* dps_client, const ENROLLMENT_INFO* enrollment_info)
{
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
        if ((registration_path = construct_registration_path(dps_client, enrollment_info->registration_id)) == NULL)
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
                    char* content = construct_enrollment_content(dps_client, enrollment_info);
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

static int send_registration_to_dps(DPS_SERVICE_CLIENT* dps_client, const char* registration_id)
{
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
        if ((registration_path = construct_registration_path(dps_client, registration_id)) == NULL)
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
                    // Construct registration content
                    char* content = construct_registration_content(dps_client, registration_id);
                    if (content == NULL)
                    {
                        LogError("Failure creating registration json content");
                        dps_client->http_state = HTTP_STATE_ERROR;
                        result = __LINE__;
                    }
                    else if ((request_headers = construct_http_headers(dps_client)) == NULL)
                    {
                        LogError("Failure creating registration json content");
                        free(content);
                        dps_client->http_state = HTTP_STATE_ERROR;
                        result = __LINE__;
                    }
                    else
                    {
                        size_t content_len = strlen(content);
                        if (uhttp_client_execute_request(http_client, HTTP_CLIENT_REQUEST_PUT, STRING_c_str(registration_path), request_headers, (unsigned char*)content, content_len, on_http_reply_recv, &dps_client) != HTTP_CLIENT_OK)
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
                        free(content);
                    }
                }
                else if (dps_client->http_state == HTTP_STATE_REQUEST_RECV)
                {
                    dps_client->http_state = HTTP_STATE_COMPLETE;
                }
                else if (dps_client->http_state == HTTP_STATE_ERROR)
                {
                    result = __LINE__;
                }
            } while (dps_client->http_state != HTTP_STATE_COMPLETE && dps_client->http_state != HTTP_STATE_ERROR);

            STRING_delete(registration_path);
            uhttp_client_close(http_client, NULL, NULL);
            uhttp_client_destroy(http_client);
        }
    }
    return result;
}

DPS_SERVICE_CLIENT_HANDLE dps_service_create(const char* hostname, const char* key_name, const char* key)
{
    DPS_SERVICE_CLIENT* result;

    if (hostname == NULL || key_name == NULL || key == NULL)
    {
        LogError("invalid parameter hostname: %p, key_name: %p, key: %p", hostname, key_name, key);
        result = NULL;
    }
    else if ((result = malloc(sizeof(DPS_SERVICE_CLIENT))) == NULL)
    {
        LogError("Allocation of dps service client failed");
    }
    else
    {
        memset(result, 0, sizeof(DPS_SERVICE_CLIENT));
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
    return result;
}

void dps_serivce_destroy(DPS_SERVICE_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle->proxy_hostname);
        free(handle->certificates);

        free(handle->dps_uri);
        free(handle->key_name);
        free(handle->access_key);
        free(handle);
    }
}

int dps_service_create_enrollment(DPS_SERVICE_CLIENT_HANDLE handle, const ENROLLMENT_INFO* device_enrollment)
{
    int result;
    if (handle == NULL || device_enrollment == NULL)
    {
        LogError("invalid parameter handle: %p, device_enrollment: %p", handle, device_enrollment);
        result = __FAILURE__;
    }
    else if (device_enrollment->registration_id == NULL)
    {
        LogError("invalid parameter registration_id: %p", device_enrollment->registration_id);
        result = __FAILURE__;
    }
    else
    {
        handle->http_state = HTTP_STATE_DISCONNECTED;
        handle->reg_type = TYPE_ENROLL;
        if (send_enrollment_to_dps(handle, device_enrollment) != 0)
        {
            LogError("Failed sending registration to dps");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int dps_service_enroll_tpm_device(DPS_SERVICE_CLIENT_HANDLE handle, const char* registration_id, const char* device_id, BUFFER_HANDLE endorsement_key)
{
    int result;
    if (handle == NULL || registration_id == NULL || device_id == NULL || endorsement_key == NULL)
    {
        LogError("invalid parameter handle: %p, registration_id: %p, device_id: %p, endorsement_key: %p", handle, registration_id, device_id, endorsement_key);
        result = __FAILURE__;
    }
    else
    {
        handle->http_state = HTTP_STATE_DISCONNECTED;
        handle->device_id = device_id;
        handle->endorsement_key = endorsement_key;
        handle->reg_type = TYPE_ENROLL;
        if (send_registration_to_dps(handle, registration_id) != 0)
        {
            LogError("Failed sending registration to dps");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int dps_service_enroll_x509_device(DPS_SERVICE_CLIENT_HANDLE handle, const char* registration_id, const char* certificate)
{
    int result;
    if (handle == NULL || registration_id == NULL || certificate == NULL)
    {
        LogError("invalid parameter handle: %p, registration_id: %p, certificate: %p", handle, registration_id, certificate);
        result = __FAILURE__;
    }
    else
    {
        handle->http_state = HTTP_STATE_DISCONNECTED;
        handle->certificate = certificate;
        handle->reg_type = TYPE_ENROLL;
        if (send_registration_to_dps(handle, registration_id) != 0)
        {
            LogError("Failed sending registration to dps");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int dps_service_remove_enrollment(DPS_SERVICE_CLIENT_HANDLE handle, const char* registration_id)
{
    int result;
    if (handle == NULL || registration_id == NULL)
    {
        LogError("invalid parameter handle: %p, registration_id: %p", handle, registration_id);
        result = __FAILURE__;
    }
    else
    {
        handle->http_state = HTTP_STATE_DISCONNECTED;
        if (send_delete_to_dps(handle, registration_id) != 0)
        {
            LogError("Failed sending registration to dps");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}
