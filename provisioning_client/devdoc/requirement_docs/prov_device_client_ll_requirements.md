# Provisioning Client Requirements

================================

## Overview

Provisioning Client module implements connecting to the DPS cloud service.

## Dependencies

uhttp

## Exposed API

```c
typedef struct PROV_INSTANCE_INFO_TAG* PROV_DEVICE_LL_HANDLE;

#define PROV_DEVICE_RESULT_VALUE       \
    PROV_DEVICE_RESULT_OK,              \
    PROV_DEVICE_RESULT_INVALID_ARG,     \
    PROV_DEVICE_RESULT_SUCCESS,         \
    PROV_DEVICE_RESULT_MEMORY,          \
    PROV_DEVICE_RESULT_PARSING,         \
    PROV_DEVICE_RESULT_TRANSPORT,       \
    PROV_DEVICE_RESULT_INVALID_STATE,   \
    PROV_DEVICE_RESULT_DEV_AUTH_ERROR,  \
    PROV_DEVICE_RESULT_TIMEOUT,         \
    PROV_DEVICE_RESULT_KEY_ERROR,       \
    PROV_DEVICE_RESULT_ERROR

DEFINE_ENUM(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

#define PROV_DEVICE_REG_STATUS_VALUES      \
    PROV_DEVICE_REG_STATUS_CONNECTED,      \
    PROV_DEVICE_REG_STATUS_REGISTERING,    \
    PROV_DEVICE_REG_STATUS_ASSIGNING,      \
    PROV_DEVICE_REG_STATUS_ASSIGNED,       \
    PROV_DEVICE_REG_STATUS_ERROR

DEFINE_ENUM(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

typedef void(*PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK)(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context);
typedef void(*PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK)(PROV_DEVICE_REG_STATUS reg_status, void* user_context);

typedef const PROV_DEVICE_TRANSPORT_PROVIDER*(*PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION)(void);

MOCKABLE_FUNCTION(, PROV_DEVICE_LL_HANDLE, Prov_Device_LL_Create, const char*, dps_uri, const char*, scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION, dps_protocol);
MOCKABLE_FUNCTION(, void, Prov_Device_LL_Destroy, PROV_DEVICE_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_Register_Device, PROV_DEVICE_LL_HANDLE, handle, PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK, register_callback, void*, user_context, PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK, reg_status_cb, void*, status_user_ctext);
MOCKABLE_FUNCTION(, void, Prov_Device_LL_DoWork, PROV_DEVICE_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, PROV_DEVICE_RESULT, Prov_Device_LL_SetOption, PROV_DEVICE_LL_HANDLE, handle, const char*, optionName, const void*, value);
MOCKABLE_FUNCTION(, const char*, Prov_Device_LL_GetVersionString);
```

### Prov_device_LL_Create

```c
extern PROV_DEVICE_LL_HANDLE Prov_device_LL_Create(const char* drs_uri, IOTHUB_PROV_ON_ERROR_CALLBACK on_error_callback, void* user_context);
```

**SRS_PROV_CLIENT_07_001: [** If uri is NULL `Prov_device_LL_Create` shall return NULL.**]**

**SRS_PROV_CLIENT_07_002: [** `Prov_device_LL_Create` shall allocate a PROV_DEVICE_LL_HANDLE and initialize all members. **]**

**SRS_PROV_CLIENT_07_003: [** If any error is encountered, `Prov_device_LL_Create` shall return NULL. **]**

**SRS_PROV_CLIENT_07_004: [** `on_error_callback` shall be used to communicate error that occur during the registration with DPS. **]**

**SRS_PROV_CLIENT_07_034: [** `Prov_device_LL_Create` shall construct a scope_id by base64 encoding the drs_uri. **]**

**SRS_PROV_CLIENT_07_035: [** `Prov_device_LL_Create` shall store the registration_id from the security module. **]**

### Prov_device_LL_Destroy

```c
extern void Prov_device_LL_Destroy(PROV_DEVICE_LL_HANDLE handle)
```

**SRS_PROV_CLIENT_07_005: [** If handle is NULL `Prov_device_LL_Destroy` shall do nothing. **]**

**SRS_PROV_CLIENT_07_006: [** `Prov_device_LL_Destroy` shall destroy resources associated with the drs_client **]**

### Prov_device_LL_Register_Device

```c
extern PROV_DEVICE_RESULT Prov_device_LL_Register_Device(PROV_DEVICE_LL_HANDLE handle, const char* device_id, IOTHUB_PROV_REGISTER_DEVICE_CALLBACK register_callback, void* user_context)
```

**SRS_PROV_CLIENT_07_007: [** If handle, device_id or register_callback is NULL, `Prov_device_LL_Register_Device` shall return IOTHUB_PROV_INVALID_ARG. **]**

**SRS_PROV_CLIENT_07_008: [** `Prov_device_LL_Register_Device` shall set the state to send the registration request to DPS on subsequent DoWork calls. **]**

**SRS_PROV_CLIENT_07_009: [** Upon success `Prov_device_LL_Register_Device` shall return IOTHUB_PROV_OK. **]**

**SRS_PROV_CLIENT_07_031: [** Any failure that is encountered `Prov_device_LL_Register_Device` shall return IOTHUB_PROV_ERROR. **]**

### Prov_device_LL_DoWork

