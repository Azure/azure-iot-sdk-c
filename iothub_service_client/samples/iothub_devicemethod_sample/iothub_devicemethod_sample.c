// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"

#include "iothub_service_client_auth.h"
#include "iothub_devicemethod.h"

/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";

static const char* deviceId = "";
static const char* methodName = "[Method Name]";
static const char* methodPayload = "[Method Payload]";
static unsigned int timeout = 60;

int main(void)
{
    (void)platform_init();

    (void)printf("Calling IoTHubServiceClientAuth_CreateFromConnectionString with connectionString\n");
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
    }
    else
    {
        IOTHUB_SERVICE_CLIENT_DEVICE_METHOD_HANDLE serviceClientDeviceMethodHandle = IoTHubDeviceMethod_Create(iotHubServiceClientHandle);
        if (serviceClientDeviceMethodHandle == NULL)
        {
            (void)printf("IoTHubDeviceMethod_Create failed\n");
        }
        else
        {
            (void)printf("Invoking method %s on device...\n", methodName);

            int responseStatus;
            unsigned char* responsePayload;
            size_t responsePayloadSize;
            IOTHUB_DEVICE_METHOD_RESULT invokeResult = IoTHubDeviceMethod_Invoke(serviceClientDeviceMethodHandle, deviceId, methodName, methodPayload, timeout, &responseStatus, &responsePayload, &responsePayloadSize);
            if (invokeResult == IOTHUB_DEVICE_METHOD_OK)
            {
                printf("\r\nDevice Method called\r\n");
                printf("Device Method name:    %s\r\n", methodName);
                printf("Device Method payload: %s\r\n", methodPayload);

                printf("\r\nResponse status: %d\r\n", responseStatus);
                printf("Response payload: %.*s\r\n", (int)responsePayloadSize, (const char*)responsePayload);

                free(responsePayload);
            }
            else
            {
                (void)printf("IoTHubDeviceMethod_Invoke failed with result: %d\n", invokeResult);
            }

            (void)printf("Calling IoTHubDeviceMethod_Destroy...\n");
            IoTHubDeviceMethod_Destroy(serviceClientDeviceMethodHandle);
        }

        (void)printf("Calling IoTHubServiceClientAuth_Destroy...\n");
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    }
    platform_deinit();
}
