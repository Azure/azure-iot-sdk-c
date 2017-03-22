// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBCLIENT_COMMON_E2E_H
#define IOTHUBCLIENT_COMMON_E2E_H

#include "iothub_client.h"
#include "iothub_account.h"

#ifdef __cplusplus
extern "C" {
#endif

extern IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo;

extern void e2e_init(void);
extern void e2e_deinit(void);

extern void e2e_send_event_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_send_event_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_recv_message_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_recv_message_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

typedef void* D2C_MESSAGE_HANDLE;

extern IOTHUB_CLIENT_HANDLE client_connect_to_hub(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern D2C_MESSAGE_HANDLE client_create_and_send_d2c(IOTHUB_CLIENT_HANDLE iotHubClientHandle);
extern bool client_wait_for_d2c_confirmation(D2C_MESSAGE_HANDLE d2cMessage);
extern bool client_received_confirmation(D2C_MESSAGE_HANDLE d2cMessage);
extern void service_wait_for_d2c_event_arrival(IOTHUB_PROVISIONED_DEVICE* deviceToUse, D2C_MESSAGE_HANDLE d2cMessage);
extern bool service_received_the_message(D2C_MESSAGE_HANDLE d2cMessage);
extern void destroy_d2c_message_handle(D2C_MESSAGE_HANDLE d2cMessage);


#ifdef __cplusplus
}
#endif

#endif /* IOTHUBCLIENT_COMMON_E2E_H */
