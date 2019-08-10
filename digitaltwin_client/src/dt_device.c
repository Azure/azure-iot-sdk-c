// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_device_client.h"
#include "iothub_client_options.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "digitaltwin_device_client.h"

#include "internal/dt_lock_thread_binding_impl.h"
#include "internal/dt_interface_private.h"
#include "internal/dt_client_core.h"

static int DeviceClientSendEventAsync(void* iothubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_SendEventAsync((IOTHUB_DEVICE_HANDLE)iothubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_SendEventAsync failed, error=%d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
   
    return result;
}

static int DeviceClientSetDeviceTwinCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_SetDeviceTwinCallback((IOTHUB_DEVICE_HANDLE)iothubClientHandle, deviceTwinCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_SetDeviceTwinCallback failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int DeviceClientSendReportedState(void* iothubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_SendReportedState((IOTHUB_DEVICE_HANDLE)iothubClientHandle, reportedState, size, reportedStateCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("DeviceClientSendReportedState failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int DeviceClientSetDeviceMethodCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;
    if ((iothubClientResult = IoTHubDeviceClient_SetDeviceMethodCallback((IOTHUB_DEVICE_HANDLE)iothubClientHandle, deviceMethodCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_SetDeviceMethodCallback failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static void DeviceClientDoWork(void* iothubClientHandle)
{
    (void)iothubClientHandle;
    // This should never be called, because digitaltwin_device never exposes out the wiring to reach this.
    LogError("DoWork is not supported for convenience layer");
}

static void DeviceClientDestroy(void* iothubClientHandle)
{
    IoTHubDeviceClient_Destroy((IOTHUB_DEVICE_CLIENT_HANDLE)iothubClientHandle);
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_DeviceClient_CreateFromDeviceHandle(IOTHUB_DEVICE_CLIENT_HANDLE deviceHandle, DIGITALTWIN_DEVICE_CLIENT_HANDLE* dtDeviceClientHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_CLIENT_RESULT iothubClientResult;
    bool urlEncodeOn = true;

    if ((deviceHandle == NULL) || (dtDeviceClientHandle == NULL))
    {
        LogError("DeviceHandle=%p, dtDeviceClientHandle=%p", deviceHandle, dtDeviceClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if ((iothubClientResult = IoTHubDeviceClient_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn)) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option %s, error=%d", OPTION_AUTO_URL_ENCODE_DECODE, iothubClientResult);
        LogError("NOTE: This error typically occurs when trying to use a non-supported DigitalTwin protocol.  You MUST use MQTT or MQTT_WS.  AMQP(_WS) and HTTP are not supported");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else 
    {
        DT_IOTHUB_BINDING iothubBinding;
        iothubBinding.iothubClientHandle = deviceHandle;
        iothubBinding.dtClientSendEventAsync = DeviceClientSendEventAsync;
        iothubBinding.dtClientSetTwinCallback = DeviceClientSetDeviceTwinCallback;
        iothubBinding.dtClientSendReportedState = DeviceClientSendReportedState;
        iothubBinding.dtClientSetMethodCallback = DeviceClientSetDeviceMethodCallback;
        iothubBinding.dtClientDoWork = DeviceClientDoWork;
        iothubBinding.dtClientDestroy = DeviceClientDestroy;

        DT_LOCK_THREAD_BINDING lockThreadBinding;
        lockThreadBinding.dtBindingLockHandle = NULL;
        lockThreadBinding.dtBindingLockInit = DT_LockBinding_LockInit_Impl;
        lockThreadBinding.dtBindingLock = DT_LockBinding_Lock_Impl;
        lockThreadBinding.dtBindingUnlock = DT_LockBinding_Unlock_Impl;
        lockThreadBinding.dtBindingLockDeinit = DT_LockBinding_LockDeinit_Impl;
        lockThreadBinding.dtBindingThreadSleep = DT_ThreadBinding_ThreadSleep_Impl;

        if ((*dtDeviceClientHandle = (DIGITALTWIN_DEVICE_CLIENT_HANDLE*)DT_ClientCoreCreate(&iothubBinding, &lockThreadBinding)) == NULL)
        {
            LogError("Failed allocating DigitalTwin device client");
            result = DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY;
        }
        else
        {   
            result = DIGITALTWIN_CLIENT_OK;
        }
    }

    return result;
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_DeviceClient_RegisterInterfacesAsync(DIGITALTWIN_DEVICE_CLIENT_HANDLE dtDeviceClientHandle, const char* deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaces, unsigned int numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback, void* userContextCallback)
{
    return DT_ClientCoreRegisterInterfacesAsync((DT_CLIENT_CORE_HANDLE)dtDeviceClientHandle, deviceCapabilityModel, dtInterfaces, numDTInterfaces, dtInterfaceRegisteredCallback, userContextCallback);
}

void DigitalTwin_DeviceClient_Destroy(DIGITALTWIN_DEVICE_CLIENT_HANDLE dtDeviceClientHandle)
{
    DT_ClientCoreDestroy((DT_CLIENT_CORE_HANDLE)dtDeviceClientHandle);
}

