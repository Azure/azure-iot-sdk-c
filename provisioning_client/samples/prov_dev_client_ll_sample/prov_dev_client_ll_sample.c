// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <vld.h>
#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_message.h"
#include "iothub_client_version.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

//
// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
#include "iothub_prov_mqtt_transport.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "iothub_prov_mqtt_ws_transport.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
#include "iothub_prov_amqp_transport.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "iothub_prov_amqp_ws_transport.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "iothub_prov_http_transport.h"
#endif // SAMPLE_HTTP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

// This sample is to demostrate iothub reconnection with provisioning and should not
// be confused as production code

static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = "[ID Scope]";

#define MESSAGES_TO_SEND            2
#define TIME_BETWEEN_MESSAGES       2

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

static int device_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)size;
    int result;

    if (strcmp("getCarVIN", method_name) == 0)
    {
        const char deviceMethodResponse[] = "{ \"Response\": \"1HGCM82633A004352\" }";
        *response_size = sizeof(deviceMethodResponse) - 1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = 200;
    }
    else
    {
        // All other entries are ignored.
        const char deviceMethodResponse[] = "{ }";
        *response_size = sizeof(deviceMethodResponse) - 1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = -1;
    }
    return result;
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    (void)printf("Stop message recieved from IoTHub\r\n");
    iothub_info->stop_running = 1;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    size_t* msg_count = (size_t*)userContextCallback;
    // When a message is sent this callback will get envoked
    (*msg_count)++;
    (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%zu] for transmission to IoT Hub.\r\n", *msg_count);
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    if (user_context == NULL)
    {
        printf("iothub_connection_status user_context is NULL\r\n");
    }
    else
    {
        IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
        if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
        {
            iothub_info->connected = 1;
        }
        else
        {
            iothub_info->connected = 0;
            iothub_info->stop_running = 1;
        }
    }
}

static void reportedStateCallback(int status_code, void* userContextCallback)
{
    (void)userContextCallback;
    printf("Device Twin reported properties update completed with result: %d\r\n", status_code);
}

int main()
{
    PROVISIONING_AUTH_INFO prov_auth_info = { 0 };
    //prov_auth_info.attestation_type = SECURE_DEVICE_TYPE_TPM;
    prov_auth_info.attestation_type = SECURE_DEVICE_TYPE_X509;
    //prov_auth_info.attestation_type = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;

    bool traceOn = true;

    (void)IoTHub_Init();

    prov_auth_info.id_scope = id_scope;
    prov_auth_info.provisioning_uri = global_prov_uri;

    // Set the symmetric key if using they auth type
    //prov_auth_info.registration_id = "<registration_id>";
    //prov_auth_info.symmetric_key = "<symmetric_Key>";

    // Protocol to USE - HTTP, AMQP, AMQP_WS, MQTT, MQTT_WS
#ifdef SAMPLE_MQTT
    prov_auth_info.transport = IoTHub_Prov_MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    prov_auth_info.transport = IoTHub_Prov_MQTT_WS_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    prov_auth_info.transport = IoTHub_Prov_AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    prov_auth_info.transport = IoTHub_Prov_AMQP_WS_Protocol;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    prov_auth_info.transport = IoTHub_Prov_HTTP_Protocol;
#endif // SAMPLE_HTTP

    printf("Iothub API Version: %s\r\n", IoTHubClient_GetVersionString());

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    (void)printf("Creating IoTHub Device handle\r\n");
    if ((device_ll_handle = IoTHubDeviceClient_LL_CreateFromProvisioning(&prov_auth_info) ) == NULL)
    {
        (void)printf("failed create IoTHub client from connection string %s!\r\n", prov_auth_info.provisioning_uri);
    }
    else
    {
        IOTHUB_CLIENT_SAMPLE_INFO iothub_info;
        TICK_COUNTER_HANDLE tick_counter_handle = tickcounter_create();
        tickcounter_ms_t current_tick;
        tickcounter_ms_t last_send_time = 0;
        size_t msg_count = 0;
        iothub_info.stop_running = 0;
        iothub_info.connected = 0;

        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, iothub_connection_status, &iothub_info);

        // Set any option that are neccessary.
        // For available options please see the iothub_sdk_options.md documentation

        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle, receive_msg_callback, &iothub_info);
        //(void)IoTHubDeviceClient_GetTwinAsync(device_ll_handle, getCompleteDeviceTwinOnDemandCallback, NULL);
        (void)IoTHubDeviceClient_LL_SendReportedState(device_ll_handle, (const unsigned char*)"{ test: 3 }", strlen("{ test: 3 }"), reportedStateCallback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceMethodCallback(device_ll_handle, device_method_callback, NULL);

        (void)printf("Sending 1 messages to IoTHub every %d seconds for %d messages (Send any message to stop)\r\n", TIME_BETWEEN_MESSAGES, MESSAGES_TO_SEND);
        do
        {
            if (iothub_info.connected != 0)
            {
                // Send a message every TIME_BETWEEN_MESSAGES seconds
                (void)tickcounter_get_current_ms(tick_counter_handle, &current_tick);
                if ((current_tick - last_send_time) / 1000 > TIME_BETWEEN_MESSAGES && msg_count < MESSAGES_TO_SEND)
                {
                    static char msgText[1024];
                    sprintf_s(msgText, sizeof(msgText), "{ \"message_index\" : \"%zu\" }", msg_count);

                    IOTHUB_MESSAGE_HANDLE msg_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText));
                    if (msg_handle == NULL)
                    {
                        (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                    }
                    else
                    {
                        if (IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, msg_handle, send_confirm_callback, &msg_count) != IOTHUB_CLIENT_OK)
                        {
                            (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                        }
                        else
                        {
                            (void)tickcounter_get_current_ms(tick_counter_handle, &last_send_time);
                        }
                        IoTHubMessage_Destroy(msg_handle);
                    }
                }
            }
            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            ThreadAPI_Sleep(1);
        } while (iothub_info.stop_running == 0 && msg_count < MESSAGES_TO_SEND);

        size_t index = 0;
        for (index = 0; index < 10; index++)
        {
            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            ThreadAPI_Sleep(1);
        }
        tickcounter_destroy(tick_counter_handle);
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }

    prov_dev_security_deinit();

    // Free all the sdk subsystem
    IoTHub_Deinit();

    (void)printf("Press any enter to continue:\r\n");
    (void)getchar();

    return 0;
}
