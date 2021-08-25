// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothub_registrymanager.h"
#include "iothubtransportamqp.h"
#include "iothub_account.h"
#include "iothubtest.h"
#include "parson.h"
#include "../common_longhaul/iothub_client_common_longhaul.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

const int FIVE_DAYS = (60 * 60 * 24) * 5;
const int YEAR_2020 = 1577865600;
static const char SAS_DEVICE_PREFIX_FMT[] = "csdk_e2eDevice_sas_j_please_delete_";
static const char X509_DEVICE_PREFIX_FMT[] = "csdk_e2eDevice_x509_j_please_delete_";

typedef struct ENUM_CONTEXT_TAG {
    IOTHUB_REGISTRYMANAGER_HANDLE registryManagerHandle;
    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE deviceTwinHandle;
} ENUM_CONTEXT;

void deviceItemEnum(const void* item, const void* action_context, bool* continue_processing)
{
    const ENUM_CONTEXT* context = (const ENUM_CONTEXT*) action_context;
    const IOTHUB_MODULE* device = (const IOTHUB_MODULE*) item;
    if (strncmp(device->deviceId, SAS_DEVICE_PREFIX_FMT, sizeof(SAS_DEVICE_PREFIX_FMT) - 1) == 0 || 
        strncmp(device->deviceId, X509_DEVICE_PREFIX_FMT, sizeof(X509_DEVICE_PREFIX_FMT) - 1) == 0)
    {
        char* twin = IoTHubDeviceTwin_GetTwin(context->deviceTwinHandle, device->deviceId);
        if (twin)
        {
            JSON_Object* root_object = NULL;
            JSON_Value* root_value = NULL;
            JSON_Object* properties = NULL;
            JSON_Object* desired = NULL;
            JSON_Object* metadata = NULL;
            const char* lastUpdated = NULL;
            if ((root_value = json_parse_string(twin)) == NULL)
            {
                LogError("json_parse_string failed");
            }
            else if ((root_object = json_value_get_object(root_value)) == NULL)
            {
                LogError("json_value_get_object root object failed");
            }
            else if ((properties = json_object_get_object(root_object, "properties")) == NULL)
            {
                LogError("json_object_get_object properties object failed");
            }
            else if ((desired = json_object_get_object(properties, "desired")) == NULL)
            {
                LogError("json_object_get_object desired object failed");
            }
            else if ((metadata = json_object_get_object(desired, "$metadata")) == NULL)
            {
                LogError("json_object_get_object metadata object failed");
            }
            else if ((lastUpdated = json_object_get_string(metadata, "$lastUpdated")) == NULL)
            {
                LogError("json_object_get_object lastUpdated time failed");
            }
            else
            {
                struct tm tm = { 0 };
                tm.tm_year = atoi(lastUpdated) - 1900;
                tm.tm_mon = atoi(&lastUpdated[5]) - 1;
                tm.tm_mday = atoi(&lastUpdated[8]);
                time_t deviceTime = mktime(&tm);
                time_t now = time(0);
                if (deviceTime > YEAR_2020 && (now - deviceTime > FIVE_DAYS))
                {
                    LogInfo("Deleting device %s, %d days old", device->deviceId, (int)(now - deviceTime) / (60 * 60 * 24));
                    IOTHUB_REGISTRYMANAGER_RESULT result = IoTHubRegistryManager_DeleteDevice(context->registryManagerHandle, device->deviceId);
                    if (result != IOTHUB_REGISTRYMANAGER_OK)
                    {
                        LogError("failed to delete device %s", device->deviceId);
                    }
                }
            }

            json_value_free(root_value);
        }
    }

    *continue_processing = true;
}


int main(void)
{
    int result = 0;
    const char* connection_string;
    IOTHUB_LONGHAUL_RESOURCES_HANDLE iotHubLonghaulRsrcsHandle = NULL;
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientAuth = NULL;
    IOTHUB_SERVICE_CLIENT_DEVICE_TWIN_HANDLE deviceTwin = NULL;
    IOTHUB_ACCOUNT_INFO_HANDLE account_info = NULL;
    IOTHUB_REGISTRYMANAGER_HANDLE registryManager = NULL;
    SINGLYLINKEDLIST_HANDLE deviceList = NULL;

    if ((iotHubLonghaulRsrcsHandle = longhaul_tests_init()) == NULL)
    {
        LogError("Test failed");
        result = MU_FAILURE;
    }
    else if ((account_info = longhaul_get_account_info(iotHubLonghaulRsrcsHandle)) == NULL)
    {
        LogError("Failed retrieving account info");
        result = MU_FAILURE;
    }
    else if ((connection_string = IoTHubAccount_GetIoTHubConnString(account_info)) == NULL)
    {
        LogError("Failed retrieving the IoT hub connection string");
        result = MU_FAILURE;
    }
    else if ((serviceClientAuth = IoTHubServiceClientAuth_CreateFromConnectionString(connection_string)) == NULL)
    {
        LogError("Failed to initialize IoT hub service client");
        result = MU_FAILURE;
    }
    else if ((deviceTwin = IoTHubDeviceTwin_Create(serviceClientAuth)) == NULL)
    {
        LogError("Failed to initialize IoT hub service device twin client");
        result = MU_FAILURE;
    }
    else if ((registryManager = IoTHubRegistryManager_Create(serviceClientAuth)) == NULL)
    {
        LogError("Failed to initialize IoT hub service registry manager");
        result = MU_FAILURE;
    }
    else if ((deviceList = singlylinkedlist_create()) == NULL)
    {
        LogError("Failed to create singly linked list");
        result = MU_FAILURE;
    }
    else if (IoTHubRegistryManager_GetModuleList(registryManager, NULL, deviceList, IOTHUB_REGISTRY_DEVICE_UPDATE_EX_VERSION_1) != IOTHUB_REGISTRYMANAGER_OK)
    {
        LogError("Failed to get hub devices");
        result = MU_FAILURE;
    }
    else
    {
        ENUM_CONTEXT context;
        context.deviceTwinHandle = deviceTwin;
        context.registryManagerHandle = registryManager;
        singlylinkedlist_foreach(deviceList, deviceItemEnum, &context);
    }

    if (deviceList)
    {
        singlylinkedlist_destroy(deviceList);
    }

    if (registryManager)
    {
        IoTHubRegistryManager_Destroy(registryManager);
    }

    if (deviceTwin)
    {
        IoTHubDeviceTwin_Destroy(deviceTwin);
    }

    if (serviceClientAuth)
    {
        IoTHubServiceClientAuth_Destroy(serviceClientAuth);
    }

    if (iotHubLonghaulRsrcsHandle)
    {
        longhaul_tests_deinit(iotHubLonghaulRsrcsHandle);
    }
   
    return result;
}
