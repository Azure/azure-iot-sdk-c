# IoThub_Security_x509 Requirements

================================

## Overview

IoThub_Security_x509 module implements the interface over a x509 module.

## Dependencies

dps RIoT Module

## Exposed API

```c
MOCKABLE_FUNCTION(, SECURITY_DEVICE_HANDLE, iothub_security_x509_create);
MOCKABLE_FUNCTION(, void, iothub_security_x509_destroy, SECURITY_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, const char*, iothub_security_x509_get_certificate, SECURITY_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, const char*, iothub_security_x509_get_alias_key, SECURITY_DEVICE_HANDLE, handle);

MOCKABLE_FUNCTION(, const X509_SECURITY_INTERFACE*, iothub_security_x509_interface);
MOCKABLE_FUNCTION(, int, initialize_x509_system);
MOCKABLE_FUNCTION(, void, deinitialize_x509_system);
```

### iothub_security_x509_create

```c
SECURITY_DEVICE_HANDLE iothub_security_x509_create()
```

**SRS_IOTHUB_SECURITY_x509_07_001: [** On success `iothub_security_x509_create` shall allocate a new instance of the `SECURITY_DEVICE_HANDLE` interface. **]**

**SRS_IOTHUB_SECURITY_x509_07_002: [** `iothub_security_x509_create` shall call into the dps RIoT module to retrieve the `DPS_SECURE_DEVICE_HANDLE`. **]**

**SRS_IOTHUB_SECURITY_x509_07_003: [** `iothub_security_x509_create` shall cache the `x509_certificate` from the RIoT module. **]**

**SRS_IOTHUB_SECURITY_x509_07_004: [** `iothub_security_x509_create` shall cache the `x509_alias_key` from the RIoT module. **]**

**SRS_IOTHUB_SECURITY_x509_07_006: [** If any failure is encountered `iothub_security_x509_create` shall return NULL **]**

### iothub_security_x509_destroy

```c
void iothub_security_x509_destroy(SECURITY_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_SECURITY_x509_07_007: [** if handle is NULL, `iothub_security_x509_destroy` shall do nothing. **]**

**SRS_IOTHUB_SECURITY_x509_07_008: [** `iothub_security_x509_destroy` shall free the `SECURITY_DEVICE_HANDLE` instance. **]**

**SRS_IOTHUB_SECURITY_x509_07_009: [** `iothub_security_x509_destroy` shall free all resources allocated in this module. **]**

### iothub_security_x509_get_certificate

```c
const char* iothub_security_x509_get_certificate(SECURITY_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_SECURITY_x509_07_010: [** if handle is NULL, `iothub_security_x509_get_certificate` shall return NULL. **]**

**SRS_IOTHUB_SECURITY_x509_07_011: [** `iothub_security_x509_get_certificate` shall return the cached riot certificate. **]**

### iothub_security_x509_get_alias_key

```c
const char* iothub_security_x509_get_alias_key(SECURITY_DEVICE_HANDLE handle)
```

**SRS_IOTHUB_SECURITY_x509_07_014: [** if handle is NULL, `secure_device_riot_get_alias_key` shall return NULL. **]**

**SRS_IOTHUB_SECURITY_x509_07_015: [** `secure_device_riot_get_alias_key` shall allocate a char* to return the alias certificate. **]**

### iothub_security_x509_interface

```c
const X509_SECURITY_INTERFACE* iothub_security_x509_interface()
```

**SRS_IOTHUB_SECURITY_x509_07_029: [** `iothub_security_x509_interface` shall return the X509_SECURITY_INTERFACE structure. **]**
