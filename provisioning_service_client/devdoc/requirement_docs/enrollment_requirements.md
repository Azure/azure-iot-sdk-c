# Enrollment Requirements

## Overview

This module is used to manage data related to Provisioning Service enrollments

## Exposed API - Structures, Handles, Enums

```c
typedef struct INDIVIDUAL_ENROLLMENT* INDIVIDUAL_ENROLLMENT_HANDLE;
typedef struct ENROLLMENT_GROUP* ENROLLMENT_GROUP_HANDLE;

#define PROVISIONING_STATUS_VALUES \
        PROVISIONING_STATUS_NONE, \
        PROVISIONING_STATUS_ENABLED, \
        PROVISIONING_STATUS_DISABLED \

//Note: PROVISIONING_STATUS_NONE is invalid, indicating error
DEFINE_ENUM(PROVISIONING_STATUS, PROVISIONING_STATUS_VALUES);
```

## Exposed API - Operation Functions

```c
//Individual Enrollment
INDIVIDUAL_ENROLLMENT_HANDLE individualEnrollment_create(const char* reg_id, ATTESTATION_MECHANISM_HANDLE att_mech);
void individualEnrollment_destroy(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);

//Enrollment Group
ENROLLMENT_GROUP_HANDLE enrollmentGroup_create(const char* group_id, ATTESTATION_MECHANISM_HANDLE att_mech);
void enrollmentGroup_destroy(ENROLLMENT_GROUP_HANDLE enrollment);
```

## Exposed API - Accessor Functions

```c
//Individual Enrollment
ATTESTATION_MECHANISM_HANDLE individualEnrollment_getAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int individualEnrollment_setAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, ATTESTATION_MECHANISM_HANDLE att_mech);
TWIN_STATE_HANDLE individualEnrollment_getInitialTwin, INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int individualEnrollment_setInitialTwin(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, TWIN_STATE_HANDLE, twin);
DEVICE_REGISTRATION_STATE_HANDLE individualEnrollment_getDeviceRegistrationState(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
const char* individualEnrollment_getRegistrationId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
const char* individualEnrollment_getIotHubHostName(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
const char* individualEnrollment_getDeviceId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int individualEnrollment_setDeviceId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, const char* device_id);
const char* individualEnrollment_getEtag(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int individualEnrollment_setEtag(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, const char* etag);
PROVISIONING_STATUS individualEnrollment_getProvisioningStatus(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int individualEnrollment_setProvisioningStatus(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, PROVISIONING_STATUS prov_status);
const char* individualEnrollment_getCreatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
const char* individualEnrollment_getUpdatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);

//Enrollment Group
ATTESTATION_MECHANISM_HANDLE enrollmentGroup_getAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int enrollmentGroup_setAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, ATTESTATION_MECHANISM_HANDLE att_mech);
TWIN_STATE_HANDLE enrollmentGroup_getInitialTwinState, INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int enrollmentGroup_setInitialTwinState(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, INITIAL_TWIN_HANDLE, twin);
const char* enrollmentGroup_getGroupId(ENROLLMENT_GROUP_HANDLE enrollment);
const char* enrollmentGroup_getEtag(ENROLLMENT_GROUP_HANDLE enrollment);
int enrollmentGroup_setEtag(ENROLLMENT_GROUP_HANDLE enrollment, const char* etag);
PROVISIONING_STATUS enrollmentGroup_getProvisioningStatus(ENROLLMENT_GROUP_HANDLE enrollment);
int enrollmentGroup_setProvisioningStatus(ENROLLMENT_GROUP_HANDLE enrollment, PROVISIONING_STATUS prov_status);
const char* enrollmentGroup_getCreatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
const char* enrollmentGroup_getUpdatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```


## individualEnrollment_create

