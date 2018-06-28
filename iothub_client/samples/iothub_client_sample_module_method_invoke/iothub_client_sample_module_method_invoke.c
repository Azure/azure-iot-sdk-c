// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/crt_abstractions.h"

#include "iothub_module_client_ll.h"
#include "iothubtransportmqtt.h"

const char* targetDevice = "[target-device-id]";
const char* targetModule = "[target-module-id]";
const char* targetMethodName = "[method-name]";
const char* targetMethodPayload = "[json-payload]";
static unsigned int timeout = 60;


int main(void)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE handle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);
    if (handle == NULL)
    {
        (void)printf("IoTHubModuleClient_LL_CreateFromEnvironment failed\n");
    }
    else
    {
        (void)printf("Invoking method %s on module...\n", targetMethodName);

        int response;
        unsigned char* responsePayload;
        size_t responsePayloadSize;

        /* Unlike other LL functions, this call executes and blocks. It does NOT require a DoWork() */
        IOTHUB_CLIENT_RESULT invokeResult = IoTHubModuleClient_LL_ModuleMethodInvoke(handle, targetDevice, targetModule, targetMethodName, targetMethodPayload, timeout, &response, &responsePayload, &responsePayloadSize);
        if (invokeResult == IOTHUB_CLIENT_OK)
        {
            (void)printf("\r\nModule Method called\r\n");
            (void)printf("Module Method name:    %s\r\n", targetMethodName);
            (void)printf("Module Method payload: %s\r\n", targetMethodPayload);
            
            (void)printf("\r\nResponse status: %d\r\n", response);
            (void)printf("Response payload: %.*s\r\n", (int)responsePayloadSize, (const char*)responsePayload);

            free(responsePayload);
        }
        else
        {
            (void)printf("IoTHubModuleClient_LL_ModuleMethodInvoke failed with result: %d\n", invokeResult);
        }
        (void)printf("Destroying Module Client handle");
        IoTHubModuleClient_LL_Destroy(handle);
    }
}
