// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/map.h"
#include "iothub_service_client_auth.h"

#include "iothub_deviceconfiguration.h"

static const char* connectionString = "[Hub connection string]";
static const char* configurationId = "[New configuration id]";
static const char* deviceId = "[Existing device id]";
static const char* targetCondition = "tags.UniqueTag='configurationapplyedgeagentreportinge2etestcita5b4e2b7f6464fe9988feea7d887584a' and tags.Environment='test'";
static const char* updatedTargetCondition = "tags.Environment='test'";

static const char* modulesContent = "{\"sunny\": {\"properties.desired\": {\"temperature\": 69,\"humidity\": 30}}, \
                                      \"goolily\": {\"properties.desired\": {\"elevation\": 45,\"orientation\": \"NE\"}}, \
                                      \"$edgeAgent\": {\"properties.desired\": {\"schemaVersion\": \"1.0\",\"runtime\": {\"type\": \"docker\",\"settings\": {\"minDockerVersion\": \"1.5\",\"loggingOptions\": \"\"}},\"systemModules\": \
                                                {\"edgeAgent\": {\"type\": \"docker\",\"settings\": {\"image\": \"edgeAgent\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}, \
                                                \"edgeHub\": {\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"always\",\"settings\": {\"image\": \"edgeHub\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}}, \
                                                    \"modules\": {\"sunny\": {\"version\": \"1.0\",\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"on-failure\",\"settings\": {\"image\": \"mongo\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}, \
                                                    \"goolily\": {\"version\": \"1.0\",\"type\": \"docker\",\"status\": \"running\",\"restartPolicy\": \"on-failure\",\"settings\": {\"image\": \"asa\",\"createOptions\": \"\"},\"configuration\": {\"id\": \"configurationapplyedgeagentreportinge2etestcit-config-a9ed4811-1b57-48bf-8af2-02319a38de01\"}}}}}, \
                                      \"$edgeHub\": {\"properties.desired\": {\"schemaVersion\": \"1.0\",\"routes\": {\"route1\": \"from * INTO $upstream\"},\"storeAndForwardConfiguration\": {\"timeToLiveSecs\": 20}}}}";

// Configurations can only have a non-blank device or modules content.
// Sample devicecontent is "{\"properties.desired.settings1\": {\"c\": 3, \"d\" : 4}, \"properties.desired.settings2\" : \"xyz\"}";
static const char* deviceContent = "";

static void printNameValuePairs_char(size_t count, const char** names, const char** values)
{
    size_t i;

    for (i = 0; i < count; i++)
    {
        (void)printf("\t\t%s:  %s\n", names[i], values[i]);
    }
}

static void printNameValuePairs_double(size_t count, const char** names, double* values)
{
    size_t i;

    for (i = 0; i < count; i++)
    {
        (void)printf("\t\t%s:  %f\n", names[i], values[i]);
    }
}

static void printDeviceInfo(const void* item, const void* action_context, bool* continue_processing)
{
    (void)action_context;
    IOTHUB_DEVICE_CONFIGURATION* configuration = (IOTHUB_DEVICE_CONFIGURATION *)item;

    if (configuration != NULL)
    {
        (void)printf("Configuration\n");

        (void)printf("    configurationId        : %s\n", configuration->configurationId);
        (void)printf("    targetCondition        : %s\n", configuration->targetCondition);
        (void)printf("    createdTime            : %s\n", configuration->createdTimeUtc);
        (void)printf("    lastUpdatedTime        : %s\n", configuration->lastUpdatedTimeUtc);
        (void)printf("    eTag                   : %s\n", configuration->eTag);
        (void)printf("    priority               : %d\n", configuration->priority);

        (void)printf("    Labels        :\n");
        printNameValuePairs_char(configuration->labels.numLabels, configuration->labels.labelNames, configuration->labels.labelValues);

        (void)printf("    Configuration content        :\n");
        if (configuration->content.deviceContent != NULL) (void)printf("    \tDevice content        : %s\n", configuration->content.deviceContent);
        if (configuration->content.modulesContent != NULL) (void)printf("    \tModules content        :%s\n", configuration->content.modulesContent);

        (void)printf("    Configuration system metrics        :\n");
        (void)printf("    \tDefinitions        :\n");
        printNameValuePairs_char(configuration->systemMetricsDefinition.numQueries, configuration->systemMetricsDefinition.queryNames, (const char **)configuration->systemMetricsDefinition.queryStrings);
        (void)printf("    \tResults        :\n");
        printNameValuePairs_double(configuration->systemMetricsResult.numQueries, configuration->systemMetricsResult.queryNames, (double *)configuration->systemMetricsResult.results);

        (void)printf("    Configuration custom metrics        :\n");
        (void)printf("    \tDefinitions        :\n");
        printNameValuePairs_char(configuration->metricsDefinition.numQueries, configuration->metricsDefinition.queryNames, (const char **)configuration->metricsDefinition.queryStrings);
        (void)printf("    \tResults        :\n");
        printNameValuePairs_double(configuration->metricResult.numQueries, configuration->metricResult.queryNames, (double *)configuration->metricResult.results);
    }

    *continue_processing = true;
}

