# IoTHub_Authorization Requirements

## Overview

IoTHub_Authorization is a module to consolidate the authorization method of the iothub.

Exposed API

```c
typedef struct IOTHUB_AUTHORIZATION_DATA_TAG* IOTHUB_AUTHORIZATION_HANDLE;

#define IOTHUB_CREDENTIAL_TYPE_VALUES       \
    IOTHUB_CREDENTIAL_TYPE_UNKNOWN,         \
    IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY,      \
    IOTHUB_CREDENTIAL_TYPE_X509,            \
    IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN

DEFINE_ENUM(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

MOCKABLE_FUNCTION(, IOTHUB_AUTHORIZATION_HANDLE, IoTHubClient_Auth_Create, const char*, device_key, const char*, device_id, const char*, device_sas_token);
MOCKABLE_FUNCTION(, void, IoTHubClient_Auth_Destroy, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CREDENTIAL_TYPE, IoTHubClient_Auth_Get_Credential_Type, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, IoTHubClient_Auth_Get_SasToken, IOTHUB_AUTHORIZATION_HANDLE, handle, const char*, scope, size_t, expiry_time_relative_seconds);
MOCKABLE_FUNCTION(, const char*, IoTHubClient_Auth_Get_DeviceId, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, bool, IoTHubClient_Auth_Is_SasToken_Valid, IOTHUB_AUTHORIZATION_HANDLE, handle);
```

## IoTHubClient_Auth_Create

```c
extern IOTHUB_AUTHORIZATION_HANDLE IoTHubClient_Auth_Create(const char* device_key, const char* device_id, const char* device_sas_token);
```

**SRS_IoTHub_Authorization_07_001: [**if `device_id` is NULL `IoTHubClient_Auth_Create`, shall return NULL.**]**

**SRS_IoTHub_Authorization_07_002: [**`IoTHubClient_Auth_Create` shall allocate a `IOTHUB_AUTHORIZATION_HANDLE` that is needed for subsequent calls. **]**

**SRS_IoTHub_Authorization_07_003: [** `IoTHubClient_Auth_Create` shall set the credential type to IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY if the device_sas_token is NULL. **]**

**SRS_IoTHub_Authorization_07_020: [** else `IoTHubClient_Auth_Create` shall set the credential type to IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN. **]**

**SRS_IoTHub_Authorization_07_024: [** if device_sas_token and device_key are NULL `IoTHubClient_Auth_Create` shall set the credential type to IOTHUB_CREDENTIAL_TYPE_UNKNOWN. **]**

**SRS_IoTHub_Authorization_07_004: [** If successful `IoTHubClient_Auth_Create` shall return a `IOTHUB_AUTHORIZATION_HANDLE` value. **]**

**SRS_IoTHub_Authorization_07_019: [** On error `IoTHubClient_Auth_Create` shall return NULL. **]**

## IoTHubClient_Auth_Destroy

```c
extern void IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle);
```

**SRS_IoTHub_Authorization_07_005: [** if `handle` is NULL `IoTHubClient_Auth_Destroy` shall do nothing. **]**

**SRS_IoTHub_Authorization_07_006: [** `IoTHubClient_Auth_Destroy` shall free all resources associated with the `IOTHUB_AUTHORIZATION_HANDLE` handle. **]**

## IoTHub_Auth_Get_Credential_Type

```c
extern IOTHUB_CREDENTIAL_TYPE IoTHub_Auth_Get_Credential_Type(IOTHUB_AUTHORIZATION_HANDLE handle);
```

**SRS_IoTHub_Authorization_07_007: [** if `handle` is NULL `IoTHub_Auth_Get_Credential_Type` shall return `IOTHUB_CREDENTIAL_TYPE_UNKNOWN`. **]**

**SRS_IoTHub_Authorization_07_008: [** `IoTHub_Auth_Get_Credential_Type` shall return the credential type that is associated with the given handle. **]**

## IoTHubClient_Auth_Get_SasToken

```c
extern char* IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expiry_time_relative_seconds);
```

**SRS_IoTHub_Authorization_07_009: [** if `handle` or `scope` are NULL, `IoTHubClient_Auth_Get_SasToken` shall return NULL. **]**

**SRS_IoTHub_Authorization_07_010: [** `IoTHubClient_Auth_Get_SasToken` shall construct the expiration time using the expiry_time_relative_seconds added to epoch time. **]**

**SRS_IoTHub_Authorization_07_011: [** `IoTHubClient_Auth_Get_SasToken` shall call SASToken_CreateString to construct the sas token. **]**

**SRS_IoTHub_Authorization_07_020: [** If any error is encountered `IoTHubClient_Auth_Get_SasToken` shall return NULL. **]**

**SRS_IoTHub_Authorization_07_012: [** On success `IoTHubClient_Auth_Get_SasToken` shall allocate and return the sas token in a char*. **]**

**SRS_IoTHub_Authorization_07_021: [** If the device_sas_token is NOT NULL `IoTHubClient_Auth_Get_SasToken` shall return a copy of the device_sas_token. **]**

## IoTHubClient_Auth_Get_DeviceId

```c
extern const char* IoTHubClient_Auth_Get_DeviceId(IOTHUB_AUTHORIZATION_HANDLE handle);
```

**SRS_IoTHub_Authorization_07_013: [** if `handle` is NULL, `IoTHubClient_Auth_Get_DeviceId` shall return NULL. **]**

**SRS_IoTHub_Authorization_07_014: [** `IoTHubClient_Auth_Get_DeviceId` shall return the device_id associated with `handle`. **]**

## IoTHubClient_Auth_Get_DeviceKey

```c
extern const char* IoTHubClient_Auth_Get_DeviceKey(IOTHUB_AUTHORIZATION_HANDLE handle);
```

**SRS_IoTHub_Authorization_07_022: [** if `handle` is NULL, `IoTHubClient_Auth_Get_DeviceKey` shall return NULL. **]**

**SRS_IoTHub_Authorization_07_023: [** `IoTHubClient_Auth_Get_DeviceKey` shall return the device_Key associated with `handle`. **]**

## IoTHubClient_Auth_Is_SasToken_Valid

```c
extern bool IoTHubClient_Auth_Is_SasToken_Valid(IOTHUB_AUTHORIZATION_HANDLE handle);
```

**SRS_IoTHub_Authorization_07_015: [** if `handle` is NULL, `IoTHubClient_Auth_Is_SasToken_Valid` shall return false. **]**

**SRS_IoTHub_Authorization_07_016: [** if credential type is not equal to IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN `IoTHubClient_Auth_Is_SasToken_Valid` shall return true. **]**

**SRS_IoTHub_Authorization_07_017: [** If the sas_token is NULL `IoTHubClient_Auth_Is_SasToken_Valid` shall return false. **]**

**SRS_IoTHub_Authorization_07_018: [** otherwise `IoTHubClient_Auth_Is_SasToken_Valid` shall return the value returned by `SASToken_Validate`. **]**