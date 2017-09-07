# IoTHub_device_auth_emulator Requirements

================================

## Overview

IoTHub_device_auth_emulator module implements the emulation of a tpm module interface.

## Dependencies

IoTHub_device_auth
Sas_Token

## Exposed API

```c
extern CONCRETE_XDA_HANDLE dev_auth_emulator_create(const SECURITY_INFO* xda_create_parameters);
extern void dev_auth_emulator_destroy(CONCRETE_XDA_HANDLE handle);
extern DEVICE_AUTH_TYPE dev_auth_emulator_get_auth_type(CONCRETE_XDA_HANDLE handle);
extern void* dev_auth_emulator_generate_credentials(CONCRETE_XDA_HANDLE handle, const DEVICE_AUTH_CREDENTIAL_INFO* dev_auth_cred);
extern char* dev_auth_emulator_get_endorsement_key(CONCRETE_XDA_HANDLE handle);
extern int dev_auth_emulator_store_data(CONCRETE_XDA_HANDLE handle, const unsigned char* data, size_t data_len);
extern unsigned char* dev_auth_emulator_retrieve_data(CONCRETE_XDA_HANDLE handle, size_t* data_len);
extern unsigned char* dev_auth_emulator_decrypt_value(CONCRETE_XDA_HANDLE handle, const unsigned char* encrypt_data, size_t length, size_t* decpypt_len);

extern const XDA_INTERFACE_DESCRIPTION* dev_auth_emulator_interface_desc();
```

### dev_auth_emulator_create

```c
extern CONCRETE_XDA_HANDLE dev_auth_emulator_create(const SECURITY_INFO* xda_create_parameters);
```

**SRS_DEV_AUTH_EMULATOR_07_001: [** If xda_create_parameters is NULL `dev_auth_emulator_create` shall return NULL. **]**

**SRS_DEV_AUTH_EMULATOR_07_002: [** If any failure is encountered `dev_auth_emulator_create` shall return NULL **]**

**SRS_DEV_AUTH_EMULATOR_07_003: [** On success `dev_auth_emulator_create` shall allocate a new instance of the device auth interface. **]**


### dev_auth_emulator_destroy

```c
extern void dev_auth_emulator_destroy(CONCRETE_XDA_HANDLE handle);
```

**SRS_DEV_AUTH_EMULATOR_07_005: [** if handle is NULL, `dev_auth_emulator_destroy` shall do nothing. **]**

**SRS_DEV_AUTH_EMULATOR_07_004: [** `dev_auth_emulator_destroy` shall free the CONCRETE_XDA_HANDLE instance. **]**

**SRS_DEV_AUTH_EMULATOR_07_006: [** `dev_auth_emulator_destroy` shall free all resources allocated in this module. **]**


### dev_auth_emulator_get_auth_type

```c
extern DEVICE_AUTH_TYPE dev_auth_emulator_get_auth_type(CONCRETE_XDA_HANDLE handle);
```

**SRS_DEV_AUTH_EMULATOR_07_007: [** if handle is NULL, `dev_auth_emulator_get_auth_type` shall return `AUTH_TYPE_UNKNOWN` **]**

**SRS_DEV_AUTH_EMULATOR_07_008: [** `dev_auth_emulator_get_auth_type` shall return `AUTH_TYPE_SAS` **]**


### dev_auth_emulator_generate_credentials

```c
extern void* dev_auth_emulator_generate_credentials(CONCRETE_XDA_HANDLE handle, const DEVICE_AUTH_CREDENTIAL_INFO* dev_auth_cred);
```

**SRS_DEV_AUTH_EMULATOR_07_009: [** if handle or dev_auth_cred is NULL, `dev_auth_emulator_generate_credentials` shall return NULL. **]**

**SRS_DEV_AUTH_EMULATOR_07_010: [** `dev_auth_emulator_generate_credentials` shall construct a SAS Token with the information defined in the DEVICE_AUTH_CREDENTIAL_INFO structure. **]**

**SRS_DEV_AUTH_EMULATOR_07_011: [** If a failure is encountered, `dev_auth_emulator_generate_credentials` shall return NULL. **]**

**SRS_DEV_AUTH_EMULATOR_07_012: [** Upon success `dev_auth_emulator_generate_credentials` shall return the created SAS Token. **]**


### dev_auth_emulator_get_endorsement_key

```c
extern char* dev_auth_emulator_get_endorsement_key(CONCRETE_XDA_HANDLE handle);
```

**SRS_DEV_AUTH_EMULATOR_07_013: [** If handle is NULL `dev_auth_emulator_get_endorsement_key` shall return NULL. **]**

**SRS_DEV_AUTH_EMULATOR_07_014: [** `dev_auth_emulator_get_endorsement_key` shall allocate and return the device key as the endorsement key. **]**

**SRS_DEV_AUTH_EMULATOR_07_015: [** If a failure is encountered, `dev_auth_emulator_get_endorsement_key` shall return NULL. **]**


### dev_auth_emulator_store_data

```c
extern int dev_auth_emulator_store_data(CONCRETE_XDA_HANDLE handle, const unsigned char* data, size_t data_len);
```

**SRS_DEV_AUTH_EMULATOR_07_016: [** If handle or data is NULL or data_len is 0, `dev_auth_emulator_store_data` shall return a non-zero value. **]**

**SRS_DEV_AUTH_EMULATOR_07_017: [** `dev_auth_emulator_store_data` shall store the the data in a BUFFER variable. **]**

**SRS_DEV_AUTH_EMULATOR_07_018: [** If the a data object has been previously stored in the BUFFER variable, `dev_auth_emulator_store_data` shall overwrite the existing value with the new data object. **]**

**SRS_DEV_AUTH_EMULATOR_07_019: [** If any failure is encountered, `dev_auth_emulator_store_data` shall return a non-zero value. **]**

**SRS_DEV_AUTH_EMULATOR_07_025: [** On success `dev_auth_emulator_store_data` shall return zero. **]**


### dev_auth_emulator_retrieve_data

```c
extern unsigned char* dev_auth_emulator_retrieve_data(CONCRETE_XDA_HANDLE handle, size_t* data_len);
```

**SRS_DEV_AUTH_EMULATOR_07_020: [** If handle or data_len is NULL, `dev_auth_emulator_retrieve_data` shall return a non-zero value. **]**

**SRS_DEV_AUTH_EMULATOR_07_021: [** `dev_auth_emulator_retrieve_data` shall return the data previously stored by `dev_auth_emulator_store_data` and add the length of the data in data_len. **]**

**SRS_DEV_AUTH_EMULATOR_07_022: [** If `dev_auth_emulator_store_data` has not been called `dev_auth_emulator_retrieve_data` shall return NULL. **]**

**SRS_DEV_AUTH_EMULATOR_07_023: [** If an error is encountered `dev_auth_emulator_store_data`shall return NULL. **]**

### dev_auth_emulator_decrypt_value

```c
extern unsigned char* dev_auth_emulator_decrypt_value(CONCRETE_XDA_HANDLE handle, const unsigned char* encrypt_data, size_t length, size_t* decpypt_len);
```

**SRS_DEV_AUTH_EMULATOR_07_024: [** `dev_auth_emulator_decrypt_value` is not implemented at this time.  It shall be completed as soon as information becomes available. **]**


### dev_auth_emulator_interface_desc

```c
extern const XDA_INTERFACE_DESCRIPTION* dev_auth_emulator_interface_desc();
```

**SRS_DEV_AUTH_EMULATOR_07_026: [** `dev_auth_emulator_interface_desc` shall return the XDA_INTERFACE_DESCRIPTION structure. **]**
