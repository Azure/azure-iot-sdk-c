# IoTHub_device_auth_tpm Requirements

================================

## Overview

IoTHub_device_auth_tpm module implements the interface over a tpm module.

## Dependencies

IoTHub_device_auth
Sas_Token

## Exposed API

```c
extern HSM_CLIENT_HANDLE hsm_client_tpm_create();
extern void hsm_client_tpm_destroy(HSM_CLIENT_HANDLE handle);
extern char* hsm_client_tpm_get_endorsement_key(HSM_CLIENT_HANDLE handle);
extern char* hsm_client_tpm_get_storage_key(HSM_CLIENT_HANDLE handle);
int hsm_client_tpm_import_key(SEC_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
BUFFER_HANDLE hsm_client_tpm_sign_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);
BUFFER_HANDLE hsm_client_tpm_decrypt_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

extern const SEC_TPM_INTERFACE* hsm_client_tpm_interface_desc();
```

### hsm_client_tpm_create

```c
SEC_DEVICE_HANDLE hsm_client_tpm_create()
```

**SRS_HSM_CLIENT_TPM_07_001: [** If any failure is encountered `hsm_client_tpm_create` shall return NULL **]**

**SRS_HSM_CLIENT_TPM_07_002: [** On success `hsm_client_tpm_create` shall allocate a new instance of the secure device tpm interface. **]**

**SRS_HSM_CLIENT_TPM_07_030: [** `hsm_client_tpm_create` shall call into the tpm_codec to initialize a TSS session. **]**

**SRS_HSM_CLIENT_TPM_07_031: [** `hsm_client_tpm_create` shall get a handle to the Endorsement Key and Storage Root Key. **]**

### hsm_client_tpm_destroy

```c
void hsm_client_tpm_destroy(HSM_CLIENT_HANDLE handle)
```

**SRS_HSM_CLIENT_TPM_07_005: [** if handle is NULL, `hsm_client_tpm_destroy` shall do nothing. **]**

**SRS_HSM_CLIENT_TPM_07_004: [** `hsm_client_tpm_destroy` shall free the HSM_CLIENT_HANDLE instance. **]**

**SRS_HSM_CLIENT_TPM_07_006: [** `hsm_client_tpm_destroy` shall free all resources allocated in this module. **]**


### hsm_client_tpm_import_key

```c
int hsm_client_tpm_import_key(SEC_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
```

**SRS_HSM_CLIENT_TPM_07_007: [** if handle or key are NULL, or key_len is 0 `hsm_client_tpm_import_key` shall return a non-zero value **]**

**SRS_HSM_CLIENT_TPM_07_008: [** On success `hsm_client_tpm_import_key` shall return zero **]**

`hsm_client_tpm_import_key` shall establish a tpm session in preparation to inserting the key into the tpm.

### secure_dev_emulator_get_endorsement_key

```c
char* hsm_client_tpm_get_endorsement_key(HSM_CLIENT_HANDLE handle);
```

**SRS_HSM_CLIENT_TPM_07_013: [** If handle is NULL `hsm_client_tpm_get_endorsement_key` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_027: [** If the `ek_public` was not initialized `hsm_client_tpm_get_endorsement_key` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_014: [** `hsm_client_tpm_get_endorsement_key` shall allocate and return the Endorsement Key. **]**

**SRS_HSM_CLIENT_TPM_07_015: [** If a failure is encountered, `hsm_client_tpm_get_endorsement_key` shall return NULL. **]**


### hsm_client_tpm_get_storage_key

```c
extern int hsm_client_tpm_get_storage_key(HSM_CLIENT_HANDLE handle);
```

**SRS_HSM_CLIENT_TPM_07_016: [** If `handle` is NULL, `hsm_client_tpm_get_storage_key` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_017: [** If the `srk_public` value was not initialized, `hsm_client_tpm_get_storage_key` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_018: [** `hsm_client_tpm_get_storage_key` shall allocate and return the Storage Root Key. **]**

**SRS_HSM_CLIENT_TPM_07_019: [** If any failure is encountered, `hsm_client_tpm_get_storage_key` shall return NULL. **]**

### hsm_client_tpm_sign_data

```c
BUFFER_HANDLE hsm_client_tpm_sign_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
```

**SRS_HSM_CLIENT_TPM_07_020: [** If `handle` or `data` is NULL or `data_len` is 0, `hsm_client_tpm_sign_data` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_021: [** `hsm_client_tpm_sign_data` shall call into the tpm to hash the supplied `data` value. **]**

**SRS_HSM_CLIENT_TPM_07_022: [** If hashing the `data` was successful, `hsm_client_tpm_sign_data` shall create a BUFFER_HANDLE with the supplied signed data. **]**

**SRS_HSM_CLIENT_TPM_07_023: [** If an error is encountered `hsm_client_tpm_sign_data` shall return NULL. **]**

### hsm_client_tpm_decrypt_data

```c
BUFFER_HANDLE hsm_client_tpm_decrypt_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
```

**SRS_HSM_CLIENT_TPM_07_025: [** If `handle` or `data` is NULL or `data_len` is 0, `hsm_client_tpm_decrypt_data` shall return NULL. **]**

**SRS_HSM_CLIENT_TPM_07_024: [** `hsm_client_tpm_decrypt_data` shall call into the tpm to decrypt the supplied `data` value. **]**


**SRS_HSM_CLIENT_TPM_07_029: [** If an error is encountered `hsm_client_tpm_decrypt_data` shall return NULL. **]**

### hsm_client_tpm_interface

```c
const SEC_TPM_INTERFACE* hsm_client_tpm_interface()
```

**SRS_HSM_CLIENT_TPM_07_026: [** `hsm_client_tpm_interface` shall return the SEC_TPM_INTERFACE structure. **]**
