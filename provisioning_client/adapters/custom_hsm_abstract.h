// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_HSM_CUSTOM_H
#define DPS_HSM_CUSTOM_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdbool>
extern "C"
{
#else
#include <stddef.h>
#include <stdbool.h>
#endif

#include "azure_hub_modules\secure_device_factory.h"
#include "azure_hub_modules\iothub_security_factory.h"

/*typedef struct HSM_SECURE_DEVICE_INFO_TAG* DPS_SECURE_DEVICE_HANDLE;
typedef struct HSM_SECURE_DEVICE_INFO_TAG* SECURITY_DEVICE_HANDLE;*/

extern DPS_SECURE_DEVICE_HANDLE dps_hsm_custom_create();
extern void dps_hsm_custom_destroy(DPS_SECURE_DEVICE_HANDLE handle);

extern SECURITY_DEVICE_HANDLE iothub_hsm_custom_create();
extern void iothub_hsm_custom_destroy(SECURITY_DEVICE_HANDLE handle);

// TPM functions
extern BUFFER_HANDLE dps_hsm_custom_get_endorsement_key(DPS_SECURE_DEVICE_HANDLE handle);
extern BUFFER_HANDLE dps_hsm_custom_get_storage_key(DPS_SECURE_DEVICE_HANDLE handle);
extern int dps_hsm_custom_import_key(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len);
extern BUFFER_HANDLE dps_hsm_custom_sign_data(DPS_SECURE_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

extern BUFFER_HANDLE iothub_hsm_custom_sign_data(SECURITY_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

// RIOT Functions
extern char* dps_hsm_custom_get_certificate(DPS_SECURE_DEVICE_HANDLE handle);
extern char* dps_hsm_custom_get_alias_key(DPS_SECURE_DEVICE_HANDLE handle);
extern char* dps_hsm_custom_get_signer_cert(DPS_SECURE_DEVICE_HANDLE handle);
extern char* dps_hsm_custom_get_common_name(DPS_SECURE_DEVICE_HANDLE handle);

extern const char* iothub_hsm_custom_get_certificate(SECURITY_DEVICE_HANDLE handle);
extern const char* iothub_hsm_custom_get_alias_key(SECURITY_DEVICE_HANDLE handle);

extern int initialize_custom_hsm();
extern void deinitialize_custom_hsm();

extern const SEC_TPM_INTERFACE* dps_custom_hsm_tpm_interface();
extern const SEC_X509_INTERFACE* dps_custom_hsm_x509_interface();

extern const SAS_SECURITY_INTERFACE* iothub_custom_hsm_sas_interface();
extern const X509_SECURITY_INTERFACE* iothub_custom_hsm_x509_interface();

#ifdef __cplusplus
}
#endif

#endif // DPS_HSM_CUSTOM_H
