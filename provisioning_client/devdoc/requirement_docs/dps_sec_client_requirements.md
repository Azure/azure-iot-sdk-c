# dps_sec_client Requirements

================================

## Overview

dps_sec_client module implements the abstraction of the DPS module interface.

## Dependencies

## Exposed API

```c
typedef struct DPS_SEC_INFO_TAG* DPS_SEC_HANDLE;

#define DPS_SEC_RESULT_VALUES   \
    DPS_SEC_SUCCESS,            \
    DPS_SEC_INVALID_ARG,        \
    DPS_SEC_ERROR,              \
    DPS_SEC_STATUS_UNASSIGNED

DEFINE_ENUM(DPS_SEC_RESULT, DPS_SEC_RESULT_VALUES);

#define DPS_SEC_TYPE_VALUES \
    DPS_SEC_TYPE_UNKNOWN,   \
    DPS_SEC_TYPE_TPM,       \
    DPS_SEC_TYPE_RIOT

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

// Riot
MOCKABLE_FUNCTION(, char*, dps_sec_get_certificate, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_alias_key, DPS_SEC_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_sec_get_signer_cert, DPS_SEC_HANDLE, handle);
```

### dps_sec_create

```c
DPS_SEC_HANDLE dps_sec_create();
```

**SRS_DPS_SEC_CLIENT_07_001: [** `dps_sec_create` shall allocate the DPS_SEC_INFO and return with DPS_SEC_HANDLE. **]**

**SRS_DPS_SEC_CLIENT_07_002: [** If any failure is encountered `dps_sec_create` shall return NULL **]**

**SRS_DPS_SEC_CLIENT_07_003: [** `dps_sec_create` shall validate the specified secure enclave interface to ensure. **]**

**SRS_DPS_SEC_CLIENT_07_004: [** `dps_sec_create` shall call `secure_device_create` on the secure enclave interface. **]**


### dps_sec_destroy

```c
void dps_sec_destroy(DPS_SEC_HANDLE handle)
```

**SRS_DPS_SEC_CLIENT_07_005: [** if handle is NULL, `dps_sec_destroy` shall do nothing. **]**

**SRS_DPS_SEC_CLIENT_07_006: [** `dps_sec_destroy` shall free the DPS_SEC_HANDLE instance. **]**

**SRS_DPS_SEC_CLIENT_07_007: [** `dps_sec_destroy` shall free all resources allocated in this module. **]**


### dps_sec_get_type

```c
DPS_SEC_TYPE dps_sec_get_type(DPS_SEC_HANDLE handle)
```

**SRS_DPS_SEC_CLIENT_07_008: [** if handle is NULL `dps_sec_get_type` shall return `DPS_SEC_TYPE_UNKNOWN` **]**

**SRS_DPS_SEC_CLIENT_07_009: [** `dps_sec_get_type` shall return the DPS_SEC_TYPE of the underlying secure enclave. **]**


### dps_sec_get_registration_id

```c
char* dps_sec_get_registration_id(DPS_SEC_HANDLE handle)
```

**SRS_DPS_SEC_CLIENT_07_010: [** if handle is NULL `dps_sec_get_registration_id` shall return NULL. **]**

**SRS_DPS_SEC_CLIENT_07_011: [** `dps_sec_get_registration_id` shall load the registration ID if it has not been previously loaded. **]**

**SRS_DPS_SEC_CLIENT_07_012: [** If a failure is encountered, `dps_sec_get_registration_id` shall return NULL. **]**

**SRS_DPS_SEC_CLIENT_07_013: [** Upon success `dps_sec_get_registration_id` shall return the registration id of the secure enclave. **]**


### dps_sec_construct_sas_token

```c
char* dps_sec_construct_sas_token(DPS_SEC_HANDLE handle, const char* token_scope, const char* key_name, size_t expiry_time)
```

**SRS_DPS_SEC_CLIENT_07_014: [** If `handle`, `token_scope`, or `key_name` is NULL `dps_sec_construct_sas_token` shall return NULL. **]**

**SRS_DPS_SEC_CLIENT_07_015: [** `dps_sec_construct_sas_token` shall sign the `token_scope/expiry_time` with the HSM by calling `secure_device_sign_data` **]**

**SRS_DPS_SEC_CLIENT_07_016: [** if the `sec_type` is not `DPS_SEC_TYPE_TPM`, `dps_sec_construct_sas_token` shall return NULL **]**

**SRS_DPS_SEC_CLIENT_07_017: [** `dps_sec_construct_sas_token` shall construct the sas token detail in sastoken.h **]**

**SRS_DPS_SEC_CLIENT_07_018: [** If successful `dps_sec_construct_sas_token` shall return the sas token. **]**

**SRS_DPS_SEC_CLIENT_07_026: [** If any failure found `dps_sec_construct_sas_token` shall return the NULL. **]**

### dps_sec_get_certificate

```c
char* dps_sec_get_certificate(DPS_SEC_HANDLE handle)
```

**SRS_DPS_SEC_CLIENT_07_019: [** If handle, json_reply, iothub_uri, or device_id is NULL `dps_sec_process_reply` shall return a DPS_SEC_INVALID_ARG. **]**

**SRS_DPS_SEC_CLIENT_07_020: [** `dps_sec_process_reply` shall parse the json_reply to get the status. **]**

**SRS_DPS_SEC_CLIENT_07_021: [** If the JSON `status` field is not found or if the value is not `assigned`, `dps_sec_process_reply` shall return DPS_SEC_STATUS_UNASSIGNED. **]**

**SRS_DPS_SEC_CLIENT_07_022: [** If the JSON `status` field is `assigned`, `dps_sec_process_reply` shall extract `registrationStatus`, `assignedHub`, `deviceId`. **]**

**SRS_DPS_SEC_CLIENT_07_023: [** If the sec_type is DPS_SEC_TYPE_TPM then `dps_sec_process_reply` shall extract the `authenticationKey`. **]**

**SRS_DPS_SEC_CLIENT_07_024: [** If any JSON field is not present `dps_sec_process_reply` shall return DPS_SEC_ERROR. **]**

**SRS_DPS_SEC_CLIENT_07_025: [** On success `dps_sec_process_reply` shall return DPS_SEC_SUCCESS with a valid iothub_uri and device_id. **]**

**SRS_DPS_SEC_CLIENT_07_027: [** If any JSON other paring error occurs `dps_sec_process_reply` shall return DPS_SEC_ERROR. **]**