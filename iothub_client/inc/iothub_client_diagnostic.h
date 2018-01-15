// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file   iothub_client_diagnostic.h
*	@brief  The @c diagnostic is a component that helps to add predefined diagnostic 
            properties to message for end to end diagnostic purpose
*/

#ifndef IOTHUB_CLIENT_DIAGNOSTIC_H
#define IOTHUB_CLIENT_DIAGNOSTIC_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "iothub_message.h"
#include "parson.h"
#include <stdint.h>

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

#define DEVICE_TWIN_SAMPLING_RATE_KEY "__e2e_diag_sample_rate"

#define E2E_DIAG_SETTING_METHOD_VALUE  \
    E2E_DIAG_SETTING_UNKNOWN,          \
    E2E_DIAG_SETTING_USE_LOCAL,        \
    E2E_DIAG_SETTING_USE_REMOTE

DEFINE_ENUM(E2E_DIAG_SETTING_METHOD, E2E_DIAG_SETTING_METHOD_VALUE);

/** @brief diagnostic related setting */
typedef struct IOTHUB_DIAGNOSTIC_SETTING_DATA_TAG
{
    uint32_t diagSamplingPercentage;
    uint32_t currentMessageNumber;
    E2E_DIAG_SETTING_METHOD diagSettingMethod;
} IOTHUB_DIAGNOSTIC_SETTING_DATA;

/**
    * @brief	Adds diagnostic information to message if: 
    *           a. diagSetting->diagSamplingPercentage > 0 and
    *           b. the number of current message matches sample rule specified by diagSetting->diagSamplingPercentage
    *
    * @param	diagSetting		Pointer to an @c IOTHUB_DIAGNOSTIC_SETTING_DATA structure
    *
    * @param	messageHandle	message handle 
    *
    * @return	0 upon success
    */
MOCKABLE_FUNCTION(, int, IoTHubClient_Diagnostic_AddIfNecessary, IOTHUB_DIAGNOSTIC_SETTING_DATA *, diagSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

/**
    * @brief	Update diagnostic settings from device twin
    *
    * @param	diagSetting		Pointer to an @c IOTHUB_DIAGNOSTIC_SETTING_DATA structure
    *
    * @param    isPartialUpdate Whether device twin is complete or partial update
    *
    * @param	payLoad			Received device twin
    *
    * @param	message			Record some messages when updating diagnostic settings
    *
    * @return	0 upon success
    */
MOCKABLE_FUNCTION(, int, IoTHubClient_Diagnostic_UpdateFromTwin, IOTHUB_DIAGNOSTIC_SETTING_DATA*, diagSetting, bool, isPartialUpdate, const unsigned char*, payLoad, STRING_HANDLE, message);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_DIAGNOSTIC_H */
