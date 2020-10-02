// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBCLIENT_COMMON_DT_E2E_H
#define IOTHUBCLIENT_COMMON_DT_E2E_H

#include "iothub_client_ll.h"
#include "iothub_account.h"

// The ModelId is used when testing that the device client successfully sends the model
// to the IoT Hub.  It is OK that there is no DTDL that implements these models since
// this is opaque data to IoT Hub in any event.
#define TEST_MODEL_ID_1 "dtmi:azure:c-sdk:e2e-test;1"
#define TEST_MODEL_ID_2 "dtmi:azure:c-sdk:e2e-test;2"
#define TEST_MODEL_ID_3 "dtmi:azure:c-sdk:e2e-test;3"
#define TEST_MODEL_ID_4 "dtmi:azure:c-sdk:e2e-test;4"

extern void dt_e2e_init(bool testing_modules);
extern void dt_e2e_deinit(void);
extern void dt_e2e_send_reported_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod);
extern void dt_e2e_get_complete_desired_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod);
extern void dt_e2e_send_reported_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod);
extern void dt_e2e_get_complete_desired_test_svc_fault_ctrl_kill_Tcp(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod);
extern void dt_e2e_get_twin_async_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod);
extern void dt_e2e_send_module_id_test(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, IOTHUB_ACCOUNT_AUTH_METHOD accountAuthMethod, const char* modelId);


#endif /* IOTHUBCLIENT_COMMON_DT_E2E_H */
