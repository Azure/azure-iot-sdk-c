# Provisioning Service Client Requirements

## Overview

This module is used to perform CRUD operations on the device enrollment records and device registration statuses stored on the Provisioning Service

## Exposed API

```c
PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_create_from_connection_string(const char* conn_string);
void prov_sc_destroy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client);
void prov_sc_set_trace(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, TRACING_STATUS status);
int prov_sc_set_certificate(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* certificate);
int prov_sc_set_proxy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, HTTP_PROXY_OPTIONS* proxy_options);

int prov_sc_create_or_update_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr);
int prov_sc_delete_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
int prov_sc_delete_individual_enrollment_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* reg_id, const char* etag);
int prov_sc_get_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr)
int prov_sc_create_or_update_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE* enrollment_ptr)
int prov_sc_delete_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE enrollment)
int prov_sc_delete_enrollment_group_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, const char* etag)
int prov_sc_get_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, ENROLLMENT_GROUP_HANDLE* enrollment_ptr)
int prov_sc_delete_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id)
int prov_sc_get_device_registration_status(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, DEVICE_REGISTRATION_STATE_HANDLE* reg_state_ptr)
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


### prov_sc_set_trace

```c
void prov_sc_set_trace(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, TRACING_STATUS status);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_068: [** If `prov_client` is NULL, `prov_sc_trace_on` shall do nothing **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_069: [** HTTP tracing for communications using `prov_client` will be set to `status` **]**


### prov_sc_set_certificate

```c
int prov_sc_set_certificate(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* certificate);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_058: [** If `prov_client` is NULL, `prov_sc_set_certificate` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_059: [** If `certificate` is NULL, any previously set trusted certificate will be cleared **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_060: [** If `certificate` is not NULL, it will be set as the trusted certificate for `prov_client` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_061: [** If allocating the trusted certificate fails, `prov_sc_set_certificate` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_062: [** Upon success, `prov_sc_set_certficiate` shall return 0 **]**


### prov_sc_set_proxy

```c
int prov_sc_set_proxy(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, HTTP_PROXY_OPTIONS* proxy_options);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_063: [** If `prov_client` or `proxy_options` are NULL, `prov_sc_set_proxy` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_064: [** If the host address is NULL in `proxy_options`, `prov_sc_set_proxy` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_065: [** If only the username, or only the password is NULL in `proxy_options`, `prov_sc_set_proxy` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_066: [** The proxy settings specified in `proxy_options` will be set for use by `prov_client` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_067: [** Upon success, `prov_sc_set_proxy` shall return 0 **]**


### prov_sc_create_or_update_individual_enrollment

```c
int prov_sc_create_or_update_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, const INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_006: [** If `prov_client` or `enrollment_ptr` are NULL, `prov_sc_create_or_update_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_007: [** A 'PUT' REST call shall be issued to create/update the enrollment record of a device on the Provisioning Service, using data contained in `enrollment_ptr` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_008: [** If the 'PUT' REST call fails, `prov_sc_create_or_update_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_044: [** The data in `enrollment_ptr` will be updated to reflect new information added by the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_045: [** If receiving the response from the Provisioning Service fails, `prov_sc_create_or_update_individual_enrollment` shall fail and return a non-zero value. **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_009: [** Upon a successful create or update, `prov_sc_create_or_update_individual_enrollment` shall return 0 **]**


### prov_sc_delete_individual_enrollment

```c
int prov_sc_delete_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, INDIVIDUAL_ENROLLMENT_HANDLE enrollment);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_010: [** If `prov_client` or `enrollment` are NULL, `prov_sc_delete_individual_enrollment` shall fail and return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_011: [** A 'DELETE' REST call shall be issued to delete the individual enrollment record on the Provisioning Service that matches `enrollment` based on registration id and etag **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_012: [** If the 'DELETE' REST call fails, `prov_sc_delete_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_013: [** Upon a successful delete, `prov_sc_delete_individual_enrollment` shall return 0 **]**


### prov_sc_delete_individual_enrollment_by_param

```c
int prov_sc_delete_individual_enrollment_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* reg_id, const char* etag);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_046: [** If `prov_client` or `reg_id` are NULL, `prov_sc_delete_individual_enrollment_by_param` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_047: [** If `etag` is given as NULL, it shall be ignored **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_048: [** A 'DELETE' REST call shall be issued to delete the individual enrollment record of a device with ID `reg_id`, and optionally, `etag` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_049: [** If the 'DELETE' REST call fails, `prov_sc_delete_individual_enrollment_by_param` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_050: [** Upon a successful delete, `prov_sc_delete_individual_enrollment_by_param` shall return 0 **]**


### prov_sc_get_individual_enrollment

```c
int prov_sc_get_individual_enrollment(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* reg_id, INDIVIDUAL_ENROLLMENT_HANDLE* enrollment_ptr);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_014: [** If `prov_client`, `reg_id` or `enrollment_ptr` are NULL, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_015: [** A 'GET' REST call shall be issued to retrieve the enrollment record of a device with ID `reg_id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_016: [** If the 'GET' REST call fails, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_017: [** The data from the retrieved device enrollment record shall populate `enrollment_ptr` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_018: [** If populating `enrollment_ptr` with retrieved data fails, `prov_sc_get_individual_enrollment` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_019: [** Upon successful population of `enrollment_ptr` with the retrieved device enrollment record data, `prov_sc_get_individual_enrollment` shall return 0 **]** 


### prov_sc_create_or_update_enrollment_group

```c
int prov_sc_create_or_update_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE* enrollment_ptr);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_030: [** If `prov_client` or `enrollment_ptr` are NULL, `prov_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_031: [** A 'PUT' REST call shall be issued to create/update the device enrollment group on the Provisioning Service, using data contained in `enrollment_ptr` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_032: [** If the 'PUT' REST call fails, `prov_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_051: [** The data in `enrollment_ptr` will be updated to reflect new information added by the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_052: [** If receiving the response from the Provisioning Service fails, `prov_sc_create_or_update_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_033: [** Upon a successful create or update, `prov_sc_create_or_update_enrollment_group` shall return 0 **]**


### prov_sc_delete_enrollment_group

```c
int prov_sc_delete_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, ENROLLMENT_GROUP_HANDLE enrollment);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_034: [** If `prov_client` or `id` are NULL, `prov_sc_delete_enrollment_group` shall return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_035: [** A 'DELETE' REST call shall be issued to delete the device enrollment group that matches `enrollment` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_036: [** If the 'DELETE' REST call fails, `prov_sc_delete_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_037: [** Upon a successful delete, `prov_sc_delete_enrollment_group` shall return 0 **]**


### prov_sc_delete_enrollment_group_by_param

```c
int prov_sc_delete_enrollment_group_by_param(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, const char* etag);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_053: [** If `prov_client` or `group_id` are NULL, `prov_sc_delete_enrollment_group_by_param` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_054: [** If `etag` is given as NULL, it shall be ignored **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_055: [** A 'DELETE' REST call shall be issued to delete the enrollment group record with ID `group_id`, and optionally, `etag` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_056: [** If the 'DELETE' REST call fails, `prov_sc_delete_enrollment_group_by_param` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_057: [** Upon a successful delete, `prov_sc_delete_enrollment_group_by_param` shall return 0 **]**


### prov_sc_get_enrollment_group

```c
int prov_sc_get_enrollment_group(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* group_id, ENROLLMENT_GROUP_HANDLE* enrollment_ptr);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_038: [** If `prov_client`, `group_id` or `enrollment_ptr` are NULL, `prov_sc_get_enrollment_group` shall return return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_039: [** A 'GET' REST call shall be issued to retrieve the device enrollment group with ID `group_id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_040: [** If the 'GET' REST call fails, `prov_sc_get_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_041: [** The data from the retrieved device enrollment group shall populate `enrollment_ptr` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_042: [** If populating `enrollment_ptr` with retrieved data fails, `prov_sc_get_enrollment_group` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_043: [** Upon successful population of `enrollment_ptr` with the retrieved device enrollment group data, `prov_sc_get_enrollment_group` shall return 0 **]** 


### prov_sc_delete_device_registration_state

```c
int prov_sc_delete_device_registration_state(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_020: [** If `prov_client` or `id` are NULL, `prov_sc_delete_device_registration_state` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_021: [** A 'DELETE' REST call shall be issued to delete the registration state of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_022: [** If the 'DELETE' REST call fails, `prov_sc_delete_device_registration_state` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_023: [** Upon a successful delete, `prov_sc_delete_device_registration_state` shall return 0 **]**


### prov_sc_get_device_registration_state

```c
int prov_sc_get_device_registration_state(PROVISIONING_SERVICE_CLIENT_HANDLE prov_client, const char* id, DEVICE_REGISTRATION_STATUS* reg_state);
```

**SRS_PROVISIONING_SERVICE_CLIENT_22_024: [** If `prov_client` or `id` are NULL, `prov_sc_get_device_registration_state` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_025: [** A 'GET' REST call shall be issued to retrieve the registration state of a device with ID `id` from the Provisioning Service **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_026: [** If the 'GET' REST call fails, `prov_sc_get_device_registration_state` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_027: [** The data from the retrieved device registration state shall populate `reg_state` **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_028: [** If populating `reg_state` with retrieved data fails, `prov_sc_get_device_registration_state` shall fail and return a non-zero value **]**

**SRS_PROVISIONING_SERVICE_CLIENT_22_029: [** Upon successful population of `reg_state` with the retrieved device registration state data, `prov_sc_get_device_registration_state` shall return 0 **]**