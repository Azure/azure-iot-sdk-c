# secure_enclave_client Requirements

================================

## Overview

secure_enclave_client module implements the abstraction of the secure_enclave device.

## Dependencies

## Exposed API

```c
typedef struct SEC_INFO_TAG* SEC_HANDLE;

#define SECURE_ENCLAVE_TYPE_VALUES \
    SECURE_ENCLAVE_TYPE_UNKNOWN,   \
    SECURE_ENCLAVE_TYPE_TPM,       \
    SECURE_ENCLAVE_TYPE_RIOT

DEFINE_ENUM(SECURE_ENCLAVE_TYPE, SECURE_ENCLAVE_TYPE_VALUES);

MOCKABLE_FUNCTION(, SEC_HANDLE, secure_enclave_create);
MOCKABLE_FUNCTION(, void, secure_enclave_destroy, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, SECURE_ENCLAVE_TYPE, secure_enclave_get_type, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_enclave_get_registration_id, SEC_HANDLE, handle);

MOCKABLE_FUNCTION(, char*, secure_enclave_get_endorsement_key, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_enclave_get_storage_key, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, int, secure_enclave_import_key, SEC_HANDLE, handle, const char*, key);

MOCKABLE_FUNCTION(, char*, secure_enclave_get_certificate, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_enclave_get_alias_key, SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_enclave_get_signer_cert, SEC_HANDLE, handle);
```

### secure_enclave_create

```c
SEC_HANDLE secure_enclave_create()
```

**SRS_SECURE_ENCLAVE_CLIENT_07_001: [** `secure_enclave_create` shall allocate the SEC_INFO and return with SEC_HANDLE. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_014: [** `secure_enclave_create` shall ensure the secure enclave interface pointers are not NULL for the specified secure enclave interface. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_015: [** `secure_enclave_create` shall call `secure_device_create` on the secure enclave interface. **]**

**SRS_DRS_SEC_CLIENT_07_001: [** If any error is encountered, `secure_enclave_create` shall return NULL. **]**

### secure_enclave_destroy

```c
void secure_enclave_destroy(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_016: [** If handle is NULL, `secure_enclave_destroy` shall do nothing. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_017: [** `secure_enclave_destroy` shall free the SEC_HANDLE instance. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_018: [** `secure_enclave_destroy` shall free all resources allocated in this module. **]**


### secure_enclave_get_type

```c
SECURE_ENCLAVE_TYPE secure_enclave_get_type(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_019: [** If handle is NULL `secure_enclave_get_type` shall return `SECURE_ENCLAVE_TYPE_UNKNOWN` **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_020: [** `secure_enclave_get_type` shall return the 'SECURE_ENCLAVE_TYPE' of the underlying secure enclave. **]**

### drs_sec_get_registration_id

```c
char* secure_enclave_get_registration_id(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_010: [** if handle is NULL `secure_enclave_get_registration_id` shall return NULL. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_011: [** For TPM `secure_enclave_get_registration_id` shall generate a base64 encoded sha256 of the Endorsement Key. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_039: [** For RIOT `secure_enclave_get_registration_id` shall get the Common Name of the certificate. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_012: [** If a failure is encountered, `secure_enclave_get_registration_id` shall return NULL. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_013: [** Upon success `secure_enclave_get_registration_id` shall return the registration id of the secure enclave. **]**

### secure_enclave_get_endorsement_key

```c
char* secure_enclave_get_endorsement_key(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_021: [** If handle is NULL `secure_enclave_get_endorsement_key` shall return NULL. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_022: [** `secure_enclave_get_endorsement_key` shall return the endorsement key returned by the secure_device_get_ek secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_023: [** If the sec_type is SECURE_ENCLAVE_TYPE_RIOT, `secure_enclave_get_endorsement_key` shall return NULL. **]**

### secure_enclave_get_storage_key

```c
char* secure_enclave_get_storage_key(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_024: [** If handle is NULL `secure_enclave_get_storage_key` shall return NULL. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_025: [** `secure_enclave_get_storage_key` shall return the endorsement key returned by the `secure_device_get_srk` secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_026: [** If the sec_type is SECURE_ENCLAVE_TYPE_RIOT, `secure_enclave_get_storage_key` shall return NULL. **]**

### secure_enclave_import_key

```c
int secure_enclave_import_key(SEC_HANDLE handle, const char* key)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_027: [** If handle or key are NULL `secure_enclave_import_key` shall return a non-zero value. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_028: [** `secure_enclave_import_key` shall import the specified key into the tpm using `secure_device_import_key` secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_029: [** If the sec_type is not SECURE_ENCLAVE_TYPE_TPM, `secure_enclave_import_key` shall return NULL. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_040: [** If `secure_device_import_key` returns an error `secure_enclave_import_key` shall return a non-zero value. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_041: [** `secure_device_import_key` shall Base64 Decode the supplied key to get the unsigned char value. **]**

### secure_enclave_get_certificate

```c
char* secure_enclave_get_certificate(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_030: [** If handle or key are NULL `secure_enclave_get_certificate` shall return a non-zero value. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_031: [** `secure_enclave_get_certificate` shall return the specified cert into the client using `secure_device_get_cert` secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_032: [** If the sec_type is not SECURE_ENCLAVE_TYPE_RIOT, `secure_enclave_get_certificate` shall return NULL. **]**

### secure_enclave_get_alias_key

```c
char* secure_enclave_get_alias_key(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_033: [** If handle or key are NULL `secure_enclave_get_alias_key` shall return a non-zero value. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_034: [** `secure_enclave_get_alias_key` shall return the specified alias key into the client using `secure_device_get_ak` secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_035: [** If the sec_type is not SECURE_ENCLAVE_TYPE_RIOT, `secure_enclave_get_alias_key` shall return NULL. **]**

### secure_enclave_get_signer_cert

```c
char* secure_enclave_get_signer_cert(SEC_HANDLE handle)
```

**SRS_SECURE_ENCLAVE_CLIENT_07_036: [** If handle or key are NULL `secure_enclave_get_signer_cert` shall return a non-zero value. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_037: [** `secure_enclave_get_signer_cert` shall return the specified signer cert into the client using `secure_device_get_signer_cert` secure enclave function. **]**

**SRS_SECURE_ENCLAVE_CLIENT_07_038: [** If the sec_type is not SECURE_ENCLAVE_TYPE_RIOT, `secure_enclave_get_signer_cert` shall return NULL. **]**

