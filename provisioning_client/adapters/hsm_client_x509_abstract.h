// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_X509_ABSTRACT_H
#define HSM_X509_ABSTRACT_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"

#include "hsm_client_data.h"

MOCKABLE_FUNCTION(, const HSM_CLIENT_X509_INTERFACE*, hsm_client_x509_interface);
MOCKABLE_FUNCTION(, int, hsm_init_x509_system);
MOCKABLE_FUNCTION(, void, hsm_deinit_x509_system);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_X509_ABSTRACT_H
