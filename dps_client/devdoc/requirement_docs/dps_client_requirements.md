# IoTHub_DPS_client Requirements

================================

## Overview

IoTHub_DPS_client module implements connecting to the DPS cloud service.

## Dependencies

uhttp

## Exposed API

```c
typedef struct DPS_INSTANCE_INFO* DPS_LL_HANDLE;

#define IOTHUB_DPS_RESULT_VALUES    \
    IOTHUB_DPS_OK,                  \
    IOTHUB_DPS_INVALID_ARG,         \
    IOTHUB_DPS_ERROR

DEFINE_ENUM(IOTHUB_DPS_RESULT, IOTHUB_DPS_RESULT_VALUES);

#define DPS_ERROR_VALUES            \
    DPS_ERROR_SUCCESS,              \
    DPS_ERROR_MEMORY,               \
    DPS_ERROR_PARSING,              \
    DPS_ERROR_TRANSMISSION,         \
    DPS_ERROR_DEV_AUTH_ERROR,       \
    DPS_ERROR_SERVER_STATUS_CODE

DEFINE_ENUM(DPS_ERROR, DPS_ERROR_VALUES);

typedef void(*IOTHUB_DPS_REGISTER_DEVICE_CALLBACK)(IOTHUB_DPS_RESULT register_result, const char* iothub_uri, const char* key, void* user_context);
typedef void(*IOTHUB_DPS_ON_ERROR_CALLBACK)(void* user_context, DPS_ERROR error_type);

DPS_LL_HANDLE DPS_LL_Create(const char* drs_uri, IOTHUB_DPS_ON_ERROR_CALLBACK on_error_callback, void* user_context);
void IoTHub_DPS_LL_Destroy(DPS_LL_HANDLE handle);
IOTHUB_DPS_RESULT IoTHub_DPS_LL_Register_Device(DPS_LL_HANDLE handle, IOTHUB_DPS_REGISTER_DEVICE_CALLBACK register_callback, void* user_context);
void IoTHub_DPS_LL_DoWork(DPS_LL_HANDLE handle);
```

### DPS_LL_Create

```c
extern DPS_LL_HANDLE DPS_LL_Create(const char* drs_uri, IOTHUB_DPS_ON_ERROR_CALLBACK on_error_callback, void* user_context);
```

**SRS_DPS_CLIENT_07_001: [** If dev_auth_handle or drs_uri is NULL `DPS_LL_Create` shall return NULL.**]**

**SRS_DPS_CLIENT_07_002: [** `DPS_LL_Create` shall allocate a DPS_LL_HANDLE and initialize all members. **]**

**SRS_DPS_CLIENT_07_003: [** If any error is encountered, `DPS_LL_Create` shall return NULL. **]**

**SRS_DPS_CLIENT_07_004: [** `on_error_callback` shall be used to communicate error that occur during the registration with DPS. **]**

**SRS_DPS_CLIENT_07_034: [** `DPS_LL_Create` shall construct a scope_id by base64 encoding the drs_uri. **]**

**SRS_DPS_CLIENT_07_035: [** `DPS_LL_Create` shall store the registration_id from the security module. **]**

### IoTHub_DPS_LL_Destroy

```c
extern void IoTHub_DPS_LL_Destroy(DPS_LL_HANDLE handle)
```

**SRS_DPS_CLIENT_07_005: [** If handle is NULL `IoTHub_DPS_LL_Destroy` shall do nothing. **]**

**SRS_DPS_CLIENT_07_006: [** `IoTHub_DPS_LL_Destroy` shall destroy resources associated with the IoTHub_drs_client **]**

### IoTHub_DPS_LL_Register_Device

```c
extern IOTHUB_DPS_RESULT IoTHub_DPS_LL_Register_Device(DPS_LL_HANDLE handle, const char* device_id, IOTHUB_DPS_REGISTER_DEVICE_CALLBACK register_callback, void* user_context)
```

**SRS_DPS_CLIENT_07_007: [** If handle, device_id or register_callback is NULL, `IoTHub_DPS_LL_Register_Device` shall return IOTHUB_DPS_INVALID_ARG. **]**

**SRS_DPS_CLIENT_07_008: [** `IoTHub_DPS_LL_Register_Device` shall set the state to send the registration request to DPS on subsequent DoWork calls. **]**

**SRS_DPS_CLIENT_07_009: [** Upon success `IoTHub_DPS_LL_Register_Device` shall return IOTHUB_DPS_OK. **]**

**SRS_DPS_CLIENT_07_031: [** Any failure that is encountered `IoTHub_DPS_LL_Register_Device` shall return IOTHUB_DPS_ERROR. **]**

### IoTHub_DPS_LL_DoWork

```c
extern void IoTHub_DPS_LL_DoWork(DPS_LL_HANDLE handle)
```

**SRS_DPS_CLIENT_07_010: [** If handle is NULL, `IoTHub_DPS_LL_DoWork` shall do nothing. **]**

**SRS_DPS_CLIENT_07_011: [** `IoTHub_DPS_LL_DoWork` shall call the underlying `http_client_dowork` function. **]**

