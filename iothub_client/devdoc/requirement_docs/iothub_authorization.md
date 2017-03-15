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
    IOTHUB_CREDENTIAL_TYPE_TOKEN_FROM_USER

DEFINE_ENUM(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

MOCKABLE_FUNCTION(, IOTHUB_AUTHORIZATION_HANDLE, IoTHubClient_Auth_Create, const char*, device_key, const char*, device_id);
MOCKABLE_FUNCTION(, void, IoTHubClient_Auth_Destroy, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CREDENTIAL_TYPE, IoTHubClient_Auth_Get_Credential_Type, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, IoTHubClient_Auth_Get_ConnString, IOTHUB_AUTHORIZATION_HANDLE, handle, const char*, scope, size_t, expiry_time);
MOCKABLE_FUNCTION(, const char*, IoTHubClient_Auth_Get_DeviceId, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, bool, IoTHubClient_Auth_Is_SasToken_Valid, IOTHUB_AUTHORIZATION_HANDLE, handle);
```

## IoTHubClient_Auth_Create

```c
extern IOTHUB_AUTHORIZATION_HANDLE IoTHubClient_Auth_Create(const char* device_key, const char* device_id);
```

**SRS_IoTHub_Authorization_07_001: [**if `device_key` or `device_id` is NULL `IoTHubClient_Auth_Create`, shall return NULL.** ]**

**SRS_IoTHub_Authorization_07_002: [**`IoTHubClient_Auth_Create` shall allocate a `IOTHUB_AUTHORIZATION_HANDLE` that is needed for subsequent calls.** ]**

**SRS_IoTHub_Authorization_07_003: [** `IoTHubClient_Auth_Create` shall set the credential type to IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY. **]**

If successful `IoTHubClient_Auth_Create` shall return a `IOTHUB_AUTHORIZATION_HANDLE` value.

## IoTHubClient_Auth_Destroy

```c
extern void IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle);
```

if `handle` is NULL `IoTHubClient_Auth_Destroy` shall do nothing.

`IoTHubClient_Auth_Destroy` shall free all resources associated with the `IOTHUB_AUTHORIZATION_HANDLE` handle.

## IoTHubClient_Auth_Get_Connection_String

```c
extern IOTHUB_CREDENTIAL_TYPE IoTHub_Auth_Get_Credential_Type(IOTHUB_AUTHORIZATION_HANDLE handle);
```

if handle is NULL `IoTHub_Auth_Get_Credential_Type` shall return IOTHUB_CREDENTIAL_TYPE_UNKNOWN.

`IoTHub_Auth_Get_Credential_Type` shall return the credential type that is set upon creation.

## IoTHubClient_Auth_Get_ConnString

```c
extern char* IoTHubClient_Auth_Get_ConnString(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expire_time);
```

if handle or scope are NULL, `IoTHubClient_Auth_Get_ConnString` shall return NULL.

`IoTHubClient_Auth_Get_ConnString` shall construct the expiration time using the expire_time

`IoTHubClient_Auth_Get_ConnString` shall call SASToken_CreateString to construct the sas token.

On success `IoTHubClient_Auth_Get_ConnString` shall allocate and return the sas token in a char*.

## IoTHubClient_Auth_Get_DeviceId

```c
extern const char* IoTHubClient_Auth_Get_DeviceId(IOTHUB_AUTHORIZATION_HANDLE handle);
```

if handle is NULL, `IoTHubClient_Auth_Get_DeviceId` shall return NULL.

`IoTHubClient_Auth_Get_DeviceId` shall return the device_id specified upon creation.

## IoTHubClient_Auth_Is_SasToken_Valid

```c
extern bool IoTHubClient_Auth_Is_SasToken_Valid(IOTHUB_AUTHORIZATION_HANDLE handle);
```

if handle is NULL, `IoTHubClient_Auth_Is_SasToken_Valid` shall return false.

if credential type is not IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN `IoTHubClient_Auth_Is_SasToken_Valid` shall return true.

If the sas_token is NULL `IoTHubClient_Auth_Is_SasToken_Valid` shall return false.

otherwise `IoTHubClient_Auth_Is_SasToken_Valid` shall return the value returned by `SASToken_Validate`.