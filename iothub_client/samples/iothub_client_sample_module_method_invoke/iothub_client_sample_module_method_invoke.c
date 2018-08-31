// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample demonstrates how a module can invoke a method on another module directly.
// NOTE: This will only work when both modules are running on an IoT Edge device.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"

#include "iothub_module_client.h"
#include "iothubtransportmqtt.h"

const char* targetDevice = "[target-device-id]";  // This can also be queried at runtime be programatically retrieving the environment variable IOTEDGE_DEVICEID.
const char* targetModule = "[target-module-id]";
const char* targetMethodName = "[method-name]";
const char* targetMethodPayload = "[json-payload]"; // This must be a valid json value.  *If it is a string, it must be quoted* - e.g. targetMethodPayload = "\"[json-payload]\"";
static unsigned int timeout = 60;

static int moduleMethodInvokeCallbackCalled = 0;

static void ModuleMethodInvokeCallback(IOTHUB_CLIENT_RESULT result, int responseStatus, unsigned char* responsePayload, size_t responsePayloadSize, void* context)
{
    printf("ModuleMethodInvokeCallback called\n");
    printf("Result = %d, responseStatus = %d, context=%p\n", result, responseStatus, context);
    printf("Response payload: %.*s\n", (int)responsePayloadSize, (const char*)responsePayload);
    moduleMethodInvokeCallbackCalled = 1;
}

int main(void)
{
    // When running on a container created by IoT Edge, we don't need a connection string.  Instead the SDK can derive
    // its connection material - including security tokens - directly from the IoTHubModuleClient_CreateFromEnvironment call.
    IOTHUB_MODULE_CLIENT_HANDLE handle = IoTHubModuleClient_CreateFromEnvironment(MQTT_Protocol);
    if (handle == NULL)
    {
        (void)printf("IoTHubModuleClient_LL_CreateFromEnvironment failed\n");
    }
    else
    {
        // Invoke 'targetMethodName' on module ''targetModule'.
        IOTHUB_CLIENT_RESULT invokeAsyncResult = IoTHubModuleClient_ModuleMethodInvokeAsync(handle, targetDevice, targetModule, targetMethodName, targetMethodPayload, 1, ModuleMethodInvokeCallback, (void*)0x1234);
        if (invokeAsyncResult == IOTHUB_CLIENT_OK)
        {
            // Because we're using the convenience layer, the callback happens asyncronously.  Wait here for it to complete.
            (void)printf("Module Method called.  Waiting for response\n");

            while (moduleMethodInvokeCallbackCalled == 0)
            {
                ThreadAPI_Sleep(100);
            }
            printf("Callback called.  Breaking out of loop\n");
        }
        else
        {
            (void)printf("IoTHubModuleClient_LL_ModuleMethodInvoke failed with result: %d\n", invokeAsyncResult);
        }
        (void)printf("Destroying Module Client handle\n");
        IoTHubModuleClient_Destroy(handle);
    }
}