```c
extern void Prov_device_LL_DoWork(PROV_DEVICE_LL_HANDLE handle)
```

**SRS_PROV_CLIENT_07_010: [** If handle is NULL, `Prov_device_LL_DoWork` shall do nothing. **]**

**SRS_PROV_CLIENT_07_011: [** `Prov_device_LL_DoWork` shall call the underlying `http_client_dowork` function. **]**

`Prov_device_LL_DoWork` is a state machine that shall go through the following states:

**SRS_PROV_CLIENT_07_028: [** `PROV_CLIENT_STATE_READY` is the initial state after the DPS object is created which will send a `uhttp_client_open` call to the DPS http endpoint. **]**

**SRS_PROV_CLIENT_07_029: [** `iothub_drs_client` shall set the is_connected variable once `on_http_connected` is called from the uHttp object, which causes the transition to the `PROV_CLIENT_STATE_REGISTER_SEND` state. **]**

**SRS_PROV_CLIENT_07_030: [** `PROV_CLIENT_STATE_REGISTER_SEND` state shall retrieve the endorsement_key and register_id from a call to the dev_auth modules function. **]**

**SRS_PROV_CLIENT_07_013: [** The `PROV_CLIENT_STATE_REGISTER_SEND` state shall construct http request using `uhttp_client_execute_request` to the DPS service with the following endorsement information: **]**

DPS Service HTTP Request Type: POST

DPS Service URI:
    /drs/registrations/<registration_id>/register-me?api-version=<api_version>

DPS json Content:
    { "registrationId":"<registration_id>", "attestation": { "tpm": { "endorsementKey": "<endorsement_key>"} }, "properties": null }

**SRS_PROV_CLIENT_07_018: [** Upon receiving the reply from `iothub_drs_client` shall transition to the `PROV_CLIENT_STATE_REGISTER_RECV` state **]**

**SRS_PROV_CLIENT_07_014: [** The `PROV_CLIENT_STATE_REGISTER_RECV` state shall process the content with the following format: **]**

{ "operationId": "<operation_id>","status": "assigning" }

**SRS_PROV_CLIENT_07_19: [** Upon successfully sending the messge `iothub_drs_client` shall transition to the `PROV_CLIENT_STATE_REGISTER_SENT` state  **]**

**SRS_PROV_CLIENT_07_032: [** If the `PROV_CLIENT_STATE_REGISTER_SENT` message response status code is 200 `iothub_drs_client` shall transition to the `PROV_CLIENT_STATE_STATUS_SEND` state. **]**

**SRS_PROV_CLIENT_07_033: [** If the `PROV_CLIENT_STATE_REGISTER_SENT` message response status code is 401 `iothub_drs_client` shall transition to the `PROV_CLIENT_STATE_CONFIRM_SEND` state. **]**

**SRS_PROV_CLIENT_07_020: [** `iothub_drs_client` shall call into the dev_auth module to decrypt the nonce received from the DPS service. **]**

**SRS_PROV_CLIENT_07_021: [** If an error is encountered decrypting the nonce, `iothub_drs_client` shall transition to the PROV_CLIENT_STATE_ERROR state **]**

**SRS_PROV_CLIENT_07_015: [** `PROV_CLIENT_STATE_CONFIRM_SEND` shall construct an http request using `uhttp_client_execute_request` and send a message with the following format: **]**

DPS Service HTTP Request Type: POST
DPS Service URI:
    /drs/trust-confirmation/<registration_id>?api-version=<api_version>
DPS json Content:
    {
        correlationId: "<correlation_Id>",
        nonce: "decrypted_nonce"
    }

**SRS_PROV_CLIENT_07_022: [** Upon receiving the reply of the `PROV_CLIENT_STATE_CONFIRM_SEND` message from DPS, `iothub_drs_client` shall transistion to the `PROV_CLIENT_STATE_CONFIRM_RECV` state. **]**

**SRS_PROV_CLIENT_07_023: [** `PROV_CLIENT_STATE_CONFIRM_RECV` state shall process the content with the following format: **]**

**SRS_PROV_CLIENT_07_024: [** Once the progressUrl is extracted from the reply `iothub_drs_client` shall transition to the `PROV_CLIENT_STATE_URL_REQ_RECV` state. **]**

**SRS_PROV_CLIENT_07_025: [** `PROV_CLIENT_STATE_URL_REQ_SEND` shall construct an http request using `uhttp_client_execute_request` to the DPS Service message with the following format: **]**

DPS Service HTTP Request Type: GET
DPS Service URI:
    <progressUrl_from_previous_content>?api-version=<api_version>
DPS json Content:
    none

**SRS_PROV_CLIENT_07_026: [** Upon receiving the reply of the `PROV_CLIENT_STATE_URL_REQ_SEND` message from DPS, `iothub_drs_client` shall process the the reply of the `PROV_CLIENT_STATE_URL_REQ_SEND` state **]**

{
    "IotHubUrl" : "<iothub_url>"
}

**SRS_PROV_CLIENT_07_016: [** `PROV_CLIENT_STATE_URL_REQ_RECV` state shall call the register_callback supplied by the user in the `Prov_device_LL_Register_Device` function call the the url and the iothub keys. **]**

**SRS_PROV_CLIENT_07_017: [** If any errors occur the state shall be set to `PROV_CLIENT_STATE_ERROR` which will cause the user supplied `error_callback` to be executed. **]**
