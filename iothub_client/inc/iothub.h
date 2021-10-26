// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub.h
*    @brief Global initialization and deinitialization routines for all IoT Hub client operations.
*
*/

#ifndef IOTHUB_H
#define IOTHUB_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#else
#endif
    /**
    * @brief    IoTHubClient_Init Initializes the IoT Hub Client System.
    *
    * @remarks  
    *           This must be called before using any functionality from the IoT Hub
    *           client library, including the device provisioning client.
    *           
    *           @c IoTHubClient_Init should be called once per process, not per-thread.
    *
    * @return   int zero upon success, any other value upon failure.
    */
    MOCKABLE_FUNCTION(, int, IoTHub_Init);

    /**
    * @brief    IoTHubClient_Deinit Frees resources initialized in the IoTHubClient_Init function call.
    *
    * @remarks
    *           This should be called when using IoT Hub client library, including
    *           the device provisioning client.
    *
    *           This function should be called once per process, not per-thread.
    *
    * @warning
    *           Close all IoT Hub and provisioning client handles prior to invoking this.
    *
    */
    MOCKABLE_FUNCTION(, void, IoTHub_Deinit);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_H */
