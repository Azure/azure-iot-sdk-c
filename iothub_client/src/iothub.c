// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h> 

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "iothub.h"

int IoTHub_Init()
{
    int result;
    if (platform_init() != 0)
    {
        LogError("Platform initialization failed");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

void IoTHub_Deinit()
{
    platform_deinit();
}
