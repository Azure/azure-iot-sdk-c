// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file   iothub_client_diagnostic.h
*    @brief  The @c diagnostic is a component that helps to add predefined diagnostic
            properties to message for end to end diagnostic purpose
*/

#ifndef IOTHUB_CLIENT_DIAGNOSTIC_H
#define IOTHUB_CLIENT_DIAGNOSTIC_H

#include "azure_c_shared_utility/umock_c_prod.h"

#include "iothub_message.h"
#include <stdint.h>

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

// Deprecated
/** @brief diagnostic related setting */
typedef struct IOTHUB_DIAGNOSTIC_SETTING_DATA_TAG
{
    uint32_t diagSamplingPercentage;
    uint32_t currentMessageNumber;
} IOTHUB_DIAGNOSTIC_SETTING_DATA;

/** @brief distributed tracing settings */
typedef struct IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA_TAG
{
    bool     samplingMode;
    uint32_t samplingRate;
    uint32_t currentMessageNumber;
} IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA;

/**
    * @brief    Adds diagnostic information to message if:
    *           a. diagSetting->diagSamplingPercentage > 0 and
    *           b. the number of current message matches sample rule specified by diagSetting->diagSamplingPercentage
    * **** NOTE: This API is deprecated **** 
    *
    * @param    diagSetting        Pointer to an @c IOTHUB_DIAGNOSTIC_SETTING_DATA structure
    * @param    messageHandle    message handle
    * @return    0 upon success, non-zero otherwise
    */
MOCKABLE_FUNCTION(, int, IoTHubClient_Diagnostic_AddIfNecessary, IOTHUB_DIAGNOSTIC_SETTING_DATA *, diagSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

/**
    * @brief    Adds tracestate information to message if:
    *           a. distributedTracingSetting->sampingMode = true
    *           b. distributedTracingSetting->samplingRate > 0 and
    *           c. the number of current message matches sample rule specified by distributedTracingSetting->samplingRate
    *
    * @param    distributedTracingSetting        Pointer to an @c IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA structure
    * @param    messageHandle    message handle
    * @return    0 upon success, non-zero otherwise
    */
MOCKABLE_FUNCTION(, int, IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary, IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA *, distributedTracingSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

/**
    * @brief	Update distributed tracing settings from device twin
    *
    * @param	distributedTracingSetting		        Pointer to an @c IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA structure
    * @param    isPartialUpdate         Whether device twin is complete or partial update
    * @param	payLoad			        Received device twin
    * @param	reportedStatePayload	Reported state payload for distributed tracing setting
    * @return	0 upon success, non-zero otherwise
    */
MOCKABLE_FUNCTION(, int, IoTHubClient_DistributedTracing_UpdateFromTwin, IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA*, distributedTracingSetting, bool, isPartialUpdate, const unsigned char*, payLoad, STRING_HANDLE, reportedStatePayload);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_DIAGNOSTIC_H */
