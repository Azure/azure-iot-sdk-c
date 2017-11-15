# Provisioning Client Requirements

================================

## Overview

Provisioning Client module implements connecting to the DPS cloud service.

## Dependencies

prov_device_ll_client

## Exposed API

```c
typedef struct PROV_INSTANCE_INFO_TAG* PROV_DEVICE_HANDLE;

MOCKABLE_FUNCTION(, PROV_DEVICE_HANDLE, Prov_Device_Create, const char*, uri, const char*, scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, protocol);
MOCKABLE_FUNCTION(, void, Prov_Device_Destroy, PROV_DEVICE_HANDLE, prov_device_handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_Register_Device, PROV_DEVICE_HANDLE, prov_device_handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, register_status_callback, void*, status_user_context);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_SetOption, PROV_DEVICE_HANDLE, prov_device_handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, const char*, Prov_Device_GetVersionString);
```

### Prov_device_Create

```c
extern PROV_DEVICE_HANDLE Prov_Device_Create(const char* uri, const char* scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION protocol)
```

**SRS_PROV_DEVICE_CLIENT_12_001: [** If any of the input parameter is NULL `Prov_Device_Create` shall return NULL.**]**

**SRS_PROV_DEVICE_CLIENT_12_002: [** The function shall allocate memory for PROV_DEVICE_INSTANCE data structure.**]**

**SRS_PROV_DEVICE_CLIENT_12_003: [** If the memory allocation failed the function shall return NULL.**]**

**SRS_PROV_DEVICE_CLIENT_12_004: [** The function shall initialize the Lock.**]**

**SRS_PROV_DEVICE_CLIENT_12_005: [** If the Lock initialization failed the function shall clean up the all resources and return NULL.**]**

**SRS_PROV_DEVICE_CLIENT_12_006: [** The function shall call the LL layer Prov_Device_LL_Create function and return with it's result.**]**

**SRS_PROV_DEVICE_CLIENT_12_007: [** The function shall initialize the result datastructure.**]**


### Prov_Device_Destroy

```c
extern void Prov_Device_Destroy(PROV_DEVICE_HANDLE prov_device_handle)
```

**SRS_PROV_DEVICE_CLIENT_12_008: [** If the input parameter is NULL `Prov_Device_Destroy` shall return.**]**

**SRS_PROV_DEVICE_CLIENT_12_009: [** The function shall check the Lock status and if it is not OK set the thread signal to stop.**]**

**SRS_PROV_DEVICE_CLIENT_12_010: [** The function shall check the Lock status and if it is OK set the thread signal to stop and unlock the Lock.**]**

**SRS_PROV_DEVICE_CLIENT_12_011: [** If there is a running worker thread the function shall call join to finish.**]**

**SRS_PROV_DEVICE_CLIENT_12_012: [** The function shall call the LL layer Prov_Device_LL_Destroy with the given handle.**]**

**SRS_PROV_DEVICE_CLIENT_12_013: [** The function shall free the Lock resource with de-init.**]**

**SRS_PROV_DEVICE_CLIENT_12_014: [** The function shall free the device handle resource.**]**


### Prov_Device_Register_Device

```c
extern PROV_DEVICE_RESULT Prov_Device_Register_Device(PROV_DEVICE_HANDLE prov_device_handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback, void* user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK register_status_callback, void* status_user_context)
```

**SRS_PROV_DEVICE_CLIENT_12_015: [** If the prov_device_handle or register_callback input parameter is NULL `Prov_Device_Register_Device` shall return with invalid argument error.**]**

**SRS_PROV_DEVICE_CLIENT_12_016: [** The function shall start a worker thread with the device instance.**]**

**SRS_PROV_DEVICE_CLIENT_12_017: [** If the thread initialization failed the function shall return error.**]**

**SRS_PROV_DEVICE_CLIENT_12_018: [** The function shall try to lock the Lock.**]**

**SRS_PROV_DEVICE_CLIENT_12_019: [** If the locking failed the function shall return with error.**]**

**SRS_PROV_DEVICE_CLIENT_12_020: [** The function shall call the LL layer Prov_Device_LL_Register_Device with the given parameters and return with the result.**]**

**SRS_PROV_DEVICE_CLIENT_12_021: [** The function shall unlock the Lock.**]**


### Prov_Device_SetOption

```c
extern PROV_DEVICE_RESULT Prov_Device_SetOption(PROV_DEVICE_HANDLE prov_device_handle, const char* optionName, const void* value)
```

**SRS_PROV_DEVICE_CLIENT_12_022: [** If any of the input parameter is NULL `Prov_Device_SetOption` shall return with invalid argument error.**]**

**SRS_PROV_DEVICE_CLIENT_12_023: [** The function shall call the LL layer Prov_Device_LL_SetOption with the given parameters and return with the result.**]**


### Prov_Device_GetVersionString

```c
const char* Prov_Device_GetVersionString(void)
```

**SRS_PROV_DEVICE_CLIENT_12_024: [** The function shall call the LL layer Prov_Device_LL_GetVersionString and return with the result.**]**

