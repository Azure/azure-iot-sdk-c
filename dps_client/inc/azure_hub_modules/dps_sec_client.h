// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_SEC_CLIENT_CLIENT_H
#define DPS_SEC_CLIENT_CLIENT_H

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
#include "azure_c_shared_utility/buffer_.h"

typedef struct DPS_SEC_INFO_TAG* DPS_SEC_HANDLE;

#define DPS_SEC_RESULT_VALUES   \
    DPS_SEC_SUCCESS,            \
    DPS_SEC_INVALID_ARG,        \
    DPS_SEC_ERROR,              \
    DPS_SEC_STATUS_UNASSIGNED,   \
    DPS_SEC_STATUS_ASSIGNING

DEFINE_ENUM(DPS_SEC_RESULT, DPS_SEC_RESULT_VALUES);

#define DPS_SEC_TYPE_VALUES \
    DPS_SEC_TYPE_UNKNOWN,   \
    DPS_SEC_TYPE_TPM,       \
    DPS_SEC_TYPE_X509

DEFINE_ENUM(DPS_SEC_TYPE, DPS_SEC_TYPE_VALUES);

MOCKABLE_FUNCTION(, DPS_SEC_HANDLE, dps_sec_create);
MOCKABLE_FUNCTION(, void, dps_sec_destroy, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, DPS_SEC_TYPE, dps_sec_get_type, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_registration_id, DPS_SEC_HANDLE, handle);

// TPM
MOCKABLE_FUNCTION(, BUFFER_HANDLE, dps_sec_get_endorsement_key, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, BUFFER_HANDLE, dps_sec_get_storage_key, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_sec_import_key, DPS_SEC_HANDLE, handle, const unsigned char*, key, size_t, key_len);
MOCKABLE_FUNCTION(, char*, dps_sec_construct_sas_token, DPS_SEC_HANDLE, handle, const char*, token_scope, const char*, key_name, size_t, expiry_time);

// X509
MOCKABLE_FUNCTION(, char*, dps_sec_get_certificate, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_alias_key, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_signer_cert, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_root_cert, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_root_key, DPS_SEC_HANDLE, handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_SEC_CLIENT_CLIENT_H
