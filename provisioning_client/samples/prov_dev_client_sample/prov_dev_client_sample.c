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
#include "iothub_client_hsm_ll.h"
#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_security_factory.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"

#include "../../../certs/certs.h"

#include "iothub_client_version.h"

DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* scope_id = "";

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

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    (void)user_context;
    (void)printf("Provisioning Status: %s\r\n", ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
    if (reg_status == PROV_DEVICE_REG_STATUS_CONNECTED)
    {
        (void)printf("\r\nRegistration status: CONNECTED\r\n");
    }
    else if (reg_status == PROV_DEVICE_REG_STATUS_REGISTERING)
    {
        (void)printf("\r\nRegistration status: REGISTERING\r\n");
    }
    else if (reg_status == PROV_DEVICE_REG_STATUS_ASSIGNING)
    {
        (void)printf("\r\nRegistration status: ASSIGNING\r\n");
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    (void)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK)
    {
        (void)printf("\r\nRegistration Information received from service: %s, deviceId: %s\r\n", iothub_uri, device_id);
    }
}

int main()
{
    int result;
    SECURE_DEVICE_TYPE hsm_type;
    //hsm_type = SECURE_DEVICE_TYPE_TPM;
    hsm_type = SECURE_DEVICE_TYPE_X509;

    if (platform_init() != 0)
    {
        (void)printf("platform_init failed\r\n");
        result = __LINE__;
    }
    else if (prov_dev_security_init(hsm_type) != 0)
    {
        (void)printf("prov_dev_security_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        const char* trusted_cert;

        HTTP_PROXY_OPTIONS http_proxy;
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;

        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));

        if (g_using_cert)
        {
            trusted_cert = certificates;
        }
        else
        {
            trusted_cert = NULL;
        }

        // Pick your transport
        prov_transport = Prov_Device_HTTP_Protocol;
        //prov_transport = Prov_Device_AMQP_Protocol;
        //prov_transport = PROV_DEVICE_MQTT_Protocol;

        printf("Provisioning API Version: %s\r\n", Prov_Device_GetVersionString());
        printf("Iothub API Version: %s\r\n", IoTHubClient_GetVersionString());

        if (g_use_proxy)
        {
            http_proxy.host_address = PROXY_ADDRESS;
            http_proxy.port = PROXY_PORT;
        }

        PROV_DEVICE_RESULT prov_device_result = PROV_DEVICE_RESULT_ERROR;
        PROV_DEVICE_HANDLE prov_device_handle;
        if ((prov_device_handle = Prov_Device_Create(global_prov_uri, scope_id, prov_transport)) == NULL)
        {
            (void)printf("failed calling Prov_Device_Create\r\n");
            result = __LINE__;
        }
        else
        {
            Prov_Device_SetOption(prov_device_handle, "logtrace", &g_trace_on);
            if (trusted_cert != NULL)
            {
                Prov_Device_SetOption(prov_device_handle, OPTION_TRUSTED_CERT, trusted_cert);
            }
            if (http_proxy.host_address != NULL)
            {
                Prov_Device_SetOption(prov_device_handle, OPTION_HTTP_PROXY, &http_proxy);
            }

            prov_device_result = Prov_Device_Register_Device(prov_device_handle, register_device_callback, NULL, registation_status_callback, NULL);

            (void)printf("\r\nRegistering... Press any key to interrupt.\r\n\r\n");
            (void)getchar();

            Prov_Device_Destroy(prov_device_handle);
        }
        prov_dev_security_deinit();
        platform_deinit();

        result = 0;
    }

    (void)printf("Press any key to exit:\r\n");
    (void)getchar();

    return result;
}
