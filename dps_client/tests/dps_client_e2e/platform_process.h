// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PLATFORM_PROCESS_H
#define PLATFORM_PROCESS_H

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct PLATFORM_PROCESS_INFO_TAG* PLATFORM_PROCESS_HANDLE;

    MOCKABLE_FUNCTION(, PLATFORM_PROCESS_HANDLE, platform_process_create, const char*, command_line);
    MOCKABLE_FUNCTION(, void, platform_process_destroy, PLATFORM_PROCESS_HANDLE, handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_PROCESS_H */
