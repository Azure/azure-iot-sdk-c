// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/wsio.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "iothubtransportmqtt_websockets.h"
#include "internal/iothubtransport_mqtt_common.h"

static XIO_HANDLE getWebSocketsIOTransport(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options)
{
    XIO_HANDLE result;
    const IO_INTERFACE_DESCRIPTION* io_interface_description = wsio_get_interface_description();
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;

    if (io_interface_description == NULL)
    {
        LogError("Failure constructing the provider interface");
        result = NULL;
    }
    else
    {
        WSIO_CONFIG ws_io_config;

        ws_io_config.hostname = fully_qualified_name;
        ws_io_config.port = 443;
        ws_io_config.protocol = "MQTT";
        ws_io_config.resource_name = "/$iothub/websocket";
        ws_io_config.underlying_io_interface = platform_get_default_tlsio();

        if (ws_io_config.underlying_io_interface == NULL)
        {
            ws_io_config.underlying_io_parameters = NULL;
        }
        else
        {
            ws_io_config.underlying_io_parameters = &tls_io_config;

            tls_io_config.hostname = fully_qualified_name;
            tls_io_config.port = 443;

            tls_io_config.invoke_on_send_complete_callback_for_fragments = false;

            if (mqtt_transport_proxy_options != NULL)
            {
                tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();

                if (tls_io_config.underlying_io_interface == NULL)
                {
                    tls_io_config.underlying_io_parameters = NULL;
                }
                else
                {
                    tls_io_config.underlying_io_parameters = &http_proxy_io_config;

                    http_proxy_io_config.proxy_hostname = mqtt_transport_proxy_options->host_address;
                    http_proxy_io_config.proxy_port = mqtt_transport_proxy_options->port;
                    http_proxy_io_config.username = mqtt_transport_proxy_options->username;
                    http_proxy_io_config.password = mqtt_transport_proxy_options->password;
                    http_proxy_io_config.hostname = fully_qualified_name;
                    http_proxy_io_config.port = 443;
                }
            }
            else
            {
                tls_io_config.underlying_io_interface = NULL;
                tls_io_config.underlying_io_parameters = NULL;
            }
        }

        result = xio_create(io_interface_description, &ws_io_config);
    }
    return result;
}

static TRANSPORT_LL_HANDLE IoTHubTransportMqtt_WS_Create(const IOTHUBTRANSPORT_CONFIG* config, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    return IoTHubTransport_MQTT_Common_Create(config, getWebSocketsIOTransport, cb_info, ctx);
}

static void IoTHubTransportMqtt_WS_Destroy(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_MQTT_Common_Destroy(handle);
}

static int IoTHubTransportMqtt_WS_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_MQTT_Common_Subscribe(handle);
}

static void IoTHubTransportMqtt_WS_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_MQTT_Common_Unsubscribe(handle);
}

static int IoTHubTransportMqtt_WS_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(handle);
}

static void IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(handle);
}

static int IoTHubTransportMqtt_WS_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    return IoTHubTransport_MQTT_Common_DeviceMethod_Response(handle, methodId, response, response_size, status_response);
}

static int IoTHubTransportMqtt_WS_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(handle);
}

static void IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    return IoTHubTransport_MQTT_Common_GetTwinAsync(handle, completionCallback, callbackContext);
}

static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportMqtt_WS_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    return IoTHubTransport_MQTT_Common_ProcessItem(handle, item_type, iothub_item);
}

static void IoTHubTransportMqtt_WS_DoWork(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_MQTT_Common_DoWork(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    return IoTHubTransport_MQTT_Common_GetSendStatus(handle, iotHubClientStatus);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    return IoTHubTransport_MQTT_Common_SetOption(handle, option, value);
}

static IOTHUB_DEVICE_HANDLE IoTHubTransportMqtt_WS_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    return IoTHubTransport_MQTT_Common_Register(handle, device, waitingToSend);
}

static void IoTHubTransportMqtt_WS_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    IoTHubTransport_MQTT_Common_Unregister(deviceHandle);
}

static STRING_HANDLE IoTHubTransportMqtt_WS_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    return IoTHubTransport_MQTT_Common_GetHostname(handle);
}

static int IoTHubTransportMqtt_WS_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitinSeconds)
{
    return IoTHubTransport_MQTT_Common_SetRetryPolicy(handle, retryPolicy, retryTimeoutLimitinSeconds);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportMqtt_WS_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE messageHandle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    return IoTHubTransport_MQTT_Common_SendMessageDisposition(handle, messageHandle, disposition);
}

static int IoTHubTransportMqtt_WS_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("IoTHubTransportMqtt_WS_Subscribe_InputQueue not implemented\n");
    return MU_FAILURE;
}

static void IoTHubTransportMqtt_WS_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    LogError("IoTHubTransportMqtt_WS_Unsubscribe_InputQueue not implemented\n");
    (void)handle;
}

static int IotHubTransportMqtt_WS_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    return IoTHubTransport_MQTT_SetCallbackContext(handle, ctx);
}

static int IotHubTransportMqtt_WS_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
{
    return IoTHubTransport_MQTT_GetSupportedPlatformInfo(handle, info);
}

static TRANSPORT_PROVIDER thisTransportProvider_WebSocketsOverTls = {
    IoTHubTransportMqtt_WS_SendMessageDisposition,
    IoTHubTransportMqtt_WS_Subscribe_DeviceMethod,
    IoTHubTransportMqtt_WS_Unsubscribe_DeviceMethod,
    IoTHubTransportMqtt_WS_DeviceMethod_Response,
    IoTHubTransportMqtt_WS_Subscribe_DeviceTwin,
    IoTHubTransportMqtt_WS_Unsubscribe_DeviceTwin,
    IoTHubTransportMqtt_WS_ProcessItem,
    IoTHubTransportMqtt_WS_GetHostname,
    IoTHubTransportMqtt_WS_SetOption,
    IoTHubTransportMqtt_WS_Create,
    IoTHubTransportMqtt_WS_Destroy,
    IoTHubTransportMqtt_WS_Register,
    IoTHubTransportMqtt_WS_Unregister,
    IoTHubTransportMqtt_WS_Subscribe,
    IoTHubTransportMqtt_WS_Unsubscribe,
    IoTHubTransportMqtt_WS_DoWork,
    IoTHubTransportMqtt_WS_SetRetryPolicy,
    IoTHubTransportMqtt_WS_GetSendStatus,
    IoTHubTransportMqtt_WS_Subscribe_InputQueue,
    IoTHubTransportMqtt_WS_Unsubscribe_InputQueue,
    IotHubTransportMqtt_WS_SetCallbackContext,
    IoTHubTransportMqtt_WS_GetTwinAsync,
    IotHubTransportMqtt_WS_GetSupportedPlatformInfo
};

const TRANSPORT_PROVIDER* MQTT_WebSocket_Protocol(void)
{
    return &thisTransportProvider_WebSocketsOverTls;
}
