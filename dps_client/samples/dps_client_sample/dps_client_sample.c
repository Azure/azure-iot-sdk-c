// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "iothubtransportmqtt.h"
#include "iothubtransportamqp.h"
#include "iothub_client_dps_ll.h"
#include "azure_hub_modules/dps_client.h"
#include "azure_hub_modules/secure_device_factory.h"

#include "azure_hub_modules/dps_transport_http_client.h"

#include "../../../certs/certs.h"

#include "iothub_client_version.h"

DEFINE_ENUM_STRINGS(DPS_ERROR, DPS_ERROR_VALUES);
DEFINE_ENUM_STRINGS(DPS_REGISTRATION_STATUS, DPS_REGISTRATION_STATUS_VALUES);

static const char* dps_uri = "global.azure-devices-provisioning.net";
static const char* dps_scope_id = "[DPS Id Scope]";

static bool g_trace_on = true;

#ifdef USE_OPENSSL
    static bool g_using_cert = true;
#else
    static bool g_using_cert = false;
#endif // USE_OPENSSL

static bool g_use_proxy = false;
static const char* PROXY_ADDRESS = "127.0.0.1";

#define PROXY_PORT                  8888
#define MESSAGES_TO_SEND            2
#define TIME_BETWEEN_MESSAGES       2

typedef struct DPS_CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char* iothub_uri;
    char* access_key_name;
    char* device_key;
    char* device_id;
    int registration_complete;
} DPS_CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    (void)printf("Stop message recieved from IoTHub\r\n");
    iothub_info->stop_running = 1;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void dps_registation_status(DPS_REGISTRATION_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        //DPS_CLIENT_SAMPLE_INFO* dps_user_ctx = (DPS_CLIENT_SAMPLE_INFO*)user_context;
        (void)printf("DPS Status: %s\r\n", ENUM_TO_STRING(DPS_REGISTRATION_STATUS, reg_status));
        if (reg_status == DPS_REGISTRATION_STATUS_CONNECTED)
        {
        }
        else if (reg_status == DPS_REGISTRATION_STATUS_REGISTERING)
        {
        }
        else if (reg_status == DPS_REGISTRATION_STATUS_ASSIGNING)
        {
        }
    }
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
            printf("iothub is not authenticated\r\n");
            iothub_info->connected = 0;
            iothub_info->stop_running = 1;
        }
    }
}

static void iothub_dps_register_device(DPS_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        DPS_CLIENT_SAMPLE_INFO* dps_user_ctx = (DPS_CLIENT_SAMPLE_INFO*)user_context;
        if (register_result == DPS_CLIENT_OK)
        {
            (void)printf("Registration Information received from DPS: %s!\r\n", iothub_uri);
            (void)mallocAndStrcpy_s(&dps_user_ctx->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&dps_user_ctx->device_id, device_id);
            dps_user_ctx->registration_complete = 1;
        }
        else
        {
            dps_user_ctx->registration_complete = 2;
        }
    }
}

static void on_dps_error_callback(DPS_ERROR error_type, void* user_context)
{
    if (user_context == NULL)
    {
        (void)printf("user_context is NULL\r\n");
    }
    else
    {
        DPS_CLIENT_SAMPLE_INFO* dps_user_ctx = (DPS_CLIENT_SAMPLE_INFO*)user_context;
        (void)printf("Failure encountered in DPS info %s\r\n", ENUM_TO_STRING(DPS_ERROR, error_type));
        dps_user_ctx->registration_complete = 2;
    }
}

