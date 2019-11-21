// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file is under development and it is subject to change

#ifndef IOTHUB_DEVICECONFIGURATION_H
#define IOTHUB_DEVICECONFIGURATION_H

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include <time.h>
#include "iothub_service_client_auth.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum IOTHUB_DEVICE_CONFIGURATION_RESULT_TAG
{
    IOTHUB_DEVICE_CONFIGURATION_OK = 0,
    IOTHUB_DEVICE_CONFIGURATION_INVALID_ARG = 1,
    IOTHUB_DEVICE_CONFIGURATION_ERROR = 2,
    IOTHUB_DEVICE_CONFIGURATION_HTTPAPI_ERROR = 3,
    IOTHUB_DEVICE_CONFIGURATION_JSON_ERROR = 4,
    IOTHUB_DEVICE_CONFIGURATION_OUT_OF_MEMORY_ERROR = 5,
    IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_NOT_EXIST = 6,
    IOTHUB_DEVICE_CONFIGURATION_CONFIGURATION_EXIST = 7
} IOTHUB_DEVICE_CONFIGURATION_RESULT;

const char * IOTHUB_DEVICE_CONFIGURATION_RESULTStrings(IOTHUB_DEVICE_CONFIGURATION_RESULT value);

typedef enum IOTHUB_DEVICECONFIGURATION_REQUEST_MODE_TAG
{
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET_LIST = 0,
    IOTHUB_DEVICECONFIGURATION_REQUEST_GET = 1,
    IOTHUB_DEVICECONFIGURATION_REQUEST_ADD = 2,
    IOTHUB_DEVICECONFIGURATION_REQUEST_UPDATE = 3,
    IOTHUB_DEVICECONFIGURATION_REQUEST_DELETE = 4,
    IOTHUB_DEVICECONFIGURATION_REQUEST_APPLY_CONFIGURATION_CONTENT = 5
} IOTHUB_DEVICECONFIGURATION_REQUEST_MODE;

typedef struct IOTHUB_DEVICE_CONFIGURATION_CONTENT_TAG
{
    const char* deviceContent;
    const char* modulesContent;
} IOTHUB_DEVICE_CONFIGURATION_CONTENT;

typedef struct IOTHUB_DEVICE_CONFIGURATION_METRICS_RESULTS_TAG
{
    size_t numQueries;
    const char** queryNames;
    double* results;
} IOTHUB_DEVICE_CONFIGURATION_METRICS_RESULT;

typedef struct IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION_TAG
{
    size_t numQueries;
    const char** queryNames;
    const char** queryStrings;
} IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION;

typedef struct IOTHUB_DEVICE_CONFIGURATION_LABEL_TAG
{
    size_t numLabels;
    const char** labelNames;
    const char** labelValues;
} IOTHUB_DEVICE_CONFIGURATION_LABELS;

#define IOTHUB_DEVICE_CONFIGURATION_SCHEMA_VERSION_1 "1.0"
#define IOTHUB_DEVICE_CONFIGURATION_VERSION_1 1
typedef struct IOTHUB_DEVICE_CONFIGURATION_TAG
{
    int version;
    const char* schemaVersion;                                                    //version 1+
    const char* configurationId;                                                  //version 1+
    const char* targetCondition;                                                  //version 1+
    const char* eTag;                                                             //version 1+
    const char* createdTimeUtc;                                                   //version 1+
    const char* lastUpdatedTimeUtc;                                               //version 1+
    int priority;                                                                 //version 1+

    IOTHUB_DEVICE_CONFIGURATION_CONTENT content;                                  //version 1+
    IOTHUB_DEVICE_CONFIGURATION_LABELS labels;                                    //version 1+

    IOTHUB_DEVICE_CONFIGURATION_METRICS_RESULT systemMetricsResult;               //version 1+
    IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION systemMetricsDefinition;       //version 1+

    IOTHUB_DEVICE_CONFIGURATION_METRICS_RESULT metricResult;                      //version 1+
    IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION metricsDefinition;             //version 1+
} IOTHUB_DEVICE_CONFIGURATION;

