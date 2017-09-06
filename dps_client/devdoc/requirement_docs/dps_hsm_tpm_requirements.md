# IoTHub_device_auth_tpm Requirements

================================

## Overview

IoTHub_device_auth_tpm module implements the interface over a tpm module.

## Dependencies

IoTHub_device_auth
Sas_Token

## Exposed API

```c
extern SEC_DEVICE_INFO secure_dev_tpm_create();
extern void secure_dev_tpm_destroy(SEC_DEVICE_INFO handle);
extern char* secure_dev_tpm_get_endorsement_key(SEC_DEVICE_INFO handle);
extern char* secure_dev_tpm_get_storage_key(SEC_DEVICE_INFO handle);
int secure_dev_tpm_import_key(SEC_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
BUFFER_HANDLE secure_dev_tpm_sign_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);
BUFFER_HANDLE secure_dev_tpm_decrypt_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len);

extern const SEC_TPM_INTERFACE* secure_dev_tpm_interface_desc();
```

### secure_dev_tpm_create

```c
SEC_DEVICE_HANDLE secure_dev_tpm_create()
```

**SRS_SECURE_DEVICE_TPM_07_001: [** If any failure is encountered `secure_dev_tpm_create` shall return NULL **]**

**SRS_SECURE_DEVICE_TPM_07_002: [** On success `secure_dev_tpm_create` shall allocate a new instance of the secure device tpm interface. **]**

**SRS_SECURE_DEVICE_TPM_07_030: [** `secure_dev_tpm_create` shall call into the tpm_codec to initialize a TSS session. **]**

**SRS_SECURE_DEVICE_TPM_07_031: [** `secure_dev_tpm_create` shall get a handle to the Endorsement Key and Storage Root Key. **]**

### secure_dev_tpm_destroy

```c
void secure_dev_tpm_destroy(SEC_DEVICE_INFO handle)
```

**SRS_SECURE_DEVICE_TPM_07_005: [** if handle is NULL, `secure_dev_tpm_destroy` shall do nothing. **]**

**SRS_SECURE_DEVICE_TPM_07_004: [** `secure_dev_tpm_destroy` shall free the SEC_DEVICE_INFO instance. **]**

**SRS_SECURE_DEVICE_TPM_07_006: [** `secure_dev_tpm_destroy` shall free all resources allocated in this module. **]**


### secure_dev_tpm_import_key

```c
int secure_dev_tpm_import_key(SEC_DEVICE_HANDLE handle, const unsigned char* key, size_t key_len)
```

**SRS_SECURE_DEVICE_TPM_07_007: [** if handle or key are NULL, or key_len is 0 `secure_dev_tpm_import_key` shall return a non-zero value **]**

**SRS_SECURE_DEVICE_TPM_07_008: [** On success `secure_dev_tpm_import_key` shall return zero **]**

`secure_dev_tpm_import_key` shall establish a tpm session in preparation to inserting the key into the tpm.

### secure_dev_emulator_get_endorsement_key

```c
char* secure_dev_tpm_get_endorsement_key(SEC_DEVICE_INFO handle);
```

**SRS_SECURE_DEVICE_TPM_07_013: [** If handle is NULL `secure_dev_tpm_get_endorsement_key` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_027: [** If the `ek_public` was not initialized `secure_dev_tpm_get_endorsement_key` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_014: [** `secure_dev_tpm_get_endorsement_key` shall allocate and return the Endorsement Key. **]**

**SRS_SECURE_DEVICE_TPM_07_015: [** If a failure is encountered, `secure_dev_tpm_get_endorsement_key` shall return NULL. **]**


### secure_dev_tpm_get_storage_key

```c
extern int secure_dev_tpm_get_storage_key(SEC_DEVICE_INFO handle);
```

**SRS_SECURE_DEVICE_TPM_07_016: [** If `handle` is NULL, `secure_dev_tpm_get_storage_key` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_017: [** If the `srk_public` value was not initialized, `secure_dev_tpm_get_storage_key` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_018: [** `secure_dev_tpm_get_storage_key` shall allocate and return the Storage Root Key. **]**

**SRS_SECURE_DEVICE_TPM_07_019: [** If any failure is encountered, `secure_dev_tpm_get_storage_key` shall return NULL. **]**

### secure_dev_tpm_sign_data

```c
BUFFER_HANDLE secure_dev_tpm_sign_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
```

**SRS_SECURE_DEVICE_TPM_07_020: [** If `handle` or `data` is NULL or `data_len` is 0, `secure_dev_tpm_sign_data` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_021: [** `secure_dev_tpm_sign_data` shall call into the tpm to hash the supplied `data` value. **]**

**SRS_SECURE_DEVICE_TPM_07_022: [** If hashing the `data` was successful, `secure_dev_tpm_sign_data` shall create a BUFFER_HANDLE with the supplied signed data. **]**

**SRS_SECURE_DEVICE_TPM_07_023: [** If an error is encountered `secure_dev_tpm_sign_data` shall return NULL. **]**

### secure_dev_tpm_decrypt_data

```c
BUFFER_HANDLE secure_dev_tpm_decrypt_data(SEC_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
```

**SRS_SECURE_DEVICE_TPM_07_025: [** If `handle` or `data` is NULL or `data_len` is 0, `secure_dev_tpm_decrypt_data` shall return NULL. **]**

**SRS_SECURE_DEVICE_TPM_07_024: [** `secure_dev_tpm_decrypt_data` shall call into the tpm to decrypt the supplied `data` value. **]**


**SRS_SECURE_DEVICE_TPM_07_029: [** If an error is encountered `secure_dev_tpm_decrypt_data` shall return NULL. **]**

### secure_dev_tpm_interface

```c
const SEC_TPM_INTERFACE* secure_dev_tpm_interface()
```

**SRS_SECURE_DEVICE_TPM_07_026: [** `secure_dev_tpm_interface` shall return the SEC_TPM_INTERFACE structure. **]**
