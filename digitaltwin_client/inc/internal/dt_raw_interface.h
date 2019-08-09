// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DIGITALTWIN_RAW_INTERFACE_H
#define DIGITALTWIN_RAW_INTERFACE_H

#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, const char*, DT_Get_RawInterfaceId, const char*, dtInterface);

#ifdef __cplusplus
}
#endif


#endif
