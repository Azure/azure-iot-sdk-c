// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubtransportamqp_websockets.h"
#include "azure_c_shared_utility/wsio.h"
#include "internal/iothubtransport_amqp_common.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"

#define DEFAULT_WS_PROTOCOL_NAME "AMQPWSB10"
#define DEFAULT_WS_RELATIVE_PATH "/$iothub/websocket"
#define DEFAULT_WS_PORT 443

static XIO_HANDLE getWebSocketsIOTransport(const char* fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options)
{
    WSIO_CONFIG ws_io_config;
    TLSIO_CONFIG tls_io_config;
    HTTP_PROXY_IO_CONFIG http_proxy_io_config;
    const IO_INTERFACE_DESCRIPTION* io_interface_description = wsio_get_interface_description();
    XIO_HANDLE result;

    if (io_interface_description == NULL)
    {
        LogError("Failure constructing the provider interface");
        result = NULL;
    }
    else
    {
        ws_io_config.hostname = fqdn;
        ws_io_config.port = DEFAULT_WS_PORT;
        ws_io_config.protocol = DEFAULT_WS_PROTOCOL_NAME;
        ws_io_config.resource_name = DEFAULT_WS_RELATIVE_PATH;
        ws_io_config.underlying_io_interface = platform_get_default_tlsio();

        if (ws_io_config.underlying_io_interface == NULL)
        {
            ws_io_config.underlying_io_parameters = NULL;
        }
        else
        {
            ws_io_config.underlying_io_parameters = &tls_io_config;

            tls_io_config.hostname = fqdn;
            tls_io_config.port = DEFAULT_WS_PORT;

            if (amqp_transport_proxy_options != NULL)
            {
                tls_io_config.underlying_io_interface = http_proxy_io_get_interface_description();

                if (tls_io_config.underlying_io_interface == NULL)
                {
                    tls_io_config.underlying_io_parameters = NULL;
                }
                else
                {
                    tls_io_config.underlying_io_parameters = &http_proxy_io_config;

                    http_proxy_io_config.proxy_hostname = amqp_transport_proxy_options->host_address;
                    http_proxy_io_config.proxy_port = amqp_transport_proxy_options->port;
                    http_proxy_io_config.username = amqp_transport_proxy_options->username;
                    http_proxy_io_config.password = amqp_transport_proxy_options->password;
                    http_proxy_io_config.hostname = fqdn;
                    http_proxy_io_config.port = DEFAULT_WS_PORT;
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

// API functions
static TRANSPORT_LL_HANDLE IoTHubTransportAMQP_WS_Create(const IOTHUBTRANSPORT_CONFIG* config, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    return IoTHubTransport_AMQP_Common_Create(config, getWebSocketsIOTransport, cb_info, ctx);
}

static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportAMQP_WS_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    return IoTHubTransport_AMQP_Common_ProcessItem(handle, item_type, iothub_item);
}

static void IoTHubTransportAMQP_WS_DoWork(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_DoWork(handle);
}

static int IoTHubTransportAMQP_WS_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe(handle);
}

static void IoTHubTransportAMQP_WS_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe(handle);
}

static int IoTHubTransportAMQP_WS_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(handle);
}

static void IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    return IoTHubTransport_AMQP_Common_GetTwinAsync(handle, completionCallback, callbackContext);
}

static int IoTHubTransportAMQP_WS_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(handle);
}

static void IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(handle);
}

static int IoTHubTransportAMQP_WS_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    return IoTHubTransport_AMQP_Common_DeviceMethod_Response(handle, methodId, response, response_size, status_response);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    return IoTHubTransport_AMQP_Common_GetSendStatus(handle, iotHubClientStatus);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    return IoTHubTransport_AMQP_Common_SetOption(handle, option, value);
}

static IOTHUB_DEVICE_HANDLE IoTHubTransportAMQP_WS_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    return IoTHubTransport_AMQP_Common_Register(handle, device, waitingToSend);
}

static void IoTHubTransportAMQP_WS_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    IoTHubTransport_AMQP_Common_Unregister(deviceHandle);
}

static void IoTHubTransportAMQP_WS_Destroy(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Destroy(handle);
}