#define IOTHUB_DEVICE_CONFIGURATION_ADD_VERSION_1 1
typedef struct IOTHUB_DEVICE_CONFIGURATION_ADD_TAG
{
    int version;
    const char* configurationId;                                    //version 1+
    const char* targetCondition;                                    //version 1+
    int priority;                                                   //version 1+

    IOTHUB_DEVICE_CONFIGURATION_CONTENT content;                    //version 1+
    IOTHUB_DEVICE_CONFIGURATION_LABELS labels;                      //version 1+
    IOTHUB_DEVICE_CONFIGURATION_METRICS_DEFINITION metrics;         //version 1+
} IOTHUB_DEVICE_CONFIGURATION_ADD;

/** @brief Handle to hide struct and use it in consequent APIs
*/
typedef struct IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_TAG* IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE;


/** @brief  Creates a IoT Hub Service Client DeviceConfiguration handle for use it in consequent APIs.
*
* @param    serviceClientHandle    Service client handle.
*
* @return   A non-NULL @c IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE value that is used when
*           invoking other functions for IoT Hub DeviceConfiguration and @c NULL on failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, IoTHubDeviceConfiguration_Create, IOTHUB_SERVICE_CLIENT_AUTH_HANDLE, serviceClientHandle);

/** @brief  Disposes of resources allocated by the IoT Hub IoTHubDeviceConfiguration_Create.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
*/
MOCKABLE_FUNCTION(, void, IoTHubDeviceConfiguration_Destroy, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle);

/** @brief  Retrieves the Configuration info for multiple configurations from IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    maxConfigurationsCount                    Maximum number of configurations requested
* @param    configurations                            Output parameter, if it is not NULL will contain the requested configurations
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_GetConfigurations, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, size_t, maxConfigurationsCount, SINGLYLINKEDLIST_HANDLE, configurationsList);

/** @brief  Retrieves the Configuration info for specified configurationId from IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    configurationId                         The configuration name (id) to retrieve Configuration info for.
* @param    configuration                           Output parameter, if it is not NULL will contain the requested configuration info structure
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_GetConfiguration, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, const char*, configurationId, IOTHUB_DEVICE_CONFIGURATION*, configuration);

/** @brief  Adds the Configuration info to IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    configurationAdd     IOTHUB_DEVICE_CONFIGURATION_ADD structure containing
*                                   the new configuration Id and other optional parameters
* @param    configuration           Output parameter, if it is not NULL will contain the created configuration info structure
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_AddConfiguration, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, const IOTHUB_DEVICE_CONFIGURATION_ADD*, configurationAdd, IOTHUB_DEVICE_CONFIGURATION*, configuration);

/** @brief  Updates the given Configuration in IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    configuration           IOTHUB_DEVICE_CONFIGURATION structure containing the new configuration info.
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_UpdateConfiguration, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, const IOTHUB_DEVICE_CONFIGURATION*, configuration);

/** @brief  Deletes the given Configuration from IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    configurationId         The configuration name (id) to delete Configuration info for.
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_DeleteConfiguration, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, const char*, configurationId);

/** @brief  Deletes the given Configuration from IoT Hub.
*
* @param    serviceClientDeviceConfigurationHandle    The handle created by a call to the create function.
* @param    deviceOrModuleId                     The target device or module id for the Configuration content.
* @param    configurationContent                 The configuration content to be applied.
*
* @return   IOTHUB_DEVICE_CONFIGURATION_RESULT upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CONFIGURATION_RESULT, IoTHubDeviceConfiguration_ApplyConfigurationContentToDeviceOrModule, IOTHUB_SERVICE_CLIENT_DEVICE_CONFIGURATION_HANDLE, serviceClientDeviceConfigurationHandle, const char*, deviceOrModuleId, const IOTHUB_DEVICE_CONFIGURATION_CONTENT*, configurationContent);


/**
* @brief    Free members of the IOTHUB_DEVICE_CONFIGURATION structure (NOT the structure itself)
*
* @param    configuration      The structure to have its members freed.
*/
extern void IoTHubDeviceConfiguration_FreeConfigurationMembers(IOTHUB_DEVICE_CONFIGURATION* configuration);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_DEVICECONFIGURATION_H
