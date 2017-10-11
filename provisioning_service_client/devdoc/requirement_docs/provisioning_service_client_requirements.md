# Provisioning Service Client Requirements

## Overview

This module is used to perform CRUD operations on the device enrollment records and device registration statuses stored on the Provisioning Service

## Exposed API

```c
PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_create_from_connection_string(const char* conn_string);
void prov_sc_destroy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client);
int prov_sc_create_or_update_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const INDIVIDUAL_ENROLLMENT* enrollment);
int prov_sc_delete_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
int prov_sc_get_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, INDIVIDUAL_ENROLLMENT* enrollment);
int prov_sc_delete_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
int prov_sc_get_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, DEVICE_REGISTRATION_STATUS* reg_status);
int prov_sc_create_or_update_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const ENROLLMENT_GROUP* enrollment_group);
int prov_sc_delete_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
int prov_sc_get_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, ENROLLMENT_GROUP* enrollment_group);
int prov_sc_get_group_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
```

### prov_sc_create_from_connection_string

```c
PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_create_from_connection_string(const char* conn_string);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_001: [** If `conn_string` is NULL `prov_sc_create_from_connection_string` shall fail and return NULL **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_002: [** `conn_string` shall be parsed and its information will populate a new `PROVISIONING_SERVICE_CLIENT_HANDLE` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_003: [** If the new `PROVISIONING_SERVICE_CLIENT_HANDLE` is not correctly populated `prov_sc_create_from_connection_string` shall fail and return NULL **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_004: [** Upon successful creation of the new `PROVISIONING_SERVICE_CLIENT_HANDLE`, `prov_sc_create_from_connection_string` shall return it **]**


### prov_sc_destroy

```c
void prov_sc_destroy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_005: [** `prov_sc_destroy` shall free all the memory contained inside `prov_client` **]**


### Iprov_sc_create_or_update_individual_enrollment

```c
int prov_sc_create_or_update_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const INDIVIDUAL_ENROLLMENT* enrollment);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_006: [** If `prov_client` or `enrollment` are NULL, `Iprov_sc_create_or_update_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_007: [** A 'PUT' REST call shall be issued to create/update the enrollment record of a device on the Provisioning Service, using data contained in `enrollment` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_008: [** If the 'PUT' REST call fails, `Iprov_sc_create_or_update_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_009: [** Upon a successful create or update, `Iprov_sc_create_or_update_individual_enrollment` shall return 0 **]**


### prov_sc_delete_individual_enrollment

```c
int prov_sc_delete_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_010: [** If `prov_client` or `id` are NULL, `prov_sc_delete_individual_enrollment` shall return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_011: [** A 'DELETE' REST call shall be issued to delete the enrollment record of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_012: [** If the 'DELETE' REST call fails, `prov_sc_delete_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_013: [** Upon a successful delete, `prov_sc_delete_individual_enrollment` shall return 0 **]**


### prov_sc_get_individual_enrollment

```c
int prov_sc_get_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, INDIVIDUAL_ENROLLMENT* enrollment);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_014: [** If `prov_client` or `id` are NULL, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_015: [** A 'GET' REST call shall be issued to retrieve the enrollment record of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_016: [** If the 'GET' REST call fails, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_017: [** The data from the retrieved device enrollment record shall populate `enrollment` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_018: [** If populating `enrollment` with retrieved data fails, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_019: [** Upon successful population of `enrollment` with the retrieved device enrollment record data, `prov_sc_get_individual_enrollment` shall return 0 **]** 


### prov_sc_delete_device_registration_status

```c
int prov_sc_delete_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_020: [** If `prov_client` or `id` are NULL, `prov_sc_delete_device_registration_status` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_021: [** A 'DELETE' REST call shall be issued to delete the registration status of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_022: [** If the 'DELETE' REST call fails, `prov_sc_delete_device_registration_status` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_023: [** Upon a successful delete, `prov_sc_delete_device_registration_status` shall return 0 **]**


### prov_sc_get_device_registration_status

```c
int prov_sc_get_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, DEVICE_REGISTRATION_STATUS* reg_status);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_024: [** If `prov_client` or `id` are NULL, `prov_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_025: [** A 'GET' REST call shall be issued to retrieve the registration status of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_026: [** If the 'GET' REST call fails, `prov_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_027: [** The data from the retrieved device registration status shall populate `reg_status` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_028: [** If populating `reg_status` with retrieved data fails, `prov_sc_get_device_registration_status` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_029: [** Upon successful population of `reg_status` with the retrieved device registration status data, `prov_sc_get_device_registration_status` shall return 0 **]**


### prov_sc_create_or_update_enrollment_group

```c
int prov_sc_create_or_update_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const ENROLLMENT_GROUP* enrollment_group);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_030: [** If `prov_client`, `id`, or `enrollment_group` are NULL, `prov_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_031: [** A 'PUT' REST call shall be issued to create/update the device enrollment group with ID `id` on the Provisioning Service, using data contained in `enrollment_group` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_032: [** If the 'PUT' REST call fails, `prov_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_033: [** Upon a successful create or update, `prov_sc_create_or_update_enrollment_group` shall return 0 **]**


### prov_sc_delete_enrollment_group

```c
int prov_sc_delete_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_034: [** If `prov_client` or `id` are NULL, `prov_sc_delete_enrollment_group` shall return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_035: [** A 'DELETE' REST call shall be issued to delete the device enrollment group with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_036: [** If the 'DELETE' REST call fails, `prov_sc_delete_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_037: [** Upon a successful delete, `prov_sc_delete_enrollment_group` shall return 0 **]**


### prov_sc_get_enrollment_group

```c
int prov_sc_get_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, ENROLLMENT_GROUP* enrollment_group);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_038: [** If `prov_client` or `id` are NULL, `prov_sc_get_enrollment_group` shall return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_039: [** A 'GET' REST call shall be issued to retrieve the device enrollment group with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_040: [** If the 'GET' REST call fails, `prov_sc_get_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_041: [** The data from the retrieved device enrollment group shall populate `enrollment_group` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_042: [** If populating `enrollment_group` with retrieved data fails, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_043: [** Upon successful population of `enrollment_group` with the retrieved device enrollment group data, `prov_sc_get_individual_enrollment` shall return 0 **]** 