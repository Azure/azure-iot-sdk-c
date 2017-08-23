// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file   iothub_client_diagnostic.h
*	@brief  The @c diagnostic component.
*/

#ifndef IOTHUB_CLIENT_DIAGNOSTIC_H
#define IOTHUB_CLIENT_DIAGNOSTIC_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "iothub_message.h"
#include <stdint.h>

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

/** @brief diagnostic related setting */
typedef struct IOTHUB_DIAGNOSTIC_SETTING_DATA_TAG
{
    uint32_t diagSamplingPercentage;
    uint32_t currentMessageNumber;
} IOTHUB_DIAGNOSTIC_SETTING_DATA;

/**
    * @brief	Add diagnostic information to message if meeting diagnostic rule
    *
    * @param	diagSetting		Pointer to an @c IOTHUB_DIAGNOSTIC_SETTING_DATA structure
    *
    * @param	messageHandle	message handle 
    *
    * @return	true upon success or false if any error
    */
MOCKABLE_FUNCTION(, bool, IoTHubClient_Diagnostic_AddIfNecessary, IOTHUB_DIAGNOSTIC_SETTING_DATA *, diagSetting, IOTHUB_MESSAGE_HANDLE, messageHandle);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_DIAGNOSTIC_H */
