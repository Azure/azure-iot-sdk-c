// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_module_client.h"
#include "iothub_client_core.h"

IOTHUB_MODULE_CLIENT_HANDLE IoTHubModuleClient_CreateFromEnvironment(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    return IoTHubClientCore_CreateFromEnvironment(protocol);
}

