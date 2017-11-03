# IoTHub_device_auth Requirements

================================

## Overview

IoTHub_device_auth module implements the interface abstraction over a Secure Enclave to enable the communication with the iothub.

## Exposed API

```c
typedef struct IOTHUB_SECURITY_INFO_TAG* IOTHUB_SECURITY_HANDLE;

#define DEVICE_AUTH_TYPE_VALUES \
    AUTH_TYPE_UNKNOWN,          \
    AUTH_TYPE_SAS,              \
    AUTH_TYPE_X509

DEFINE_ENUM(DEVICE_AUTH_TYPE, DEVICE_AUTH_TYPE_VALUES);

typedef struct DEVICE_AUTH_SAS_INFO_TAG
{
    uint64_t expiry_seconds;
    const char* token_scope;
    const char* key_name;
} DEVICE_AUTH_SAS_INFO;

typedef struct DEVICE_AUTH_SAS_RESULT_TAG
{
    char* sas_token;
} DEVICE_AUTH_SAS_RESULT;

typedef struct DEVICE_AUTH_X509_RESULT_TAG
{
    const char* x509_cert;
    const char* x509_alias_key;
} DEVICE_AUTH_X509_RESULT;

typedef struct DEVICE_AUTH_CREDENTIAL_INFO_TAG
{
    DEVICE_AUTH_TYPE dev_auth_type;
    DEVICE_AUTH_SAS_INFO sas_info;
} DEVICE_AUTH_CREDENTIAL_INFO;

typedef struct CREDENTIAL_RESULT_TAG
{
    DEVICE_AUTH_TYPE dev_auth_type;
    union
    {
        DEVICE_AUTH_SAS_RESULT sas_result;
        DEVICE_AUTH_X509_RESULT x509_result;
    } auth_cred_result;
} CREDENTIAL_RESULT;

IOTHUB_SECURITY_HANDLE iothub_device_auth_create(void);
void device_auth_destroy(IOTHUB_SECURITY_HANDLE handle);
DEVICE_AUTH_TYPE device_auth_get_auth_type(IOTHUB_SECURITY_HANDLE handle);
CREDENTIAL_RESULT* iothub_device_auth_generate_credentials(IOTHUB_SECURITY_HANDLE handle, const DEVICE_AUTH_CREDENTIAL_INFO* dev_auth_cred);
```

### device_auth_create

Creates the IOTHUB_SECURITY_INFO using the specified interface description and the custom parameter if specified

```c
IOTHUB_SECURITY_HANDLE iothub_device_auth_create()
```

**IOTHUB_DEV_AUTH_07_001: [** if any failure is encountered `iothub_device_auth_create` shall return NULL. **]**

**IOTHUB_DEV_AUTH_07_002: [** `iothub_device_auth_create` shall allocate the IOTHUB_SECURITY_INFO and shall fail if the allocation fails. **]**

**IOTHUB_DEV_AUTH_07_025: [** `iothub_device_auth_create` shall call the `concrete_device_auth_create` function associated with the `iothub_security_interface`. **]**

**IOTHUB_DEV_AUTH_07_003: [** If `concrete_device_auth_create` succeeds `iothub_device_auth_create` shall return a IOTHUB_SECURITY_INFO. **]**

**IOTHUB_DEV_AUTH_07_026: [** if `concrete_device_auth_create` fails `iothub_device_auth_create` shall return NULL. **]**

**IOTHUB_DEV_AUTH_07_030: [** `iothub_device_auth_create` shall store the interface_desc functions determined by the credintal type. **]**

**IOTHUB_DEV_AUTH_07_034: [** if any of the `iothub_security_interface` function are NULL `iothub_device_auth_create` shall return NULL. **]**

### iothub_device_auth_destroy

Frees any resources created by the iothub_device_auth module

```c
extern void iothub_device_auth_destroy(IOTHUB_SECURITY_INFO handle);
```

**IOTHUB_DEV_AUTH_07_004: [** `iothub_device_auth_destroy` shall free all resources associated with the IOTHUB_SECURITY_INFO handle **]**

**IOTHUB_DEV_AUTH_07_005: [** `iothub_device_auth_destroy` shall call the `concrete_device_auth_destroy` function function associated with the XDA_INTERFACE_DESCRIPTION. **]**

**IOTHUB_DEV_AUTH_07_006: [** If the argument handle is NULL, `iothub_device_auth_destroy` shall do nothing **]**

### iothub_device_auth_get_auth_type

Returns the type of device auth supported by the interface

```c
extern DEVICE_AUTH_TYPE iothub_device_auth_get_auth_type(IOTHUB_SECURITY_INFO handle);
```

**IOTHUB_DEV_AUTH_07_007: [** If the argument handle is NULL, `iothub_device_auth_get_auth_type` shall return AUTH_TYPE_UNKNOWN. **]**

**IOTHUB_DEV_AUTH_07_008: [** `iothub_device_auth_get_auth_type` shall call `concrete_device_auth_type` function associated with the XDA_INTERFACE_DESCRIPTION. **]**

**IOTHUB_DEV_AUTH_07_009: [** `iothub_device_auth_get_auth_type` shall return the DEVICE_AUTH_TYPE returned by the `concrete_device_auth_type` function. **]**

### iothub_device_auth_generate_credentials

Generates the credential used to validate the iothub connection.  free() must be called on the returned resource after use.

```c
extern void* iothub_device_auth_generate_credentials(IOTHUB_SECURITY_INFO handle, const DEVICE_AUTH_CREDENTIAL_INFO* dev_auth_cred);
```

**IOTHUB_DEV_AUTH_07_010: [** If the argument `handle` or `dev_auth_cred` is NULL, `iothub_device_auth_generate_credentials` shall return a NULL value. **]**

**IOTHUB_DEV_AUTH_07_011: [** `iothub_device_auth_generate_credentials` shall call `concrete_device_auth_credentials` function associated with the XDA_INTERFACE_DESCRIPTION and pass either the `DEVICE_AUTH_SAS_INFO` or the `DEVICE_AUTH_X509_INFO` depending on the cred_type. **]**

**IOTHUB_DEV_AUTH_07_035: [** For tpm type `iothub_device_auth_generate_credentials` shall call the `concrete_dev_auth_sign_data` function to hash the data. **]**

**IOTHUB_DEV_AUTH_07_012: [** For x509 type `iothub_device_auth_generate_credentials` shall call the `concrete_dev_auth_get_cert` and `concrete_dev_auth_get_alias_key` function. **]**