int main(void)
{
    (void)platform_init();

    IOTHUB_DEVICE_CONFIGURATION_RESULT result;
    IOTHUB_DEVICE_CONFIGURATION deviceConfigurationInfo;

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
    }
    else
    {
        // Add configuration
        IOTHUB_DEVICE_CONFIGURATION_ADD deviceConfigurationAddInfo;
        IOTHUB_DEVICE_CONFIGURATION_CONTENT deviceConfigurationAddInfoContent;
        IOTHUB_DEVICE_CONFIGURATION_LABELS deviceConfigurationAddInfoLabels;
        MAP_HANDLE labels = Map_Create(NULL);

        IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE iotHubDeviceConfigurationHandle = IoTHubDeviceConfiguration_Create(iotHubServiceClientHandle);

        (void)memset(&deviceConfigurationAddInfo, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_ADD));
        (void)memset(&deviceConfigurationAddInfoContent, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_CONTENT));
        (void)memset(&deviceConfigurationAddInfoLabels, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION_LABELS));

        mallocAndStrcpy_s((char **)&deviceConfigurationAddInfoContent.deviceContent, deviceContent);
        mallocAndStrcpy_s((char **)&deviceConfigurationAddInfoContent.modulesContent, modulesContent);

        Map_Add(labels, "label1", "value1");
        Map_GetInternals(labels, (const char* const **)&deviceConfigurationAddInfoLabels.labelNames, (const char* const **)&deviceConfigurationAddInfoLabels.labelValues, &deviceConfigurationAddInfoLabels.numLabels);

        mallocAndStrcpy_s((char **)&deviceConfigurationAddInfo.configurationId, configurationId);
        mallocAndStrcpy_s((char **)&deviceConfigurationAddInfo.targetCondition, targetCondition);
        deviceConfigurationAddInfo.content = deviceConfigurationAddInfoContent;
        deviceConfigurationAddInfo.labels = deviceConfigurationAddInfoLabels;
        deviceConfigurationAddInfo.priority = 10;
        deviceConfigurationAddInfo.version = IOTHUB_DEVICE_CONFIGURATION_ADD_VERSION_1;

        (void)memset(&deviceConfigurationInfo, 0, sizeof(IOTHUB_DEVICE_CONFIGURATION));

        if ((result = IoTHubDeviceConfiguration_AddConfiguration(iotHubDeviceConfigurationHandle, &deviceConfigurationAddInfo, &deviceConfigurationInfo)) != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            (void)printf("IoTHubDeviceConfiguration_AddConfiguration failed. Result = %d\r\n", result);
        }

        Map_Destroy(labels);

        // Get configuration
        if ((result = IoTHubDeviceConfiguration_GetConfiguration(iotHubDeviceConfigurationHandle, deviceConfigurationAddInfo.configurationId, &deviceConfigurationInfo)) != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            (void)printf("IoTHubDeviceConfiguration_GetConfiguration failed. Result = %d\r\n", result);
        }

        // Update configuration
        mallocAndStrcpy_s((char **)&deviceConfigurationInfo.configurationId, configurationId);
        mallocAndStrcpy_s((char **)&deviceConfigurationInfo.targetCondition, updatedTargetCondition);
        deviceConfigurationInfo.version = IOTHUB_DEVICE_CONFIGURATION_VERSION_1;

        if ((result = IoTHubDeviceConfiguration_UpdateConfiguration(iotHubDeviceConfigurationHandle, &deviceConfigurationInfo)) != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            (void)printf("IoTHubDeviceConfiguration_UpdateConfiguration failed. Result = %d\r\n", result);
        }

        SINGLYLINKEDLIST_HANDLE temp_list;

        if ((temp_list = singlylinkedlist_create()) == NULL)
        {
            (void)printf("singlylinkedlist_create failed. skip IoTHubDeviceConfiguration_GetConfigurations\r\n");
        }
        else
        {
            if ((result = IoTHubDeviceConfiguration_GetConfigurations(iotHubDeviceConfigurationHandle, 20, temp_list)) != IOTHUB_DEVICE_CONFIGURATION_OK)
            {
                (void)printf("IoTHubDeviceConfiguration_GetConfigurations failed. Result = %d\r\n", result);
            }

            singlylinkedlist_foreach(temp_list, printDeviceInfo, NULL);
        }

        if ((result = IoTHubDeviceConfiguration_ApplyConfigurationContentToDeviceOrModule(iotHubDeviceConfigurationHandle, deviceId, &(deviceConfigurationAddInfo.content))) != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            (void)printf("IoTHubDeviceConfiguration_ApplyConfigurationContentToDeviceOrModule failed. Result = %d\r\n", result);
        }

        if ((result = IoTHubDeviceConfiguration_DeleteConfiguration(iotHubDeviceConfigurationHandle, deviceConfigurationAddInfo.configurationId)) != IOTHUB_DEVICE_CONFIGURATION_OK)
        {
            (void)printf("IoTHubDeviceConfiguration_DeleteConfiguration failed. Result = %d\r\n", result);
        }

        singlylinkedlist_destroy(temp_list);
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        IoTHubDeviceConfiguration_FreeConfigurationMembers(&deviceConfigurationInfo);
    }


    platform_deinit();
}
