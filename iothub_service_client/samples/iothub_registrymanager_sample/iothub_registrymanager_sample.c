// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"

#include "iothub_service_client_auth.h"
#include "iothub_registrymanager.h"

/* Paste in the your iothub connection string  */
/* Paste in the your iothub connection string  */
static const char* connectionString = "[device connection string]";
static const char* deviceId = "";

static void printDeviceInfo(const IOTHUB_DEVICE_EX* device, int orderNum)
{
    if ((device != NULL) && (device->deviceId != NULL))
    {
        if (orderNum >= 0)
        {
            (void)printf("Device(%d)\n", orderNum);
        }
        else
        {
            (void)printf("Device\n");
        }
        (void)printf("    deviceId                    : %s\n", device->deviceId);
        (void)printf("    primaryKey                  : %s\n", device->primaryKey);
        (void)printf("    secondaryKey                : %s\n", device->secondaryKey);
        (void)printf("    generationId                : %s\n", device->generationId);
        (void)printf("    eTag                        : %s\n", device->eTag);
        (void)printf("    authMethod                  : %d\n", device->authMethod);

        if (device->connectionState == IOTHUB_DEVICE_CONNECTION_STATE_CONNECTED)
        {
            (void)printf("    connectionState             : Connected\n");
        }
        else
        {
            (void)printf("    connectionState             : Disconnected\n");
        }
        (void)printf("    connectionStateUpdatedTime  : %s\n", device->eTag);
        if (device->status == IOTHUB_DEVICE_STATUS_ENABLED)
        {
            (void)printf("    status                      : Enabled\n");
        }
        else
        {
            (void)printf("    status                      : Disabled\n");
        }
        (void)printf("    statusReason                : %s\n", device->statusReason);
        (void)printf("    statusUpdatedTime           : %s\n", device->statusUpdatedTime);
        (void)printf("    lastActivityTime            : %s\n", device->lastActivityTime);
        (void)printf("    cloudToDeviceMessageCount   : %lu\n", (unsigned long)device->cloudToDeviceMessageCount);
    }
}

static void free_device_resource(IOTHUB_DEVICE_EX* device)
{
    free((char*)device->deviceId);
    free((char*)device->primaryKey);
    free((char*)device->secondaryKey);
    free((char*)device->generationId);
    free((char*)device->eTag);
    free((char*)device->connectionStateUpdatedTime);
    free((char*)device->statusReason);
    free((char*)device->statusUpdatedTime);
    free((char*)device->lastActivityTime);
    free((char*)device->configuration);
    free((char*)device->deviceProperties);
    free((char*)device->serviceProperties);
}

int main(void)
{
    (void)platform_init();

    IOTHUB_REGISTRYMANAGER_RESULT result;

    (void)printf("Calling IoTHubServiceClientAuth_CreateFromConnectionString with the connection string\n");
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
    }
    else
    {
        IOTHUB_REGISTRY_DEVICE_CREATE_EX deviceCreateInfo;
        (void)printf("Creating device with RegistryManager...\n");
        IOTHUB_REGISTRYMANAGER_HANDLE iotHubRegistryManagerHandle = IoTHubRegistryManager_Create(iotHubServiceClientHandle);

        (void)memset(&deviceCreateInfo, 0, sizeof(IOTHUB_REGISTRY_DEVICE_CREATE_EX));
        deviceCreateInfo.deviceId = deviceId;
        deviceCreateInfo.primaryKey = "";
        deviceCreateInfo.secondaryKey = "";
        deviceCreateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
        deviceCreateInfo.version = IOTHUB_REGISTRY_DEVICE_CREATE_EX_VERSION_1;

        //TODO: Switch this to IOTHUB_DEVICE_EX
        IOTHUB_DEVICE_EX deviceInfo;
        deviceInfo.version = IOTHUB_REGISTRY_DEVICE_CREATE_EX_VERSION_1;

        // Create device
        result = IoTHubRegistryManager_CreateDevice_Ex(iotHubRegistryManagerHandle, &deviceCreateInfo, &deviceInfo);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice: Device has been created successfully: deviceId=%s\n", deviceInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_DEVICE_EXIST)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice: Device already exists\n");
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice failed\n");
        }
        // You will need to Free the returned device information after it was created
        free_device_resource(&deviceInfo);

        // Update device
        IOTHUB_REGISTRY_DEVICE_UPDATE deviceUpdateInfo;
        (void)memset(&deviceUpdateInfo, 0, sizeof(IOTHUB_REGISTRY_DEVICE_UPDATE));

        deviceUpdateInfo.deviceId = deviceId;
        deviceUpdateInfo.primaryKey = "aaabbbcccdddeeefffggghhhiiijjjkkklllmmmnnnoo";
        deviceUpdateInfo.secondaryKey = "111222333444555666777888999000aaabbbcccdddee";
        deviceUpdateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
        deviceUpdateInfo.status = IOTHUB_DEVICE_STATUS_DISABLED;
        result = IoTHubRegistryManager_UpdateDevice(iotHubRegistryManagerHandle, &deviceUpdateInfo);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_UpdateDevice: Device has been updated successfully: deviceId=%s\n", deviceUpdateInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_UpdateDevice failed\n");
        }

        // Get device
        (void)memset(&deviceInfo, 0, sizeof(IOTHUB_DEVICE_EX));
        deviceInfo.version = IOTHUB_REGISTRY_DEVICE_CREATE_EX_VERSION_1;

        result = IoTHubRegistryManager_GetDevice_Ex(iotHubRegistryManagerHandle, deviceCreateInfo.deviceId, &deviceInfo);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_GetDevice: Successfully got device info: deviceId=%s\n", deviceInfo.deviceId);
            printDeviceInfo(&deviceInfo, -1);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_GetDevice failed\n");
        }
        // You will need to free the device information getting the device
        free_device_resource(&deviceInfo);

        // Delete device
        result = IoTHubRegistryManager_DeleteDevice(iotHubRegistryManagerHandle, deviceCreateInfo.deviceId);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_DeleteDevice: Device has been deleted successfully: deviceId=%s\n", deviceCreateInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_DeleteDevice: Delete device failed\n");
        }

        // Get statistics
        IOTHUB_REGISTRY_STATISTICS registryStatistics;
        result = IoTHubRegistryManager_GetStatistics(iotHubRegistryManagerHandle, &registryStatistics);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_GetStatistics: Successfully got registry statistics\n");
            (void)printf("Total device count: %lu\n", (unsigned long)registryStatistics.totalDeviceCount);
            (void)printf("Enabled device count: %lu\n", (unsigned long)registryStatistics.enabledDeviceCount);
            (void)printf("Disabled device count: %lu\n", (unsigned long)registryStatistics.disabledDeviceCount);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_GetStatistics failed\n");
        }

        (void)printf("Calling IoTHubRegistryManager_Destroy...\n");
        IoTHubRegistryManager_Destroy(iotHubRegistryManagerHandle);

        (void)printf("Calling IoTHubServiceClientAuth_Destroy...\n");
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
    }
    platform_deinit();
}
