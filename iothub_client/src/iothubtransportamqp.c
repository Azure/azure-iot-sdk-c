// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubtransportamqp.h"
#include "internal/iothubtransport_amqp_common.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"

#define RESULT_OK 0
#define DEFAULT_IOTHUB_AMQP_PORT 5671

static XIO_HANDLE getTLSIOTransport(const char* fqdn, const AMQP_TRANSPORT_PROXY_OPTIONS* amqp_transport_proxy_options)
{
    XIO_HANDLE result;
    TLSIO_CONFIG tls_io_config;

    (void)amqp_transport_proxy_options;

    const IO_INTERFACE_DESCRIPTION* io_interface_description = platform_get_default_tlsio();
    if (io_interface_description == NULL)
    {
        LogError("Failed obtaining default TLS IO interface");
        result = NULL;
    }
    else
    {
        tls_io_config.hostname = fqdn;
        tls_io_config.port = DEFAULT_IOTHUB_AMQP_PORT;

        tls_io_config.underlying_io_interface = NULL;
        tls_io_config.underlying_io_parameters = NULL;

        if ((result = xio_create(io_interface_description, &tls_io_config)) == NULL)
        {
            LogError("Failed to get the TLS/IO transport (xio_create failed)");
        }
    }

    return result;
}

// API functions
static TRANSPORT_LL_HANDLE IoTHubTransportAMQP_Create(const IOTHUBTRANSPORT_CONFIG* config, TRANSPORT_CALLBACKS_INFO* cb_info, void* ctx)
{
    return IoTHubTransport_AMQP_Common_Create(config, getTLSIOTransport, cb_info, ctx);
}

static IOTHUB_PROCESS_ITEM_RESULT IoTHubTransportAMQP_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    return IoTHubTransport_AMQP_Common_ProcessItem(handle, item_type, iothub_item);
}

static void IoTHubTransportAMQP_DoWork(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_DoWork(handle);
}

static int IoTHubTransportAMQP_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe(handle);
}

static void IoTHubTransportAMQP_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe(handle);
}

static int IoTHubTransportAMQP_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe_DeviceTwin(handle);
}

static void IoTHubTransportAMQP_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceTwin(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_GetTwinAsync(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK completionCallback, void* callbackContext)
{
    return IoTHubTransport_AMQP_Common_GetTwinAsync(handle, completionCallback, callbackContext);
}

static int IoTHubTransportAMQP_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_Subscribe_DeviceMethod(handle);
}

static void IoTHubTransportAMQP_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Unsubscribe_DeviceMethod(handle);
}

static int IoTHubTransportAMQP_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    return IoTHubTransport_AMQP_Common_DeviceMethod_Response(handle, methodId, response, response_size, status_response);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    return IoTHubTransport_AMQP_Common_GetSendStatus(handle, iotHubClientStatus);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    return IoTHubTransport_AMQP_Common_SetOption(handle, option, value);
}

static IOTHUB_DEVICE_HANDLE IoTHubTransportAMQP_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, PDLIST_ENTRY waitingToSend)
{
    return IoTHubTransport_AMQP_Common_Register(handle, device, waitingToSend);
}

static void IoTHubTransportAMQP_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    IoTHubTransport_AMQP_Common_Unregister(deviceHandle);
}

static void IoTHubTransportAMQP_Destroy(TRANSPORT_LL_HANDLE handle)
{
    IoTHubTransport_AMQP_Common_Destroy(handle);
}

static int IoTHubTransportAMQP_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    return IoTHubTransport_AMQP_Common_SetRetryPolicy(handle, retryPolicy, retryTimeoutLimitInSeconds);
}

static STRING_HANDLE IoTHubTransportAMQP_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    return IoTHubTransport_AMQP_Common_GetHostname(handle);
}

static IOTHUB_CLIENT_RESULT IoTHubTransportAMQP_SendMessageDisposition(IOTHUB_DEVICE_HANDLE handle, IOTHUB_MESSAGE_HANDLE message_handle, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    return IoTHubTransport_AMQP_Common_SendMessageDisposition(handle, message_handle, disposition);
}

