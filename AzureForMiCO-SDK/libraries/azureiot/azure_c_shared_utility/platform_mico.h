// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PLATFORMMICO_H
#define PLATFORMMICO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xio.h"
#include "umock_c_prod.h"

    MOCKABLE_FUNCTION(, int, platform_init);
    MOCKABLE_FUNCTION(, void, platform_deinit);
    MOCKABLE_FUNCTION(, const IO_INTERFACE_DESCRIPTION*, platform_get_default_tlsio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_H */
