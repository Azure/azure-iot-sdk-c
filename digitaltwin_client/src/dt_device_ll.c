// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"

#include "digitaltwin_device_client_ll.h"

#include "internal/dt_lock_thread_binding_stub.h"
#include "internal/dt_interface_private.h"
#include "internal/dt_client_core.h"

static int DeviceClient_LL_SendEventAsync(void* iothubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_LL_SendEventAsync((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_LL_SendEventAsync failed, error=%d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
   
    return result;
}

static int DeviceClientSetDeviceLLTwinCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_LL_SetDeviceTwinCallback((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle, deviceTwinCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_LL_SetDeviceTwinCallback failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int DeviceClient_LL_SendReportedState(void* iothubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;

    if ((iothubClientResult = IoTHubDeviceClient_LL_SendReportedState((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle, reportedState, size, reportedStateCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("DeviceClient_LL_SendReportedState failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static int DeviceClient_LL_SetDeviceMethodCallback(void* iothubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    int result;
    if ((iothubClientResult = IoTHubDeviceClient_LL_SetDeviceMethodCallback((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle, deviceMethodCallback, userContextCallback)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubDeviceClient_LL_SetDeviceMethodCallback failed, error = %d", iothubClientResult);
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

static void DeviceClient_LL_Destroy(void* iothubClientHandle)
{
    IoTHubDeviceClient_LL_Destroy((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle);
}

static void DeviceClient_LL_DoWork(void* iothubClientHandle)
{
    IoTHubDeviceClient_LL_DoWork((IOTHUB_DEVICE_CLIENT_LL_HANDLE)iothubClientHandle);
}

DIGITALTWIN_CLIENT_RESULT DigitalTwin_DeviceClient_LL_CreateFromDeviceHandle(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceLLHandle, DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE* dtDeviceLLClientHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;
    IOTHUB_CLIENT_RESULT iothubClientResult;
    bool urlEncodeOn = true;

    if ((deviceLLHandle == NULL) || (dtDeviceLLClientHandle == NULL))
    {
        LogError("deviceLLHandle=%p, dtDeviceLLClientHandle=%p", deviceLLHandle, dtDeviceLLClientHandle);
        result = DIGITALTWIN_CLIENT_ERROR_INVALID_ARG;
    }
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceLLHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn)) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option %s, error=%d", OPTION_AUTO_URL_ENCODE_DECODE, iothubClientResult);
        LogError("NOTE: This error typically occurs when trying to use a non-supported DigitalTwin protocol.  You MUST use MQTT or MQTT_WS.  AMQP(_WS) and HTTP are not supported");
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    else 
    {
        DT_IOTHUB_BINDING iothubBinding;
        iothubBinding.iothubClientHandle = deviceLLHandle;
        iothubBinding.dtClientSendEventAsync = DeviceClient_LL_SendEventAsync;
        iothubBinding.dtClientSetTwinCallback = DeviceClientSetDeviceLLTwinCallback;
        iothubBinding.dtClientSendReportedState = DeviceClient_LL_SendReportedState;
        iothubBinding.dtClientSetMethodCallback = DeviceClient_LL_SetDeviceMethodCallback;
        iothubBinding.dtClientDoWork = DeviceClient_LL_DoWork;
        iothubBinding.dtClientDestroy = DeviceClient_LL_Destroy;

        DT_LOCK_THREAD_BINDING lockThreadBinding;
        lockThreadBinding.dtBindingLockHandle = NULL;
        lockThreadBinding.dtBindingLockInit = DT_LockBinding_LockInit_Stub;
        lockThreadBinding.dtBindingLock = DT_LockBinding_Lock_Stub;
        lockThreadBinding.dtBindingUnlock = DT_LockBinding_Unlock_Stub;
        lockThreadBinding.dtBindingLockDeinit = DT_LockBinding_LockDeinit_Stub;
        lockThreadBinding.dtBindingThreadSleep = DT_ThreadBinding_ThreadSleep_Stub;

        if ((*dtDeviceLLClientHandle = (DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE)DT_ClientCoreCreate(&iothubBinding, &lockThreadBinding)) == NULL)
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

DIGITALTWIN_CLIENT_RESULT DigitalTwin_DeviceClient_LL_RegisterInterfacesAsync(DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE dtDeviceClientHandle, const char* deviceCapabilityModel, DIGITALTWIN_INTERFACE_CLIENT_HANDLE* dtInterfaces, unsigned int numDTInterfaces, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK dtInterfaceRegisteredCallback, void* userContextCallback)
{
    return DT_ClientCoreRegisterInterfacesAsync((DT_CLIENT_CORE_HANDLE)dtDeviceClientHandle, deviceCapabilityModel, dtInterfaces, numDTInterfaces, dtInterfaceRegisteredCallback, userContextCallback);
}

void DigitalTwin_DeviceClient_LL_DoWork(DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE dtDeviceClientHandle)
{
    DT_ClientCoreDoWork((DT_CLIENT_CORE_HANDLE)dtDeviceClientHandle);
}

void DigitalTwin_DeviceClient_LL_Destroy(DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE dtDeviceClientHandle)
{
    DT_ClientCoreDestroy((DT_CLIENT_CORE_HANDLE)dtDeviceClientHandle);
}