int main()
{
    int result;
    if (platform_init() != 0)
    {
        (void)printf("platform_init failed\r\n");
        result = __LINE__;
    }
    else if (dps_secure_device_init() != 0)
    {
        (void)printf("dps_secure_device_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        const char* trusted_cert;
        IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;
        HTTP_PROXY_OPTIONS http_proxy;
        DPS_CLIENT_SAMPLE_INFO dps_user_ctx;
        DPS_TRANSPORT_PROVIDER_FUNCTION dps_transport;

        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        memset(&dps_user_ctx, 0, sizeof(DPS_CLIENT_SAMPLE_INFO));

        if (g_using_cert)
        {
            trusted_cert = certificates;
        }
        else
        {
            trusted_cert = NULL;
        }

        // Pick your transport
        dps_transport = DPS_HTTP_Protocol;

        //iothub_transport = MQTT_Protocol;
        iothub_transport = AMQP_Protocol;

        // Set ini
        dps_user_ctx.registration_complete = 0;
        dps_user_ctx.sleep_time = 10;

        printf("   DPS Version: %s\r\n", DPS_LL_Get_Version_String());
        printf("Iothub Version: %s\r\n", IoTHubClient_GetVersionString());

        if (g_use_proxy)
        {
            http_proxy.host_address = PROXY_ADDRESS;
            http_proxy.port = PROXY_PORT;
        }

        DPS_LL_HANDLE handle;
        if ((handle = DPS_LL_Create(dps_uri, dps_scope_id, dps_transport, on_dps_error_callback, &dps_user_ctx)) == NULL)
        {
            (void)printf("failed calling DPS_LL_Create\r\n");
            result = __LINE__;
        }
        else
        {
            DPS_LL_SetOption(handle, "logtrace", &g_trace_on);
            if (trusted_cert != NULL)
            {
                DPS_LL_SetOption(handle, "TrustedCerts", trusted_cert);
            }
            if (http_proxy.host_address != NULL)
            {
                DPS_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);
            }

            if (DPS_LL_Register_Device(handle, iothub_dps_register_device, &dps_user_ctx, dps_registation_status, &dps_user_ctx) != DPS_CLIENT_OK)
            {
                (void)printf("failed calling DPS_LL_Register_Device\r\n");
                result = __LINE__;
            }
            else
            {
                do
                {
                    DPS_LL_DoWork(handle);
                    ThreadAPI_Sleep(dps_user_ctx.sleep_time);
                } while (dps_user_ctx.registration_complete == 0);
            }
            DPS_LL_Destroy(handle);
        }

        if (dps_user_ctx.registration_complete != 1)
        {
            (void)printf("DPS registration failed!\r\n");
            result = __LINE__;
        }
        else
        {
            // Sending the iothub messages
            IOTHUB_CLIENT_LL_HANDLE iothub_client;
            if ((iothub_client = IoTHubClient_LL_CreateFromDeviceAuth(dps_user_ctx.iothub_uri, dps_user_ctx.device_id, iothub_transport)) == NULL)
            {
                (void)printf("failed create IoTHub client from connection string %s!\r\n", dps_user_ctx.iothub_uri);
                result = __LINE__;
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

                (void)IoTHubClient_LL_SetConnectionStatusCallback(iothub_client, iothub_connection_status, &iothub_info);

                IoTHubClient_LL_SetOption(iothub_client, "logtrace", &g_trace_on);
                if (trusted_cert != NULL)
                {
                    IoTHubClient_LL_SetOption(iothub_client, "TrustedCerts", trusted_cert);
                }

                if (g_use_proxy)
                {
                    IoTHubClient_LL_SetOption(iothub_client, OPTION_HTTP_PROXY, &http_proxy);
                }

                if (IoTHubClient_LL_SetMessageCallback(iothub_client, receive_msg_callback, &iothub_info) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
                }

                (void)printf("Sending 1 messages to IoTHub every %d seconds for %d messages (Send any message to stop)\r\n", TIME_BETWEEN_MESSAGES, MESSAGES_TO_SEND);
                do
                {
                    if (iothub_info.connected != 0)
                    {
                        // Send a message every TIME_BETWEEN_MESSAGES seconds
                        (void)tickcounter_get_current_ms(tick_counter_handle, &current_tick);
                        if ((current_tick - last_send_time) / 1000 > TIME_BETWEEN_MESSAGES)
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
                                if (IoTHubClient_LL_SendEventAsync(iothub_client, msg_handle, NULL, NULL) != IOTHUB_CLIENT_OK)
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
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                } while (iothub_info.stop_running == 0 && msg_count < MESSAGES_TO_SEND);
                result = 0;

                size_t index = 0;
                for (index = 0; index < 10; index++)
                {
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                }
                tickcounter_destroy(tick_counter_handle);
                IoTHubClient_LL_Destroy(iothub_client);
            }
        }
        free(dps_user_ctx.iothub_uri);
        free(dps_user_ctx.device_id);
        dps_secure_device_deinit();
        platform_deinit();
    }

    (void)printf("Press any key to continue:\r\n");
    (void)getchar();

    return result;
}
