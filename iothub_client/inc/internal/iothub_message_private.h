// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file   iothub_message_private.h
*    @brief  For internal use of the Azure IoT C SDK only.
*/

#ifndef IOTHUB_MESSAGE_PRIVATE_H
#define IOTHUB_MESSAGE_PRIVATE_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"
#include "iothub_message.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

struct MESSAGE_DISPOSITION_CONTEXT_TAG;
typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG* MESSAGE_DISPOSITION_CONTEXT_HANDLE;

typedef void(*MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION)(MESSAGE_DISPOSITION_CONTEXT_HANDLE dispositionContext);

/**
* @brief   Sets the context for the transport layer to send a DISPOSITION or ACK for a cloud-to-device message.
*
* @param   iotHubMessageHandle                The message on which to have the context set.
* @param   dispositionContext                 The transport context for disposition.
* @param   dispositionContextDestroyFunction  A function defined by the transport for destroying the context instance.
*
* @return  An #IOTHUB_MESSAGE_RESULT with the result of the operation.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_SetDispositionContext, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, MESSAGE_DISPOSITION_CONTEXT_HANDLE, dispositionContext, MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION, dispositionContextDestroyFunction);

/**
* @brief   Gets the context for the transport layer to send a DISPOSITION or ACK for a cloud-to-device message.
*
* @param   iotHubMessageHandle                The message to have the context set on.
* @param   dispositionContext                 Variable to hold the transport context for disposition.
*
* @return  An #IOTHUB_MESSAGE_RESULT with the result of the operation.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_GetDispositionContext, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, MESSAGE_DISPOSITION_CONTEXT_HANDLE*, dispositionContext);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_MESSAGE_PRIVATE_H */