```c
INDIVIDUAL_ENROLLMENT_HANDLE individualEnrollment_create(const char* reg_id, ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_ENROLLMENT_22_001: [** If `reg_id` is `NULL`, `individualEnrollment_create` shall fail and return NULL **]**

**SRS_ENROLLMENT_22_002: [** If `att_mech` is `NULL`, `individualEnrollment_create` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_003: [** If `att_mech` has an invalid Attestation Type for Individual Enrollment, `individualEnrollment_create` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_004: [** If allocating memory for the new individual enrollment fails, `individualEnrollment_create` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_005: [** Upon success, `individualEnrollment_create` shall return a handle for the new individual enrollment **]**


## individualEnrollment_destroy

```c
void individualEnrollment_destroy(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_006: [** `individualEnrollment_destroy` shall free all memory contained within `enrollment` **]**


## enrollmentGroup_create

```c
ENROLLMENT_GROUP_HANDLE enrollmentGroup_create(const char* group_id, ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_ENROLLMENT_22_007: [** If `group_id` is `NULL`, `enrollmentGroup_create` shall fail and return NULL **]**

**SRS_ENROLLMENT_22_008: [** If `att_mech` is `NULL`, `enrollmentGroup_create` shall fail and return NULL **]**

**SRS_ENROLLMENT_22_009: [** If `att_mech` has an invalid Attestation Type for Enrollment Group, `enrollmentGroup_create` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_010: [** If allocating memory for the new enrollment group fails, `enrollmentGroup_create` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_011: [** Upon success, `enrollmentGroup_create` shall return a handle for the new enrollment group **]**


## enrollmentGroup_destroy

```c
void enrollmentGroup_destroy(ENROLLMENT_GROUP_HANDLE handle);
```

**SRS_ENROLLMENT_22_012: [** `enrollmentGroup_destroy` shall free all memory contained within `handle` **]**


## individualEnrollment_getAttestationMechanism

```c
ATTESTATION_MECHANISM_HANDLE individualEnrollment_getAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_013: [** If `enrollment` is `NULL`, `individualEnrollment_getAttestationMechanism` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_014: [** Otherwise, `individualEnrollment_getAttestationMechanism` shall return the attestation mechanism contained within `enrollment` **]**


## individualEnrollment_setAttestationMechanism

```c
int individualEnrollment_setAttestationMechanism(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_ENROLLMENT_22_015: [** If `enrollment` is `NULL`, `individualEnrollment_setAttestationMechanism` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_016: [** If `att_mech` is `NULL` or has an invalid type (e.g. X509 Signing Certificate), `individualEnrollment_setAttestationMechanism` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_017: [** The new attestation mechanism, `att_mech`, will be attached to the individual enrollment `enrollment`, and any existing attestation mechanism will have its memory freed. Then `individualEnrollment_setAttestationMechanism` shall return `0` **]**


## individualEnrollment_getInitialTwin

```c
INITIAL_TWIN_HANDLE individualEnrollment_getInitialTwin(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_018: [** If `enrollment` is `NULL`, `individualEnrollment_getInitialTwin` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_019: [** Otherwise, `individualEnrollment_getInitialTwin` shall return the initial twin contained within `enrollment` **]**


## individualEnrollment_setInitialTwin

```c
int individualEnrollment_setInitialTwin(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, INITIAL_TWIN_HANDLE twin);
```

**SRS_ENROLLMENT_22_020: [** If `enrollment` is `NULL`, `individualEnrollment_setInitialTwin` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_021: [** Upon success, the new initial twin, `twin`, will be attached to the individual enrollment `enrollment`, and any existing initial twin will have its memory freed. Then `individualEnrollment_setInitialTwin` shall return `0` **]**


## individualEnrollment_getDeviceRegistrationState

```c
DEVICE_REGISTRATION_STATE_HANDLE individualEnrollment_getDeviceRegistrationState(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_022: [** If `enrollment` is `NULL`, `individualEnrollment_getDeviceRegistrationState` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_023: [** Otherwise, `individualEnrollment_getDeviceRegistrationState` shall return the device registration state contained within `enrollment` **]**


## individualEnrollment_getRegistrationId

```c
const char* individualEnrollment_getRegistrationId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_024: [** If `enrollment` is `NULL`, `individualEnrollment_getRegistrationId` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_025: [** Otherwise, `individualEnrollment_getRegistrationId` shall return the registartion id of `enrollment` **]**


## individualEnrollment_getDeviceId

```c
const char* individualEnrollment_getDeviceId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_026: [** If `enrollment` is `NULL`, `individualEnrollment_getDeviceId` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_027: [** Otherwise, `individualEnrollment_getDeviceId` shall return the device id of `enrollment` **]**


## individualEnrollment_setDeviceId

```c
int individualEnrollment_setDeviceId(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, const char* device_id);
```

**SRS_ENROLLMENT_22_028: [** If `enrollment` is `NULL`, `individualEnrollment_setDeviceId` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_029: [** Otherwise, `device_id` will be set as the device id of `enrollment` and `individualEnrollment_setDeviceId` shall return `0` **]**


## individualEnrollment_getIotHubHostName

```c
const char* individualEnrollment_getIotHubHostName(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_030: [** If `enrollment` is `NULL`, `individualEnrollment_getIotHubHostName` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_031: [** Otherwise, `individualEnrollment_getIotHubHostName` shall return the IoT Hub hostname of `enrollment` **]**


## individualEnrollment_getEtag

```c
const char* individualEnrollment_getEtag(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_032: [** If `enrollment` is `NULL`, `individualEnrollment_getEtag` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_033: [** Otherwise, `individualEnrollment_getEtag` shall return the etag of `enrollment` **]**


## individualEnrollment_setEtag

```c
int individualEnrollment_setEtag(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, const char* etag);
```

**SRS_ENROLLMENT_22_034: [** If `enrollment` is `NULL`, `individualEnrollment_setEtag` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_035: [** Otherwise, `etag` will be set as the etag of `enrollment` and `individualEnrollment_setEtag` shall return `0` **]**


## individualEnrollment_getProvisioningStatus

```c
PROVISIONING_STATUS individualEnrollment_getProvisioningStatus(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_036: [** If `enrollment` is `NULL`, `individualEnrollment_getProvisioningStatus` shall fail and return `PROVISIONING_STATUS_NONE` **]**

**SRS_ENROLLMENT_22_037: [** Otherwise, `individualEnrollment_getProvisioningStatus` shall return the provisioning status of `enrollment` **]**


## individualEnrollment_setProvisioningStatus

```c
int individualEnrollment_setProvisioningStatus(INDIVIDUAL_ENROLLMENT_HANDLE enrollment, PROVISIONING_STATUS prov_status);
```

**SRS_ENROLLMENT_22_038: [** If `enrollment` is `NULL`, `individualEnrollment_setProvisioningStatus` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_039: [** If `prov_status` is invalid type (i.e. `PROVISIONING_STATUS_NONE`), `individualEnrollment_setProvisioningStatus` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_040: [** Otherwise, `prov_status` will be set as the provisioning status of `enrollment` and `individualEnrollment_setProvisioningStatus` shall return `0` **]**


## individualEnrollment_getCreatedDateTime

```c
const char* individualEnrollment_getCreatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_041: [** If `enrollment` is `NULL`, `individualEnrollment_getCreatedDateTime` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_042: [** Otherwise, `individualEnrollment_getCreatedDateTime` shall return the created date time of `enrollment` **]**


## individualEnrollment_getUpdatedDateTime

```c
const char* individualEnrollment_getUpdatedDateTime(INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_043: [** If `enrollment` is `NULL`, `individualEnrollment_getUpdatedDateTime` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_044: [** Otherwise, `individualEnrollment_getUpdatedDateTime` shall return the updated date time of `enrollment` **]**


## enrollmentGroup_getAttestationMechanism

```c
ATTESTATION_MECHANISM_HANDLE enrollmentGroup_getAttestationMechanism(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_045: [** If `enrollment` is `NULL`, `enrollmentGroup_getAttestationMechanism` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_046: [** Otherwise, `enrollmentGroup_getAttestationMechanism` shall return the attestation mechanism contained within `enrollment` **]**


## enrollmentGroup_setAttestationMechanism

```c
int enrollmentGroup_setAttestationMechanism(ENROLLMENT_GROUP_HANDLE enrollment, ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_ENROLLMENT_22_047: [** If `enrollment` is `NULL`, `enrollmentGroup_setAttestationMechanism` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_048: [** If `att_mech` is `NULL` or has an invalid type (e.g. X509 Signing Certificate), `enrollmentGroup_setAttestationMechanism` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_049: [** The new attestation mechanism, `att_mech`, will be attached to the individual enrollment `enrollment`, and any existing attestation mechanism will have its memory freed. Then `enrollmentGroup_setAttestationMechanism` shall return `0` **]**


## enrollmentGroup_getInitialTwin

```c
INITIAL_TWIN_HANDLE enrollmentGroup_getInitialTwin(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_050: [** If `enrollment` is `NULL`, `enrollmentGroup_getInitialTwin` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_051: [** Otherwise, `enrollmentGroup_getInitialTwin` shall return the initial twin contained within `enrollment` **]**


## enrollmentGroup_setInitialTwin

```c
int enrollmentGroup_setInitialTwin(ENROLLMENT_GROUP_HANDLE enrollment, INITIAL_TWIN_HANDLE twin);
```

**SRS_ENROLLMENT_22_052: [** If `enrollment` is `NULL`, `enrollmentGroup_setInitialTwin` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_053: [** Upon success, the new initial twin, `twin`, will be attached to the enrollment group `enrollment`, and any existing initial twin will have its memory freed. Then `enrollmentGroup_setInitialTwin` shall return `0` **]**


## enrollmentGroup_getGroupId

```c
const char* enrollmentGroup_getGroupId(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_054: [** If `enrollment` is `NULL`, `enrollmentGroup_getGroupId` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_055: [** Otherwise, `enrollmentGroup_getGroupId` shall return the group id of `enrollment` **]**


## enrollmentGroup_getIotHubHostName

```c
const char* enrollmentGroup_getIotHubHostName(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_056: [** If `enrollment` is `NULL`, `enrollmentGroup_getIotHubHostName` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_057: [** Otherwise, `enrollmentGroup_getIotHubHostName` shall return the IoT Hub hostname of `enrollment` **]**


## enrollmentGroup_getEtag

```c
const char* enrollmentGroup_getEtag(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_058: [** If `enrollment` is `NULL`, `enrollmentGroup_getEtag` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_059: [** Otherwise, `enrollmentGroup_getEtag` shall return the etag of `enrollment` **]**


## enrollmentGroup_setEtag

```c
int enrollmentGroup_setEtag(ENROLLMENT_GROUP_HANDLE enrollment, const char* etag);
```

**SRS_ENROLLMENT_22_060: [** If `enrollment` is `NULL`, `enrollmentGroup_setEtag` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_061: [** Otherwise, `etag` will be set as the etag of `enrollment` and `enrollmentGroup_setEtag` shall return `0` **]**


## enrollmentGroup_getProvisioningStatus

```c
PROVISIONING_STATUS enrollmentGroup_getProvisioningStatus(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_062: [** If `enrollment` is `NULL`, `enrollmentGroup_getProvisioningStatus` shall fail and return `PROVISIONING_STATUS_NONE` **]**

**SRS_ENROLLMENT_22_063: [** Otherwise, `enrollmentGroup_getProvisioningStatus` shall return the provisioning status of `enrollment` **]**


## enrollmentGroup_setProvisioningStatus

```c
int enrollmentGroup_setProvisioningStatus(ENROLLMENT_GROUP_HANDLE enrollment, PROVISIONING_STATUS prov_status);
```

**SRS_ENROLLMENT_22_064: [** If `enrollment` is `NULL`, `enrollmentGroup_setProvisioningStatus` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_065: [** If `prov_status` is invalid type (i.e. `PROVISIONING_STATUS_NONE`), `enrollmentGroup_setProvisioningStatus` shall fail and return a non-zero number **]**

**SRS_ENROLLMENT_22_066: [** Otherwise, `prov_status` will be set as the provisioning status of `enrollment` and `enrollmentGroup_setProvisioningStatus` shall return `0` **]**


## enrollmentGroup_getCreatedDateTime

```c
const char* enrollmentGroup_getCreatedDateTime(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_067: [** If `enrollment` is `NULL`, `enrollmentGroup_getCreatedDateTime` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_068: [** Otherwise, `enrollmentGroup_getCreatedDateTime` shall return the created date time of `enrollment` **]**


## enrollmentGroup_getUpdatedDateTime

```c
const char* enrollmentGroup_getUpdatedDateTime(ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_ENROLLMENT_22_069: [** If `enrollment` is `NULL`, `enrollmentGroup_getUpdatedDateTime` shall fail and return `NULL` **]**

**SRS_ENROLLMENT_22_070: [** Otherwise, `enrollmentGroup_getUpdatedDateTime` shall return the updated date time of `enrollment` **]**
