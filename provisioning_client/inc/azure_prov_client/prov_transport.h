// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_TRANSPORT_H
#define PROV_TRANSPORT_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/buffer_.h"

#ifdef __cplusplus
extern "C" {
#include <cstdbool>
#else
#include <stdbool.h>
#endif /* __cplusplus */

    struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG;
    typedef struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG PROV_DEVICE_TRANSPORT_PROVIDER;

    typedef void* PROV_DEVICE_TRANSPORT_HANDLE;

#define TRANSPORT_HSM_TYPE_VALUES   \
    TRANSPORT_HSM_TYPE_TPM,         \
    TRANSPORT_HSM_TYPE_X509

    DEFINE_ENUM(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);

#define PROV_DEVICE_TRANSPORT_RESULT_VALUES     \
    PROV_DEVICE_TRANSPORT_RESULT_OK,            \
    PROV_DEVICE_TRANSPORT_RESULT_UNAUTHORIZED,  \
    PROV_DEVICE_TRANSPORT_RESULT_ERROR

    DEFINE_ENUM(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);

#define PROV_DEVICE_TRANSPORT_STATUS_VALUES     \
    PROV_DEVICE_TRANSPORT_STATUS_CONNECTED,     \
    PROV_DEVICE_TRANSPORT_STATUS_AUTHENTICATED, \
    PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED,    \
    PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING,     \
    PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED,      \
    PROV_DEVICE_TRANSPORT_STATUS_BLACKLISTED,   \
    PROV_DEVICE_TRANSPORT_STATUS_ERROR

    DEFINE_ENUM(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);

    typedef struct PROV_JSON_INFO_TAG
    {
        PROV_DEVICE_TRANSPORT_STATUS prov_status;
        char* operation_id;
        BUFFER_HANDLE authorization_key;
        char* key_name;
        char* iothub_uri;
        char* device_id;
    } PROV_JSON_INFO;

    typedef void(*PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK)(PROV_DEVICE_TRANSPORT_RESULT transport_result, BUFFER_HANDLE iothub_key, const char* assigned_hub, const char* device_id, void* user_ctx);
    typedef void(*PROV_DEVICE_TRANSPORT_STATUS_CALLBACK)(PROV_DEVICE_TRANSPORT_STATUS transport_status, void* user_ctx);
    typedef char*(*PROV_TRANSPORT_CHALLENGE_CALLBACK)(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx);
    typedef PROV_JSON_INFO*(*PROV_TRANSPORT_JSON_PARSE)(const char* json_document, void* user_ctx);


    typedef PROV_DEVICE_TRANSPORT_HANDLE(*pfprov_transport_create)(const char* uri, TRANSPORT_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* api_version);
    typedef void(*pfprov_transport_destroy)(PROV_DEVICE_TRANSPORT_HANDLE handle);
    typedef int(*pfprov_transport_open)(PROV_DEVICE_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, PROV_DEVICE_TRANSPORT_REGISTER_CALLBACK data_callback, void* user_ctx, PROV_DEVICE_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx);
    typedef int(*pfprov_transport_close)(PROV_DEVICE_TRANSPORT_HANDLE handle);

    typedef int(*pfprov_transport_register_device)(PROV_DEVICE_TRANSPORT_HANDLE handle, PROV_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, PROV_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx);
    typedef int(*pfprov_transport_get_operation_status)(PROV_DEVICE_TRANSPORT_HANDLE handle);
    typedef void(*pfprov_transport_dowork)(PROV_DEVICE_TRANSPORT_HANDLE handle);
    typedef int(*pfprov_transport_set_trace)(PROV_DEVICE_TRANSPORT_HANDLE handle, bool trace_on);
    typedef int(*pfprov_transport_set_x509_cert)(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key);
    typedef int(*pfprov_transport_set_trusted_cert)(PROV_DEVICE_TRANSPORT_HANDLE handle, const char* certificate);
    typedef int(*pfprov_transport_set_proxy)(PROV_DEVICE_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_option);

    struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG
    {
        pfprov_transport_create prov_transport_create;
        pfprov_transport_destroy prov_transport_destroy;
        pfprov_transport_open prov_transport_open;
        pfprov_transport_close prov_transport_close;
        pfprov_transport_register_device prov_transport_register;
        pfprov_transport_get_operation_status prov_transport_get_op_status;
        pfprov_transport_dowork prov_transport_dowork;
        pfprov_transport_set_trace prov_transport_set_trace;
        pfprov_transport_set_x509_cert prov_transport_x509_cert;
        pfprov_transport_set_trusted_cert prov_transport_trusted_cert;
        pfprov_transport_set_proxy prov_transport_set_proxy;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_TRANSPORT