static int IoTHubTransportAMQP_WS_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    return IoTHubTransport_AMQP_Common_SetRetryPolicy(handle, retryPolicy, retryTimeoutLimitInSeconds);
}

static STRING_HANDLE IoTHubTransportAMQP_WS_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_GetHostname(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_WS_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE message_handle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    return IoTHubTransport_AMQP_Common_SendMessageDisposition(handle, message_handle, disposition);
}

static int IotHubTransportAMQP_WS_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("AMQP WS does not support input queues");
    return (int)-1;
}

static void IotHubTransportAMQP_WS_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("AMQP WS does not support input queues");
}

static int IoTHubTransportAMQP_WS_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    return IoTHubTransport_AMQP_SetCallbackContext(handle, ctx);
}

static int IoTHubTransportAMQP_WS_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
{
    return IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(handle, info);
}

static TRANSPORT_PROVIDER thisTransportProvider_WebSocketsOverTls =
{
    IoTHubTransportAMQP_WS_SendMessageDisposition,                     /*pfIotHubTransport_Send_Message_Disposition IoTHubTransport_Send_Message_Disposition;*/
    IoTHubTransportAMQP_WS_Subscribe_DeviceMethod,                     /*pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;*/
    IoTHubTransportAMQP_WS_Unsubscribe_DeviceMethod,                   /*pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;*/
    IoTHubTransportAMQP_WS_DeviceMethod_Response,
    IoTHubTransportAMQP_WS_Subscribe_DeviceTwin,                       /*pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin;*/
    IoTHubTransportAMQP_WS_Unsubscribe_DeviceTwin,                     /*pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin;*/
    IoTHubTransportAMQP_WS_ProcessItem,                                /*pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem;*/
    IoTHubTransportAMQP_WS_GetHostname,                                /*pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname;*/
    IoTHubTransportAMQP_WS_SetOption,                                  /*pfIoTHubTransport_SetOption IoTHubTransport_SetOption;*/
    IoTHubTransportAMQP_WS_Create,                                     /*pfIoTHubTransport_Create IoTHubTransport_Create;*/
    IoTHubTransportAMQP_WS_Destroy,                                    /*pfIoTHubTransport_Destroy IoTHubTransport_Destroy;*/
    IoTHubTransportAMQP_WS_Register,                                   /*pfIotHubTransport_Register IoTHubTransport_Register;*/
    IoTHubTransportAMQP_WS_Unregister,                                 /*pfIotHubTransport_Unregister IoTHubTransport_Unregister;*/
    IoTHubTransportAMQP_WS_Subscribe,                                  /*pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;*/
    IoTHubTransportAMQP_WS_Unsubscribe,                                /*pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;*/
    IoTHubTransportAMQP_WS_DoWork,                                     /*pfIoTHubTransport_DoWork IoTHubTransport_DoWork;*/
    IoTHubTransportAMQP_WS_SetRetryPolicy,                             /*pfIoTHubTransport_SetRetryLogic IoTHubTransport_SetRetryPolicy;*/
    IoTHubTransportAMQP_WS_GetSendStatus,                              /*pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus;*/
    IotHubTransportAMQP_WS_Subscribe_InputQueue,                       /*pfIoTHubTransport_Subscribe_InputQueue IoTHubTransport_Subscribe_InputQueue; */
    IotHubTransportAMQP_WS_Unsubscribe_InputQueue,                     /*pfIoTHubTransport_Unsubscribe_InputQueue IoTHubTransport_Unsubscribe_InputQueue; */
    IoTHubTransportAMQP_WS_SetCallbackContext,                         /*pfIoTHubTransport_SetCallbackContext IoTHubTransport_SetCallbackContext; */
    IoTHubTransportAMQP_WS_GetTwinAsync,                               /*pfIoTHubTransport_GetTwinAsync IoTHubTransport_GetTwinAsync;*/
    IoTHubTransportAMQP_WS_GetSupportedPlatformInfo                         /*pfIoTHubTransport_GetSupportedPlatformInfo IoTHubTransport_GetSupportedPlatformInfo;*/
};

extern const TRANSPORT_PROVIDER* AMQP_Protocol_over_WebSocketsTls(void)
{
    return &thisTransportProvider_WebSocketsOverTls;
}
