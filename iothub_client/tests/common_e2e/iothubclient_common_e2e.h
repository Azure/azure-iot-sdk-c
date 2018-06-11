// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBCLIENT_COMMON_E2E_H
#define IOTHUBCLIENT_COMMON_E2E_H

#include "iothub_device_client.h"
#include "iothub_account.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct E2E_TEST_OPTIONS_TAG
{
    bool set_mac_address;
    bool use_special_chars;
} E2E_TEST_OPTIONS;

typedef enum TEST_MESSAGE_CREATION_MECHANISM_TAG
{
    TEST_MESSAGE_CREATE_BYTE_ARRAY,
    TEST_MESSAGE_CREATE_STRING
} TEST_MESSAGE_CREATION_MECHANISM;

typedef enum TEST_PROTOCOL_TYPE_TAG
{
    TEST_MQTT,
    TEST_MQTT_WEBSOCKETS,
    TEST_AMQP,
    TEST_AMQP_WEBSOCKETS,
    TEST_HTTP
} TEST_PROTOCOL_TYPE;

extern E2E_TEST_OPTIONS g_e2e_test_options;
extern IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo;

extern void e2e_init(TEST_PROTOCOL_TYPE protocol_type, bool testing_modules);
extern void e2e_deinit(void);

extern void e2e_send_event_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_send_event_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_recv_message_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_recv_message_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void e2e_d2c_svc_fault_ctrl_kill_TCP_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_kill_TCP_connection_with_transport_status_check(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_session(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_request_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_response_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_D2C_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_kill_C2D_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_AMQP_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_MQTT_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_MQTT_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_MQTT_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_d2c_svc_fault_ctrl_MQTT_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern void e2e_c2d_svc_fault_ctrl_kill_TCP_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_session(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_request_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_response_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_D2C_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_kill_C2D_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_AMQP_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
extern void e2e_c2d_svc_fault_ctrl_MQTT_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

typedef void* D2C_MESSAGE_HANDLE;

extern bool client_wait_for_d2c_confirmation(D2C_MESSAGE_HANDLE d2cMessage, IOTHUB_CLIENT_CONFIRMATION_RESULT expectedClientResult);
extern bool client_received_confirmation(D2C_MESSAGE_HANDLE d2cMessage, IOTHUB_CLIENT_CONFIRMATION_RESULT expectedClientResult);

extern D2C_MESSAGE_HANDLE client_create_with_properies_and_send_d2c(IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle, MAP_HANDLE mapHandle);
extern bool client_wait_for_connection_fault();
extern bool client_status_fault_happened();
extern void clear_connection_status_info_flags();
extern bool client_wait_for_connection_restored();
extern bool client_status_restored();
extern bool wait_for_client_authenticated(size_t wait_time);

extern void service_wait_for_d2c_event_arrival(IOTHUB_PROVISIONED_DEVICE* deviceToUse, D2C_MESSAGE_HANDLE d2cMessage);
extern bool service_received_the_message(D2C_MESSAGE_HANDLE d2cMessage);
extern void destroy_d2c_message_handle(D2C_MESSAGE_HANDLE d2cMessage);


#ifdef __cplusplus
}
#endif

#endif /* IOTHUBCLIENT_COMMON_E2E_H */