static int IotHubTransportAMQP_Subscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("AMQP does not support input queues");
    return MU_FAILURE;
}

static void IotHubTransportAMQP_Unsubscribe_InputQueue(IOTHUB_DEVICE_HANDLE handle)
{
    (void)handle;
    LogError("AMQP does not support input queues");
}

static int IoTHubTransportAMQP_SetCallbackContext(TRANSPORT_LL_HANDLE handle, void* ctx)
{
    return IoTHubTransport_AMQP_SetCallbackContext(handle, ctx);
}

static int IoTHubTransportAMQP_GetSupportedPlatformInfo(TRANSPORT_LL_HANDLE handle, PLATFORM_INFO_OPTION* info)
{
    return IoTHubTransport_AMQP_Common_GetSupportedPlatformInfo(handle, info);
}

static TRANSPORT_PROVIDER thisTransportProvider =
{
    IoTHubTransportAMQP_SendMessageDisposition,     /*pfIotHubTransport_Send_Message_Disposition IoTHubTransport_Send_Message_Disposition;*/
    IoTHubTransportAMQP_Subscribe_DeviceMethod,     /*pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;*/
    IoTHubTransportAMQP_Unsubscribe_DeviceMethod,   /*pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;*/
    IoTHubTransportAMQP_DeviceMethod_Response,
    IoTHubTransportAMQP_Subscribe_DeviceTwin,       /*pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin;*/
    IoTHubTransportAMQP_Unsubscribe_DeviceTwin,     /*pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin;*/
    IoTHubTransportAMQP_ProcessItem,                /*pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem;*/
    IoTHubTransportAMQP_GetHostname,                /*pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname;*/
    IoTHubTransportAMQP_SetOption,                  /*pfIoTHubTransport_SetOption IoTHubTransport_SetOption;*/
    IoTHubTransportAMQP_Create,                     /*pfIoTHubTransport_Create IoTHubTransport_Create;*/
    IoTHubTransportAMQP_Destroy,                    /*pfIoTHubTransport_Destroy IoTHubTransport_Destroy;*/
    IoTHubTransportAMQP_Register,                   /*pfIotHubTransport_Register IoTHubTransport_Register;*/
    IoTHubTransportAMQP_Unregister,                 /*pfIotHubTransport_Unregister IoTHubTransport_Unregister;*/
    IoTHubTransportAMQP_Subscribe,                  /*pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;*/
    IoTHubTransportAMQP_Unsubscribe,                /*pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;*/
    IoTHubTransportAMQP_DoWork,                     /*pfIoTHubTransport_DoWork IoTHubTransport_DoWork;*/
    IoTHubTransportAMQP_SetRetryPolicy,             /*pfIoTHubTransport_DoWork IoTHubTransport_SetRetryPolicy;*/
    IoTHubTransportAMQP_GetSendStatus,              /*pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus;*/
    IotHubTransportAMQP_Subscribe_InputQueue,       /*pfIoTHubTransport_Subscribe_InputQueue IoTHubTransport_Subscribe_InputQueue; */
    IotHubTransportAMQP_Unsubscribe_InputQueue,     /*pfIoTHubTransport_Unsubscribe_InputQueue IoTHubTransport_Unsubscribe_InputQueue; */
    IoTHubTransportAMQP_SetCallbackContext,         /*pfIoTHubTransport_SetTransportCallbacks IoTHubTransport_SetTransportCallbacks; */
    IoTHubTransportAMQP_GetTwinAsync,               /*pfIoTHubTransport_GetTwinAsync IoTHubTransport_GetTwinAsync;*/
    IoTHubTransportAMQP_GetSupportedPlatformInfo      /*pfIoTHubTransport_GetSupportedPlatformInfo IoTHubTransport_GetSupportedPlatformInfo;*/
};

extern const TRANSPORT_PROVIDER* AMQP_Protocol(void)
{
    return &thisTransportProvider;
}
