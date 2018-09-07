// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/platform.h"

#include "iothub_service_client_auth.h"
#include "iothub_devicetwin.h"

/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";
static const char* deviceId = "[Device Id]";

int main(void)
{
    //IOTHUB_TWIN_REQUEST_GET               GET      {iot hub}/twins/{device id}                     // Get device twin
    //IOTHUB_TWIN_REQUEST_UPDATE            PATCH    {iot hub}/twins/{device id}                     // Partally update device twin
    //IOTHUB_TWIN_REQUEST_REPLACE_TAGS      PUT      {iot hub}/twins/{device id}/tags                // Replace update tags
    //IOTHUB_TWIN_REQUEST_REPLACE_DESIRED   PUT      {iot hub}/twins/{device id}/properties/desired  // Replace update desired properties
    //IOTHUB_TWIN_REQUEST_UPDATE_DESIRED    PATCH    {iot hub}/twins/{device id}/properties/desired  // Partially update desired properties

    (void)platform_init();

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\r\n");
    }
    else
    {
        IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE serviceClientDeviceTwinHandle = IoTHubDeviceTwin_Create(iotHubServiceClientHandle);
        if (serviceClientDeviceTwinHandle == NULL)
        {
            (void)printf("IoTHubDeviceTwin_Create failed\r\n");
        }
        else
        {
            (void)printf("Retrieving DeviceTwin...\r\n");

            char* deviceTwinJson;

            if ((deviceTwinJson = IoTHubDeviceTwin_GetTwin(serviceClientDeviceTwinHandle, deviceId)) == NULL)
            {
                (void)printf("IoTHubDeviceTwin_GetTwin failed\r\n");
            }
            else
            {
                (void)printf("\r\nDeviceTwin:\r\n");
                printf("%s\r\n", deviceTwinJson);

                const char* updateJson = "{\"properties\":{\"desired\":{\"telemetryInterval\":30}}}";
                char* updatedDeviceTwinJson;

                if ((updatedDeviceTwinJson = IoTHubDeviceTwin_UpdateTwin(serviceClientDeviceTwinHandle, deviceId, updateJson)) == NULL)
                {
                    (void)printf("IoTHubDeviceTwin_UpdateTwin failed\r\n");
                }
                else
                {
                    (void)printf("\r\nDeviceTwin has been successfully updated (partial update):\r\n");
                    printf("%s\r\n", updatedDeviceTwinJson);

                    free(updatedDeviceTwinJson);
                }

                // Free the device Twin json string
                free(deviceTwinJson);
            }
            IoTHubDeviceTwin_Destroy(serviceClientDeviceTwinHandle);
        }
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    }
    platform_deinit();
}
