// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HSM_TPM_ABSTRACT_H
#define HSM_TPM_ABSTRACT_H

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

MOCKABLE_FUNCTION(, const HSM_CLIENT_TPM_INTERFACE*, hsm_client_tpm_interface);
MOCKABLE_FUNCTION(, int, hsm_client_tpm_init);
MOCKABLE_FUNCTION(, void, hsm_client_tpm_deinit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HSM_TPM_ABSTRACT_H
