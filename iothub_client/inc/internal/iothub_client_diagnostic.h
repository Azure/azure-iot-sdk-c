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
#define IOTHUB_DISTRIBUTED_TRACING_SETTING_VERSION_1 1
typedef struct IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA_TAG
{
    int version;

    /* Distributed Tracing sampling mode - Off, On, Inherit */
    bool samplingMode;

    /* Distributed tracing fixed-rate sampling percentage, set to 100/N where N is an integer 
    E.g. 50 (=100/2), 33.33 (=100/3), 25 (=100/4), 20, 1 (=100/100), 0.1 (=100/1000) */
    uint8_t samplingRate;

    /* Distributed tracing message sequence number */
    uint32_t currentMessageNumber;
} IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA;

/**
* @brief    Adds diagnostic information to message if:
** DEPRECATED: Use IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary instead. **
*           a. diagSetting->diagSamplingPercentage > 0 and
*           b. the number of current message matches sample rule specified by diagSetting->diagSamplingPercentage
*
* @param    diagSetting        Pointer to an @c IOTHUB_DIAGNOSTIC_SETTING_DATA structure
* @param    messageHandle    message handle
* @return   0 upon success, non-zero otherwise
*/
MOCKABLE_FUNCTION(, int, IoTHubClient_Diagnostic_AddIfNecessary, IOTHUB_DIAGNOSTIC_SETTING_DATA*, diagSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

/**
* @brief    Adds tracestate information to message if:
*           a. distributedTracingSetting->sampingMode = true
*           b. distributedTracingSetting->samplingRate > 0 and
*           c. the number of current message matches sample rule specified by distributedTracingSetting->samplingRate
*
* @param    distributedTracingSetting        Pointer to an @c IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA structure
* @param    messageHandle    message handle
* @return   0 upon success, non-zero otherwise
*/
MOCKABLE_FUNCTION(, int, IoTHubClient_DistributedTracing_AddToMessageHeadersIfNecessary, IOTHUB_DISTRIBUTED_TRACING_SETTING_DATA*, distributedTracingSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

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
