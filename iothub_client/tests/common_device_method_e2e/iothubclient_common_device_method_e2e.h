// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef COMMON_DEVICE_METHOD_E2E
#define COMMON_DEVICE_METHOD_E2E

#include "iothub_client.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern void device_method_e2e_init(bool testing_modules);

extern void device_method_e2e_deinit(void);

extern void device_method_function_cleanup();

extern void device_method_e2e_method_call_with_string_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_string_sas_multiplexed(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, size_t number_of_devices);

extern void device_method_e2e_method_call_with_double_quoted_json_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_empty_json_object_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_null_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_embedded_double_quote_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_embedded_single_quote_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_calls_upload_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_string_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_double_quoted_json_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_empty_json_object_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_NULL_json_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_null_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_embedded_double_quote_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_with_embedded_single_quote_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_calls_upload_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void device_method_e2e_method_call_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // COMMON_DEVICE_METHOD_E2E
