# Device Registration State Requirements

## Overview

This module is used to manage data related to the Device Registration State model, used with the Provisioning Service

## Exposed API

```c
typedef struct DEVICE_REGISTRATION_STATE_TAG* DEVICE_REGISTRATION_STATE_HANDLE;

typedef enum REGISTRATION_STATUS_VALUES {
    REGISTRATION_STATUS_ERROR,
    REGISTRATION_STATUS_ERROR,
    REGISTRATION_STATUS_UNASSIGNED,
    REGISTRATION_STATUS_ASSIGNING,
    REGISTRATION_STATUS_ASSIGNED,
    REGISTRATION_STATUS_FAILED,
    REGISTRATION_STATUS_DISABLED
} REGISTRATION_STATUS;

const char* deviceRegistrationState_getRegistrationId(DEVICE_REGISTRATION_STATE_HANDLE drs);
const char* deviceRegistrationState_getCreatedDateTime(DEVICE_REGISTRATION_STATE_HANDLE drs);
const char* deviceRegistrationState_getDeviceId(DEVICE_REGISTRATION_STATE_HANDLE drs);
REGISTRATION_STATUS deviceRegistrationState_getRegistrationStatus(DEVICE_REGISTRATION_STATE_HANDLE drs);
const char* deviceRegistrationState_getUpdatedDateTime(DEVICE_REGISTRATION_STATE_HANDLE drs);
int deviceRegistrationState_getErrorCode(DEVICE_REGISTRATION_STATE_HANDLE drs);
const char* deviceRegistrationState_getErrorMessage(DEVICE_REGISTRATION_STATE_HANDLE drs);
const char* deviceRegistrationState_getEtag(DEVICE_REGISTRATION_STATE_HANDLE drs);
```


## deviceRegistrationState_getRegistrationId

```c
const char* deviceRegistrationState_getRegistrationId(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_001: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getRegistrationId` shall fail and return `NULL` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_002: [** Otherwise, `deviceRegistrationState_getRegistrationId` shall return the registration id contained in `drs` **]**


## deviceRegistrationState_getCreatedDateTime

```c
const char* deviceRegistrationState_getCreatedDateTime(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_003: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getCreatedDateTime` shall fail and return `NULL` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_004: [** Otherwise, `deviceRegistrationState_getCreatedDateTime` shall return the created date time contained in `drs` **]**


## deviceRegistrationState_getDeviceId

```c
const char* deviceRegistrationState_getDeviceId(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_005: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getDeviceId` shall fail and return NULL **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_006: [** Otherwise, `deviceRegistrationState_getDeviceId` shall return the device id contained in `drs` **]**


## deviceRegistrationState_getRegistrationStatus

```c
REGISTRATION_STATUS deviceRegistrationState_getRegistrationStatus(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_007: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getRegistrationStatus` shall fail and return `REGISTRATION_STATUS_ERROR` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_008: [** Otherwise, `deviceRegistrationState_getRegistrationStatus` shall return the registration status contained in `drs` **]**


## deviceRegistrationState_getUpdatedDateTime

```c
const char* deviceRegistrationState_getUpdatedDateTime(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_009: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getUpdatedDateTime` shall fail and return `NULL` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_010: [** Otherwise, `deviceRegistrationState_getUpdatedDateTime` shall return the updated date time contained in `drs` **]**


## deviceRegistrationState_getErrorCode

```c
int deviceRegistrationState_getErrorCode(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_011: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getErrorCode` shall fail and return `-1` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_012: [** Otherwise, `deviceRegistrationState_getErrorCode` shall return the error code contained in `drs` **]**


## deviceRegistrationState_getErrorMessage

```c
const char* deviceRegistrationState_getErrorMessage(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_013: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getErrorMessage` shall fail and return `NULL` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_014: [** Otherwise, `deviceRegistrationState_getErrorMessage` shall return the error message contained in `drs` **]**


## deviceRegistrationState_getEtag

```c
const char* deviceRegistrationState_getEtag(DEVICE_REGISTRATION_STATE_HANDLE drs);
```

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_015: [** If the given handle, `drs` is `NULL`, `deviceRegistrationState_getEtag` shall fail and return `NULL` **]**

**SRS_PROV_DEVICE_REGISTRATION_STATE_22_016: [** Otherwise, `deviceRegistrationState_getEtag` shall return the etag contained in `drs` **]**