`IoTHub_DPS_LL_DoWork` is a state machine that shall go through the following states:

**SRS_DPS_CLIENT_07_028: [** `DPS_CLIENT_STATE_READY` is the initial state after the DPS object is created which will send a `uhttp_client_open` call to the DPS http endpoint. **]**

**SRS_DPS_CLIENT_07_029: [** `iothub_drs_client` shall set the is_connected variable once `on_http_connected` is called from the uHttp object, which causes the transition to the `DPS_CLIENT_STATE_REGISTER_SEND` state. **]**

**SRS_DPS_CLIENT_07_030: [** `DPS_CLIENT_STATE_REGISTER_SEND` state shall retrieve the endorsement_key and register_id from a call to the dev_auth modules function. **]**

**SRS_DPS_CLIENT_07_013: [** The `DPS_CLIENT_STATE_REGISTER_SEND` state shall construct http request using `uhttp_client_execute_request` to the DPS service with the following endorsement information: **]**

DPS Service HTTP Request Type: POST

DPS Service URI:
    /drs/registrations/<registration_id>/register-me?api-version=<api_version>

DPS json Content:
    { "registrationId":"<registration_id>", "attestation": { "tpm": { "endorsementKey": "<endorsement_key>"} }, "properties": null }

**SRS_DPS_CLIENT_07_018: [** Upon receiving the reply from `iothub_drs_client` shall transition to the `DPS_CLIENT_STATE_REGISTER_RECV` state **]**

**SRS_DPS_CLIENT_07_014: [** The `DPS_CLIENT_STATE_REGISTER_RECV` state shall process the content with the following format: **]**

{ "operationId": "<operation_id>","status": "assigning" }

**SRS_DPS_CLIENT_07_19: [** Upon successfully sending the messge `iothub_drs_client` shall transition to the `DPS_CLIENT_STATE_REGISTER_SENT` state  **]**

**SRS_DPS_CLIENT_07_032: [** If the `DPS_CLIENT_STATE_REGISTER_SENT` message response status code is 200 `iothub_drs_client` shall transition to the `DPS_CLIENT_STATE_STATUS_SEND` state. **]**

**SRS_DPS_CLIENT_07_033: [** If the `DPS_CLIENT_STATE_REGISTER_SENT` message response status code is 401 `iothub_drs_client` shall transition to the `DPS_CLIENT_STATE_CONFIRM_SEND` state. **]**

**SRS_DPS_CLIENT_07_020: [** `iothub_drs_client` shall call into the dev_auth module to decrypt the nonce received from the DPS service. **]**

**SRS_DPS_CLIENT_07_021: [** If an error is encountered decrypting the nonce, `iothub_drs_client` shall transition to the DPS_CLIENT_STATE_ERROR state **]**

**SRS_DPS_CLIENT_07_015: [** `DPS_CLIENT_STATE_CONFIRM_SEND` shall construct an http request using `uhttp_client_execute_request` and send a message with the following format: **]**

DPS Service HTTP Request Type: POST
DPS Service URI:
    /drs/trust-confirmation/<registration_id>?api-version=<api_version>
DPS json Content:
    {
        correlationId: "<correlation_Id>",
        nonce: "decrypted_nonce"
    }

**SRS_DPS_CLIENT_07_022: [** Upon receiving the reply of the `DPS_CLIENT_STATE_CONFIRM_SEND` message from DPS, `iothub_drs_client` shall transistion to the `DPS_CLIENT_STATE_CONFIRM_RECV` state. **]**

**SRS_DPS_CLIENT_07_023: [** `DPS_CLIENT_STATE_CONFIRM_RECV` state shall process the content with the following format: **]**

**SRS_DPS_CLIENT_07_024: [** Once the progressUrl is extracted from the reply `iothub_drs_client` shall transition to the `DPS_CLIENT_STATE_URL_REQ_RECV` state. **]**

**SRS_DPS_CLIENT_07_025: [** `DPS_CLIENT_STATE_URL_REQ_SEND` shall construct an http request using `uhttp_client_execute_request` to the DPS Service message with the following format: **]**

DPS Service HTTP Request Type: GET
DPS Service URI:
    <progressUrl_from_previous_content>?api-version=<api_version>
DPS json Content:
    none

**SRS_DPS_CLIENT_07_026: [** Upon receiving the reply of the `DPS_CLIENT_STATE_URL_REQ_SEND` message from DPS, `iothub_drs_client` shall process the the reply of the `DPS_CLIENT_STATE_URL_REQ_SEND` state **]**

{
    "IotHubUrl" : "<iothub_url>"
}

**SRS_DPS_CLIENT_07_016: [** `DPS_CLIENT_STATE_URL_REQ_RECV` state shall call the register_callback supplied by the user in the `IoTHub_DPS_LL_Register_Device` function call the the url and the iothub keys. **]**

**SRS_DPS_CLIENT_07_017: [** If any errors occur the state shall be set to `DPS_CLIENT_STATE_ERROR` which will cause the user supplied `error_callback` to be executed. **]**
