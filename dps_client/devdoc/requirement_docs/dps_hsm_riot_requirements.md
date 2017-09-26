# dps_hsm_riot Requirements

================================

## Overview

dps_hsm_riot module implements the interface over a riot module.

## Dependencies

MSR RIoT Module

## Exposed API

```c
MOCKABLE_FUNCTION(, DPS_SECURE_DEVICE_HANDLE, dps_hsm_riot_create);
MOCKABLE_FUNCTION(, void, dps_hsm_riot_destroy, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_certificate, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_alias_key, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_signer_cert, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_root_cert, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_root_key, DPS_SECURE_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, dps_hsm_riot_get_common_name, DPS_SECURE_DEVICE_HANDLE, handle);

MOCKABLE_FUNCTION(, const SEC_RIOT_INTERFACE*, dps_hsm_riot_interface);
MOCKABLE_FUNCTION(, int, initialize_riot_system);
MOCKABLE_FUNCTION(, void, deinitialize_riot_system);

MOCKABLE_FUNCTION(, char*, dps_hsm_riot_create_leaf_cert, DPS_SECURE_DEVICE_HANDLE, handle, const char*, common_name);

```

### dps_hsm_riot_create

```c
extern DPS_SECURE_DEVICE_HANDLE dps_hsm_riot_create();
```

**SRS_DPS_HSM_RIOT_07_001: [** On success `dps_hsm_riot_create` shall allocate a new instance of the device auth interface. **]**

**SRS_DPS_HSM_RIOT_07_002: [** `dps_hsm_riot_create` shall call into the RIot code to sign the RIoT certificate. **]**

**SRS_DPS_HSM_RIOT_07_003: [** `dps_hsm_riot_create` shall cache the device id public value from the RIoT module. **]**

**SRS_DPS_HSM_RIOT_07_004: [** `dps_hsm_riot_create` shall cache the alias key pair value from the RIoT module. **]**

**SRS_DPS_HSM_RIOT_07_005: [** `dps_hsm_riot_create` shall create the Signer regions of the alias key certificate. **]**

**SRS_DPS_HSM_RIOT_07_006: [** If any failure is encountered `dps_hsm_riot_create` shall return NULL **]**


### dps_hsm_riot_destroy

```c
extern void dps_hsm_riot_destroy(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_007: [** if handle is NULL, `dps_hsm_riot_destroy` shall do nothing. **]**

**SRS_DPS_HSM_RIOT_07_008: [** `dps_hsm_riot_destroy` shall free the DPS_SECURE_DEVICE_HANDLE instance. **]**

**SRS_DPS_HSM_RIOT_07_009: [** `dps_hsm_riot_destroy` shall free all resources allocated in this module. **]**


### dps_hsm_riot_get_certificate

```c
extern char* dps_hsm_riot_get_certificate(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_010: [** if handle is NULL, `dps_hsm_riot_get_certificate` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_011: [** `dps_hsm_riot_get_certificate` shall allocate a char* to return the riot certificate. **]**

**SRS_DPS_HSM_RIOT_07_012: [** On success `dps_hsm_riot_get_certificate` shall return the riot certificate. **]**

**SRS_DPS_HSM_RIOT_07_013: [** If any failure is encountered `dps_hsm_riot_get_certificate` shall return NULL **]**

### dps_hsm_riot_get_alias_key

```c
extern char* dps_hsm_riot_get_alias_key(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_014: [** if handle is NULL, `dps_hsm_riot_get_alias_key` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_015: [** `dps_hsm_riot_get_alias_key` shall allocate a char* to return the alias certificate. **]**

**SRS_DPS_HSM_RIOT_07_016: [** On success `dps_hsm_riot_get_alias_key` shall return the alias certificate. **]**

**SRS_DPS_HSM_RIOT_07_017: [** If any failure is encountered `dps_hsm_riot_get_alias_key` shall return NULL **]**

### dps_hsm_riot_get_device_cert

```c
extern char* dps_hsm_riot_get_device_cert(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_018: [** if handle is NULL, `dps_hsm_riot_get_device_cert` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_019: [** `dps_hsm_riot_get_device_cert` shall allocate a char* to return the device certificate. **]**

**SRS_DPS_HSM_RIOT_07_020: [** On success `dps_hsm_riot_get_device_cert` shall return the device certificate. **]**

**SRS_DPS_HSM_RIOT_07_021: [** If any failure is encountered `dps_hsm_riot_get_device_cert` shall return NULL **]**

### dps_hsm_riot_get_signer_cert

```c
extern char* dps_hsm_riot_get_signer_cert(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_022: [** if handle is NULL, `dps_hsm_riot_get_signer_cert` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_023: [** `dps_hsm_riot_get_signer_cert` shall allocate a char* to return the signer certificate. **]**

**SRS_DPS_HSM_RIOT_07_024: [** On success `dps_hsm_riot_get_signer_cert` shall return the signer certificate. **]**

**SRS_DPS_HSM_RIOT_07_025: [** If any failure is encountered `dps_hsm_riot_get_signer_cert` shall return NULL **]**

### dps_hsm_riot_get_common_name

```c
extern char* dps_hsm_riot_get_common_name(DPS_SECURE_DEVICE_HANDLE handle);
```

**SRS_DPS_HSM_RIOT_07_026: [** if handle is NULL, `dps_hsm_riot_get_common_name` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_027: [** `dps_hsm_riot_get_common_name` shall allocate a char* to return the certificate common name. **]**

**SRS_DPS_HSM_RIOT_07_028: [** If any failure is encountered `dps_hsm_riot_get_signer_cert` shall return NULL **]**

### dps_hsm_riot_create_leaf_cert

```c
char* dps_hsm_riot_create_leaf_cert(DPS_SECURE_DEVICE_HANDLE handle, const char* common_name);
```

**SRS_DPS_HSM_RIOT_07_030: [** If handle or `common_name` is NULL, `dps_hsm_riot_create_leaf_cert` shall return NULL. **]**

**SRS_DPS_HSM_RIOT_07_031: [** If successful `dps_hsm_riot_create_leaf_cert` shall return a leaf cert with the CN of `common_name`. **]**

**SRS_DPS_HSM_RIOT_07_032: [** If `dps_hsm_riot_create_leaf_cert` encounters an error it shall return NULL. **]**

### dev_auth_emulator_interface_desc

```c
extern const SEC_RIOT_INTERFACE* dps_hsm_riot_interface();
```

**SRS_DPS_HSM_RIOT_07_029: [** `dps_hsm_riot_interface` shall return the SEC_RIOT_INTERFACE structure. **]**
