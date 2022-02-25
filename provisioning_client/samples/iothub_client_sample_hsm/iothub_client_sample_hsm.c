// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_prov_client/iothub_security_factory.h"

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportamqp.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

static const char* iothub_uri = "<iothub_uri>";
static const char* device_id = "<device_id>";
static const char* conn_string = "HostName=<hostname>;DeviceId=<device_id>;UseProvisioning=true";

#ifdef USE_OPENSSL
    static bool g_using_cert = true;
#else
    static bool g_using_cert = false;
#endif // USE_OPENSSL

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int stop_running;
    int connected;
} IOTHUB_CLIENT_SAMPLE_INFO;

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    if (iothub_info != NULL)
    {
        if (reason == IOTHUB_CLIENT_CONNECTION_OK && result == IOTHUB_CLIENT_CONFIRMATION_OK)
        {
            iothub_info->connected = 1;
        }
        else
        {
            iothub_info->connected = 0;
            (void)printf("Disconnection from service encountered");
        }
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    iothub_info->stop_running = 1;
    return IOTHUBMESSAGE_ACCEPTED;
}

int main(void)
{
    int result;
    IOTHUB_SECURITY_TYPE security_type;
    //security_type = IOTHUB_SECURITY_TYPE_SAS;
    security_type = IOTHUB_SECURITY_TYPE_X509;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)iothub_security_init(security_type);

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;
    //if ((iothub_client = IoTHubDeviceClient_LL_CreateFromDeviceAuth(iothub_uri, device_id, MQTT_Protocol)) == NULL)
    if ((device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(conn_string, MQTT_Protocol)) == NULL)
    {
        (void)printf("Failure creating device Auth!\r\n");
        result = __LINE__;
    }
    else
    {
        size_t msg_count = 1;
        IOTHUB_CLIENT_SAMPLE_INFO iothub_info;
        TICK_COUNTER_HANDLE tick_counter_handle = tickcounter_create();

        tickcounter_ms_t current_tick;
        tickcounter_ms_t last_send_time = 0;

        iothub_info.stop_running = 0;

        (void)IoTHubDeviceClient_LL_SetMessageCallback(device_ll_handle, receive_msg_callback, &iothub_info);
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, &iothub_info);

        // Set any option that are necessary.
        // For available options please see the iothub_sdk_options.md documentation
        //bool traceOn = true;
        //IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);
        // Setting the Trusted Certificate. This is only necessary on systems without
        // built in certificate stores.
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

        do
        {
            if (iothub_info.connected != 0)
            {
                (void)tickcounter_get_current_ms(tick_counter_handle, &current_tick);
                if ((current_tick - last_send_time) / 1000 > 10)
                {
                    static char msgText[1024];
                    sprintf_s(msgText, sizeof(msgText), "{ \"message_index\" : \"%zu\" }", msg_count++);

                    IOTHUB_MESSAGE_HANDLE msg_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText));
                    if (msg_handle == NULL)
                    {
                        (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                    }
                    else
                    {
                        if (IoTHubClient_LL_SendEventAsync(device_ll_handle, msg_handle, NULL, NULL) != IOTHUB_CLIENT_OK)
                        {
                            (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                        }
                        else
                        {
                            (void)tickcounter_get_current_ms(tick_counter_handle, &last_send_time);
                            (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%zu] for transmission to IoT Hub.\r\n", msg_count);
                        }
                        IoTHubMessage_Destroy(msg_handle);
                    }
                }
            }
            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            ThreadAPI_Sleep(1);
        } while (iothub_info.stop_running == 0);
        result = 0;

        size_t index = 0;
        for (index = 0; index < 10; index++)
        {
            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            ThreadAPI_Sleep(1);
        }
    }
    // Clean up the iothub sdk handle
    IoTHubDeviceClient_LL_Destroy(device_ll_handle);

    // Free all the sdk subsystem
    IoTHub_Deinit();

    return result;
}
