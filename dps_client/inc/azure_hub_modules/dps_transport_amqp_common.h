// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_TRANSPORT_AMQP_COMMON_H
#define DPS_TRANSPORT_AMQP_COMMON_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_hub_modules/dps_transport.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

typedef struct DPS_TRANSPORT_IO_INFO_TAG
{
    XIO_HANDLE transport_handle;
    XIO_HANDLE sasl_handle;
} DPS_TRANSPORT_IO_INFO;

typedef DPS_TRANSPORT_IO_INFO*(*DPS_AMQP_TRANSPORT_IO)(const char* fully_qualified_name, SASL_MECHANISM_HANDLE* sasl_mechanism, const HTTP_PROXY_OPTIONS* proxy_info);

MOCKABLE_FUNCTION(, DPS_TRANSPORT_HANDLE, dps_transport_common_amqp_create, const char*, uri, DPS_HSM_TYPE, type, const char*, scope_id, const char*, registration_id, const char*, dps_api_version, DPS_AMQP_TRANSPORT_IO, transport_io);
MOCKABLE_FUNCTION(, void, dps_transport_common_amqp_destroy, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_open, DPS_TRANSPORT_HANDLE, handle, BUFFER_HANDLE, ek, BUFFER_HANDLE, srk, DPS_TRANSPORT_REGISTER_DATA_CALLBACK, data_callback, void*, user_ctx, DPS_TRANSPORT_STATUS_CALLBACK, status_cb, void*, status_ctx);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_close, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_register_device, DPS_TRANSPORT_HANDLE, handle, DPS_TRANSPORT_CHALLENGE_CALLBACK, reg_challenge_cb, void*, user_ctx, DPS_TRANSPORT_JSON_PARSE, json_parse_cb, void*, json_ctx);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_get_operation_status, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, void, dps_transport_common_amqp_dowork, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_set_trace, DPS_TRANSPORT_HANDLE, handle, bool, trace_on);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_set_proxy, DPS_TRANSPORT_HANDLE, handle, const HTTP_PROXY_OPTIONS*, proxy_options);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_set_trusted_cert, DPS_TRANSPORT_HANDLE, handle, const char*, certificate);
MOCKABLE_FUNCTION(, int, dps_transport_common_amqp_x509_cert, DPS_TRANSPORT_HANDLE, handle, const char*, certificate, const char*, private_key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_TRANSPORT_AMQP_COMMON_H
