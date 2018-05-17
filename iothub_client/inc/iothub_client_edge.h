// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_ll_edge.h
*	@brief	 APIs that allow a user to communicate to an Edge Device.
*
*/

#ifndef IOTHUB_CLIENT_LL_EDGE_H
#define IOTHUB_CLIENT_LL_EDGE_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "iothub_module_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
    * @brief	This API creates a module handle based on environment variables set in the Edge runtime.
                NOTE: It is *ONLY* valid when the code is running in a container initiated by Edge.
    *
    * @param	protocol            Function pointer for protocol implementation
    *
    * @return	A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.

    */
    MOCKABLE_FUNCTION(, IOTHUB_MODULE_CLIENT_HANDLE, IoTHubModuleClient_CreateFromEnvironment, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_LL_EDGE_H */
