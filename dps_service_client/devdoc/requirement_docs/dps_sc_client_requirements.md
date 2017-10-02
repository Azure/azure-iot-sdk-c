# DPS_SC Client Requirements

## Overview

This module is used to perform CRUD operations on the device enrollment records and device registration statuses stored on the DPS Service

## Exposed API

```c
DPS_SC_HANDLE dps_sc_create_from_connection_string(const char* conn_string);

void dps_sc_destroy(DPS_SC_HANDLE handle);
int dps_sc_create_or_update_enrollment(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT* enrollment);
int dps_sc_delete_enrollment(DPS_SC_HANDLE handle, const char* id);
int dps_sc_get_enrollment(DPS_SC_HANDLE handle, const char* id, ENROLLMENT* enrollment);
int dps_sc_delete_device_registration_status(DPS_SC_HANDLE handle, const char* id);
int dps_sc_get_device_registration_status(DPS_SC_HANDLE handle, const char* id, DEVICE_REGISTRATION_STATUS* reg_status);
int dps_sc_create_or_update_enrollment_group(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT_GROUP* enrollment_group);
int dps_sc_delete_enrollment_group(DPS_SC_HANDLE handle, const char* id);
int dps_sc_get_enrollment_group(DPS_SC_HANDLE handle, const char* id, ENROLLMENT_GROUP* enrollment_group);
int dps_sc_get_group_device_registration_status(DPS_SC_HANDLE handle, const char* id);
```

### dps_sc_create_from_connection_string

```c
DPS_SC_HANDLE dps_sc_create_from_connection_string(const char* conn_string);
```

**SRS_DPS_SC_CLIENT_22_001: [** If `conn_string` is NULL `dps_sc_create_from_connection_string` shall fail and return NULL **]**

**SRS_DPS_SC_CLIENT_22_002: [** `conn_string` shall be parsed and its information will populate a new `DPS_SC_HANDLE` **]**

**SRS_DPS_SC_CLIENT_22_003: [** If the new `DPS_SC_HANDLE` is not correctly populated `dps_sc_create_from_connection_string` shall fail and return NULL **]**

**SRS_DPS_SC_CLIENT_22_004: [** Upon successful creation of the new `DPS_SC_HANDLE`, `dps_sc_create_from_connection_string` shall return it **]**


### dps_sc_destroy

```c
void dps_sc_destroy(DPS_SC_HANDLE handle);
```

**SRS_DPS_SC_CLIENT_22_005: [** `dps_sc_destroy` shall free all the memory contained inside `handle` **]**


### dps_sc_create_or_update_enrollment

```c
int dps_sc_create_or_update_enrollment(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT* enrollment);
```

