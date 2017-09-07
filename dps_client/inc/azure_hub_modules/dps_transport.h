// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_TRANSPORT_H
#define DPS_TRANSPORT_H

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

    struct DPS_TRANSPORT_PROVIDER_TAG;
    typedef struct DPS_TRANSPORT_PROVIDER_TAG DPS_TRANSPORT_PROVIDER;

    typedef void* DPS_TRANSPORT_HANDLE;

#define DPS_TRANSPORT_RESULT_VALUES     \
    DPS_TRANSPORT_RESULT_OK,            \
    DPS_TRANSPORT_RESULT_UNAUTHORIZED,  \
    DPS_TRANSPORT_RESULT_ERROR          \

    DEFINE_ENUM(DPS_TRANSPORT_RESULT, DPS_TRANSPORT_RESULT_VALUES);

#define DPS_HSM_TYPE_VALUES \
    DPS_HSM_TYPE_TPM,       \
    DPS_HSM_TYPE_X509

    DEFINE_ENUM(DPS_HSM_TYPE, DPS_HSM_TYPE_VALUES);

#define DPS_TRANSPORT_STATUS_VALUES     \
    DPS_TRANSPORT_STATUS_CONNECTED,     \
    DPS_TRANSPORT_STATUS_AUTHENTICATED, \
    DPS_TRANSPORT_STATUS_UNASSIGNED,    \
    DPS_TRANSPORT_STATUS_ASSIGNING,     \
    DPS_TRANSPORT_STATUS_ASSIGNED,      \
    DPS_TRANSPORT_STATUS_BLACKLISTED,   \
    DPS_TRANSPORT_STATUS_ERROR

    DEFINE_ENUM(DPS_TRANSPORT_STATUS, DPS_TRANSPORT_STATUS_VALUES);

    typedef struct DPS_JSON_INFO_TAG
    {
        DPS_TRANSPORT_STATUS dps_status;
        char* operation_id;
        BUFFER_HANDLE authorization_key;
        char* key_name;
        char* iothub_uri;
        char* device_id;
    } DPS_JSON_INFO;

    typedef void(*DPS_TRANSPORT_REGISTER_DATA_CALLBACK)(DPS_TRANSPORT_RESULT transport_result, BUFFER_HANDLE iothub_key, const char* assigned_hub, const char* device_id, void* user_ctx);
    typedef void(*DPS_TRANSPORT_STATUS_CALLBACK)(DPS_TRANSPORT_STATUS transport_status, void* user_ctx);
    typedef char*(*DPS_TRANSPORT_CHALLENGE_CALLBACK)(const unsigned char* nonce, size_t nonce_len, const char* key_name, void* user_ctx);
    typedef DPS_JSON_INFO*(*DPS_TRANSPORT_JSON_PARSE)(const char* json_document, void* user_ctx);


    typedef DPS_TRANSPORT_HANDLE(*pfdps_transport_create)(const char* uri, DPS_HSM_TYPE type, const char* scope_id, const char* registration_id, const char* dps_api_version);
    typedef void(*pfdps_transport_destroy)(DPS_TRANSPORT_HANDLE handle);
    typedef int(*pfdps_transport_open)(DPS_TRANSPORT_HANDLE handle, BUFFER_HANDLE ek, BUFFER_HANDLE srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK data_callback, void* user_ctx, DPS_TRANSPORT_STATUS_CALLBACK status_cb, void* status_ctx);
    typedef int(*pfdps_transport_close)(DPS_TRANSPORT_HANDLE handle);

    typedef int(*pfdps_transport_register_device)(DPS_TRANSPORT_HANDLE handle, DPS_TRANSPORT_CHALLENGE_CALLBACK reg_challenge_cb, void* user_ctx, DPS_TRANSPORT_JSON_PARSE json_parse_cb, void* json_ctx);
    typedef int(*pfdps_transport_get_operation_status)(DPS_TRANSPORT_HANDLE handle);
    typedef void(*pfdps_transport_dowork)(DPS_TRANSPORT_HANDLE handle);
    typedef int(*pfdps_transport_set_trace)(DPS_TRANSPORT_HANDLE handle, bool trace_on);
    typedef int(*pfdps_transport_set_x509_cert)(DPS_TRANSPORT_HANDLE handle, const char* certificate, const char* private_key);
    typedef int(*pfdps_transport_set_trusted_cert)(DPS_TRANSPORT_HANDLE handle, const char* certificate);
    typedef int(*pfdps_transport_set_proxy)(DPS_TRANSPORT_HANDLE handle, const HTTP_PROXY_OPTIONS* proxy_option);

    struct DPS_TRANSPORT_PROVIDER_TAG
    {
        pfdps_transport_create dps_transport_create;
        pfdps_transport_destroy dps_transport_destroy;
        pfdps_transport_open dps_transport_open;
        pfdps_transport_close dps_transport_close;
        pfdps_transport_register_device dps_transport_register;
        pfdps_transport_get_operation_status dps_transport_get_op_status;
        pfdps_transport_dowork dps_transport_dowork;
        pfdps_transport_set_trace dps_transport_set_trace;
        pfdps_transport_set_x509_cert dps_transport_x509_cert;
        pfdps_transport_set_trusted_cert dps_transport_trusted_cert;
        pfdps_transport_set_proxy dps_transport_set_proxy;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_TRANSPORT