**SRS_DPS_SC_CLIENT_22_006: [** If `handle`, `id`, or `enrollment` are NULL, `dps_sc_create_or_update_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_007: [** A 'PUT' REST call shall be issued to create/update the enrollment record of a device with ID `id` on the DPS Service, using data contained in `enrollment` **]**

**SRS_DPS_SC_CLIENT_22_008: [** If the 'PUT' REST call fails, `dps_sc_create_or_update_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_009: [** Upon a successful create or update, `dps_sc_create_or_update_enrollment` shall return 0 **]**


### dps_sc_delete_enrollment

```c
int dps_sc_delete_enrollment(DPS_SC_HANDLE handle, const char* id);
```

**SRS_DPS_SC_CLIENT_22_010: [** If `handle` or `id` are NULL, `dps_sc_delete_enrollment` shall return return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_011: [** A 'DELETE' REST call shall be issued to delete the enrollment record of a device with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_012: [** If the 'DELETE' REST call fails, `dps_sc_delete_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_013: [** Upon a successful delete, `dps_sc_delete_enrollment` shall return 0 **]**


### dps_sc_get_enrollment

```c
int dps_sc_get_enrollment(DPS_SC_HANDLE handle, const char* id, ENROLLMENT* enrollment);
```

**SRS_DPS_SC_CLIENT_22_014: [** If `handle` or `id` are NULL, `dps_sc_get_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_015: [** A 'GET' REST call shall be issued to retrieve the enrollment record of a device with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_016: [** If the 'GET' REST call fails, `dps_sc_get_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_017: [** The data from the retrieved device enrollment record shall populate `enrollment` **]**

**SRS_DPS_SC_CLIENT_22_018: [** If populating `enrollment` with retrieved data fails, `dps_sc_get_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_019: [** Upon successful population of `enrollment` with the retrieved device enrollment record data, `dps_sc_get_enrollment` shall return 0 **]** 


### dps_sc_delete_device_registration_status

```c
int dps_sc_delete_device_registration_status(DPS_SC_HANDLE handle, const char* id);
```

**SRS_DPS_SC_CLIENT_22_020: [** If `handle` or `id` are NULL, `dps_sc_delete_device_registration_status` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_021: [** A 'DELETE' REST call shall be issued to delete the registration status of a device with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_022: [** If the 'DELETE' REST call fails, `dps_sc_delete_device_registration_status` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_023: [** Upon a successful delete, `dps_sc_delete_device_registration_status` shall return 0 **]**


### dps_sc_get_device_registration_status

```c
int dps_sc_get_device_registration_status(DPS_SC_HANDLE handle, const char* id, DEVICE_REGISTRATION_STATUS* reg_status);
```

**SRS_DPS_SC_CLIENT_22_024: [** If `handle` or `id` are NULL, `dps_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_025: [** A 'GET' REST call shall be issued to retrieve the registration status of a device with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_026: [** If the 'GET' REST call fails, `dps_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_027: [** The data from the retrieved device registration status shall populate `reg_status` **]**

**SRS_DPS_SC_CLIENT_22_028: [** If populating `reg_status` with retrieved data fails, `dps_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_029: [** Upon successful population of `reg_status` with the retrieved device registration status data, `dps_sc_get_device_registration_status` shall return 0 **]**


### dps_sc_create_or_update_enrollment_group

```c
int dps_sc_create_or_update_enrollment_group(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT_GROUP* enrollment_group);
```

**SRS_DPS_SC_CLIENT_22_030: [** If `handle`, `id`, or `enrollment_group` are NULL, `dps_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_031: [** A 'PUT' REST call shall be issued to create/update the device enrollment group with ID `id` on the DPS Service, using data contained in `enrollment_group` **]**

**SRS_DPS_SC_CLIENT_22_032: [** If the 'PUT' REST call fails, `dps_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_033: [** Upon a successful create or update, `dps_sc_create_or_update_enrollment_group` shall return 0 **]**


### dps_sc_delete_enrollment_group

```c
int dps_sc_delete_enrollment_group(DPS_SC_HANDLE handle, const char* id);
```

**SRS_DPS_SC_CLIENT_22_034: [** If `handle` or `id` are NULL, `dps_sc_delete_enrollment_group` shall return return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_035: [** A 'DELETE' REST call shall be issued to delete the device enrollment group with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_036: [** If the 'DELETE' REST call fails, `dps_sc_delete_enrollment_group` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_037: [** Upon a successful delete, `dps_sc_delete_enrollment_group` shall return 0 **]**


### dps_sc_get_enrollment_group

```c
int dps_sc_get_enrollment_group(DPS_SC_HANDLE handle, const char* id, ENROLLMENT_GROUP* enrollment_group);
```

**SRS_DPS_SC_CLIENT_22_038: [** If `handle` or `id` are NULL, `dps_sc_get_enrollment_group` shall return return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_039: [** A 'GET' REST call shall be issued to retrieve the device enrollment group with ID `id` from the DPS Service **]**

**SRS_DPS_SC_CLIENT_22_040: [** If the 'GET' REST call fails, `dps_sc_get_enrollment_group` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_041: [** The data from the retrieved device enrollment group shall populate `enrollment_group` **]**

**SRS_DPS_SC_CLIENT_22_042: [** If populating `enrollment_group` with retrieved data fails, `dps_sc_get_enrollment` shall fail and return a non-zero value **]**

**SRS_DPS_SC_CLIENT_22_043: [** Upon successful population of `enrollment_group` with the retrieved device enrollment group data, `dps_sc_get_enrollment` shall return 0 **]** 