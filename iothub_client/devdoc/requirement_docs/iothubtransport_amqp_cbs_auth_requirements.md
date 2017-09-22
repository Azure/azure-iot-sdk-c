# iothubtransport_amqp_cbs_auth Requirements

## Overview

This module provides APIs for authenticating devices through Azure IoT Hub CBS.  
Ultimately, it depends on a CONNECTION_HANDLE instance managed by the upper layer.
To have the actual authentication messages sent/received on the wire, connection_dowork() (uAMQP) shall be invoked upon such CONNECTION_HANDLE.

   
## Exposed API

```c
#define AUTHENTICATION_OPTION_SAVED_OPTIONS               "saved_authentication_options"
#define AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS    "cbs_request_timeout_secs"
#define AUTHENTICATION_OPTION_SAS_TOKEN_REFRESH_TIME_SECS "sas_token_refresh_time_secs"
#define AUTHENTICATION_OPTION_SAS_TOKEN_LIFETIME_SECS     "sas_token_lifetime_secs"

typedef enum AUTHENTICATION_STATE_TAG
{
	AUTHENTICATION_STATE_STOPPED,
	AUTHENTICATION_STATE_STARTING,
	AUTHENTICATION_STATE_STARTED,
	AUTHENTICATION_STATE_ERROR
} AUTHENTICATION_STATE;

typedef enum AUTHENTICATION_ERROR_TAG
{
	AUTHENTICATION_ERROR_NONE,
	AUTHENTICATION_ERROR_AUTH_TIMEOUT,
	AUTHENTICATION_ERROR_AUTH_FAILED,
	AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT,
	AUTHENTICATION_ERROR_SAS_REFRESH_FAILED
} AUTHENTICATION_ERROR_CODE;

typedef void(*ON_AUTHENTICATION_STATE_CHANGED_CALLBACK)(void* context, AUTHENTICATION_STATE previous_state, AUTHENTICATION_STATE new_state);

typedef struct AUTHENTICATION_CONFIG_TAG
{
    const char* device_id;
    char* iothub_host_fqdn;

    ON_AUTHENTICATION_STATE_CHANGED_CALLBACK on_state_changed_callback;
    const void* on_state_changed_callback_context;

    ON_AUTHENTICATION_ERROR_CALLBACK on_error_callback;
    const void* on_error_callback_context;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
} AUTHENTICATION_CONFIG;

typedef struct AUTHENTICATION_INSTANCE* AUTHENTICATION_HANDLE;

extern AUTHENTICATION_HANDLE authentication_create(const AUTHENTICATION_CONFIG* config);
extern int authentication_start(AUTHENTICATION_HANDLE authentication_handle, const CBS_HANDLE cbs_handle);
extern int authentication_stop(AUTHENTICATION_HANDLE authentication_handle);
extern void authentication_do_work(AUTHENTICATION_HANDLE authentication_handle);
extern void authentication_destroy(AUTHENTICATION_HANDLE authentication_handle);
extern int authentication_set_option(AUTHENTICATION_HANDLE authentication_handle, const char* name, void* value);
extern OPTIONHANDLER_HANDLE authentication_retrieve_options(AUTHENTICATION_HANDLE authentication_handle);
```


### authentication_create

```c
AUTHENTICATION_HANDLE authentication_create(const AUTHENTICATION_CONFIG* config)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_001: [**If parameter `config` is NULL, authentication_create() shall fail and return NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_004: [**If `config->iothub_host_fqdn` is NULL, authentication_create() shall fail and return NULL.**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_005: [**If `config->on_state_changed_callback` is NULL, authentication_create() shall fail and return NULL**]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_006: [**authentication_create() shall allocate memory for a new authenticate state structure AUTHENTICATION_INSTANCE.**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_007: [**If malloc() fails, authentication_create() shall fail and return NULL.**]**

Note: the AUTHENTICATION_INSTANCE instance shall be referred to as `instance` throughout the remaining requirements.

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_123: [**authentication_create() shall initialize all fields of `instance` with 0 using memset().**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_009: [**If STRING_construct() fails, authentication_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_010: [**If `device_config->device_sas_token` is not NULL, authentication_create() shall save a copy into the `instance->device_sas_token`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_011: [**If STRING_construct() fails, authentication_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_016: [**If provided, authentication_create() shall save a copy of `config->iothub_host_fqdn` into `instance->iothub_host_fqdn`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_017: [**If STRING_clone() fails to copy `config->iothub_host_fqdn`, authentication_create() shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_018: [**authentication_create() shall save `config->on_state_changed_callback` and `config->on_state_changed_callback_context` into `instance->on_state_changed_callback` and `instance->on_state_changed_callback_context`.**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_019: [**authentication_create() shall save `config->on_error_callback` and `config->on_error_callback_context` into `instance->on_error_callback` and `instance->on_error_callback_context`.**]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_020: [**If any failure occurs, authentication_create() shall free any memory it allocated previously**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_021: [**authentication_create() shall set `instance->cbs_request_timeout_secs` with the default value of UINT32_MAX**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_022: [**authentication_create() shall set `instance->sas_token_lifetime_secs` with the default value of one hour**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_023: [**authentication_create() shall set `instance->sas_token_refresh_time_secs` with the default value of 30 minutes**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_024: [**If no failure occurs, authentication_create() shall return a reference to the AUTHENTICATION_INSTANCE handle**]**


### authentication_start

```c
int authentication_start(AUTHENTICATION_HANDLE authentication_handle, const CBS_HANDLE cbs_handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_025: [**If authentication_handle is NULL, authentication_start() shall fail and return __FAILURE__ as error code**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_026: [**If `cbs_handle` is NULL, authentication_start() shall fail and return __FAILURE__ as error code**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_027: [**If authenticate state has been started already, authentication_start() shall fail and return __FAILURE__ as error code**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_028: [**authentication_start() shall save `cbs_handle` on `instance->cbs_handle`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_029: [**If no failures occur, `instance->state` shall be set to AUTHENTICATION_STATE_STARTING and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_030: [**If no failures occur, authentication_start() shall return 0**]**


### authentication_stop

```c
int authentication_stop(AUTHENTICATION_HANDLE authentication_handle, ON_AUTHENTICATION_STOP_COMPLETED on_stop_completed, const void* context)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_031: [**If `authentication_handle` is NULL, authentication_stop() shall fail and return __FAILURE__**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_032: [**If `instance->state` is AUTHENTICATION_STATE_STOPPED, authentication_stop() shall fail and return __FAILURE__**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_033: [**`instance->cbs_handle` shall be set to NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_034: [**`instance->state` shall be set to AUTHENTICATION_STATE_STOPPED and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_035: [**authentication_stop() shall return success code 0**]**


### authentication_do_work

```c
void authentication_do_work(AUTHENTICATION_HANDLE authentication_handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_036: [**If authentication_handle is NULL, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_037: [**If `instance->state` is not AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_do_work() shall fail and return**]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_038: [**If `instance->is_cbs_put_token_async_in_progress` is TRUE, authentication_do_work() shall only verify the authentication timeout**]**
Note: see "Authentication and SAS token refresh timeout" below.

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_039: [**If `instance->state` is AUTHENTICATION_STATE_STARTED and device keys were used, authentication_do_work() shall only verify the SAS token refresh time**]**
Note: see "SAS token refresh" below.

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_040: [**If `instance->state` is AUTHENTICATION_STATE_STARTED and user-provided SAS token was used, authentication_do_work() shall return**]**

The below will take place if `instance->state` is AUTHENTICATION_STATE_STARTING.

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_041: [**If `instance->device_sas_token` is provided, authentication_do_work() shall put it to CBS**]**
Note: see "SAS token authentication" below.

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_042: [**Otherwise, authentication_do_work() shall use device keys for CBS authentication**]**
Note: see "Device Key authentication" below.


#### SAS token authentication

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_043: [**authentication_do_work() shall set `instance->is_cbs_put_token_async_in_progress` to TRUE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_044: [**A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id**]**


**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_045: [**If `devices_path` failed to be created, authentication_do_work() shall set `instance->is_cbs_put_token_async_in_progress` to FALSE and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_046: [**The SAS token provided shall be sent to CBS using cbs_put_token_async(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_047: [**If cbs_put_token_async() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_048: [**If cbs_put_token_async() failed, authentication_do_work() shall set `instance->is_cbs_put_token_async_in_progress` to FALSE, destroy `devices_path` and return**]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_121: [**If cbs_put_token_async() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_122: [**If cbs_put_token_async() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED**]**

Note: see "on_cbs_put_token_complete_callback" below. 


#### Device Key authentication

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_07_001: [**`authentication_do_work()` shall determine what credential type is used SAS_TOKEN or DEVICE_KEY by calling `IoTHubClient_Auth_Get_Credential_Type` **]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_049: [**`authentication_do_work()` shall create a SAS token using `IoTHubClient_Auth_Get_SasToken`, unless it has failed previously**]**

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_07_002: [** If credential Type is SAS_TOKEN `authentication_do_work()` shall validate the sas_token, and fail if it's not valid. **]**

##### Creating a SAS token from a device key and putting it to CBS 

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_053: [**A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_054: [**If `devices_path` failed to be created, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_055: [**The SAS token shall be created using `IoTHubClient_Auth_Get_SasToken`, passing the selected device key, `device_path`, `sasTokenKeyName` and expiration time as arguments**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_056: [**If `IoTHubClient_Auth_Get_SasToken` fails, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_057: [**authentication_do_work() shall set `instance->is_cbs_put_token_async_in_progress` to TRUE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_058: [**The SAS token shall be sent to CBS using cbs_put_token_async(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_059: [**If cbs_put_token_async() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with current time**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_060: [**If cbs_put_token_async() fails, `instance->is_cbs_put_token_async_in_progress` shall be set to FALSE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_061: [**If cbs_put_token_async() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_062: [**If cbs_put_token_async() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_063: [**authentication_do_work() shall free the memory it allocated for `devices_path`, `sas_token` and SAS token**]**

Note: see "on_cbs_put_token_complete_callback" below.


#### SAS token refresh

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_065: [**The SAS token shall be refreshed if the current time minus `instance->current_sas_token_put_time` equals or exceeds `instance->sas_token_refresh_time_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_066: [**If SAS token does not need to be refreshed, authentication_do_work() shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_067: [**authentication_do_work() shall create a SAS token using `instance->device_primary_key`, unless it has failed previously**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_068: [**If using `instance->device_primary_key` has failed previously and `instance->device_secondary_key` is not provided,  authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_069: [**If using `instance->device_primary_key` has failed previously, a SAS token shall be created using `instance->device_secondary_key`**]**

The requirements below apply to the creation of the SAS token and putting it to CBS:
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_071: [**A STRING_HANDLE, referred to as `devices_path`, shall be created from the following parts: iothub_host_fqdn + "/devices/" + device_id**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_072: [**If `devices_path` failed to be created, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_117: [**An empty STRING_HANDLE, referred to as `sasTokenKeyName`, shall be created using STRING_new()**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_118: [**If `sasTokenKeyName` failed to be created, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_073: [**The SAS token shall be created using SASToken_Create(), passing the selected device key, device_path, sasTokenKeyName and expiration time as arguments**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_074: [**If SASToken_Create() fails, authentication_do_work() shall fail and return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_075: [**authentication_do_work() shall set `instance->is_cbs_put_token_async_in_progress` to TRUE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_119: [**authentication_do_work() shall set `instance->is_sas_token_refresh_in_progress` to TRUE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_076: [**The SAS token shall be sent to CBS using cbs_put_token_async(), using `servicebus.windows.net:sastoken` as token type, `devices_path` as audience and passing on_cbs_put_token_complete_callback**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_077: [**If cbs_put_token_async() succeeds, authentication_do_work() shall set `instance->current_sas_token_put_time` with the current time**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_078: [**If cbs_put_token_async() fails, `instance->is_cbs_put_token_async_in_progress` shall be set to FALSE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_120: [**If cbs_put_token_async() fails, `instance->is_sas_token_refresh_in_progress` shall be set to FALSE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_079: [**If cbs_put_token_async() fails, `instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_080: [**If cbs_put_token_async() fails, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_081: [**authentication_do_work() shall free the memory it allocated for `devices_path`, `sasTokenKeyName` and SAS token**]**


#### Authentication and SAS token refresh timeout

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_083: [**authentication_do_work() shall check for authentication timeout comparing the current time since `instance->current_sas_token_put_time` to `instance->cbs_request_timeout_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_084: [**If no timeout has occurred, authentication_do_work() shall return**]**

If the authentication has timed out,
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_085: [**`instance->is_cbs_put_token_async_in_progress` shall be set to FALSE**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_086: [**`instance->state` shall be updated to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_087: [**If `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_TIMEOUT**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_088: [**If `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback` shall be invoked with AUTHENTICATION_ERROR_AUTH_TIMEOUT**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_089: [**`instance->is_sas_token_refresh_in_progress` shall be set to FALSE**]**


###### on_cbs_put_token_complete_callback

```c
static void on_cbs_put_token_complete_callback(void* context, CBS_OPERATION_RESULT result, unsigned int status_code, const char* status_description)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_091: [**If `result` is CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_STARTED and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_092: [**If `result` is not CBS_OPERATION_RESULT_OK `instance->state` shall be set to AUTHENTICATION_STATE_ERROR and `instance->on_state_changed_callback` invoked**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_093: [**If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is FALSE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_AUTH_FAILED**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_094: [**If `result` is not CBS_OPERATION_RESULT_OK and `instance->is_sas_token_refresh_in_progress` is TRUE, `instance->on_error_callback`shall be invoked with AUTHENTICATION_ERROR_SAS_REFRESH_FAILED**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_095: [**`instance->is_sas_token_refresh_in_progress` and `instance->is_cbs_put_token_async_in_progress` shall be set to FALSE**]**




### authentication_set_option

```c
int authentication_set_option(AUTHENTICATION_HANDLE authentication_handle, const char* name, void* value)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_097: [**If `authentication_handle` or `name` or `value` is NULL, authentication_set_option shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_098: [**If name matches AUTHENTICATION_OPTION_CBS_REQUEST_TIMEOUT_SECS, `value` shall be saved on `instance->cbs_request_timeout_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_124: [**If name matches AUTHENTICATION_OPTION_SAS_TOKEN_REFRESH_TIME_SECS, `value` shall be saved on `instance->sas_token_refresh_time_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_125: [**If name matches AUTHENTICATION_OPTION_SAS_TOKEN_LIFETIME_SECS, `value` shall be saved on `instance->sas_token_lifetime_secs`**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_098: [**If name matches AUTHENTICATION_OPTION_SAVED_OPTIONS, `value` shall be applied using OptionHandler_FeedOptions**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_126: [**If OptionHandler_FeedOptions fails, authentication_set_option shall fail and return a non-zero value**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_099: [**If no errors occur, authentication_set_option shall return 0**]**


### authentication_retrieve_options

```c
OPTIONHANDLER_HANDLE authentication_retrieve_options(AUTHENTICATION_HANDLE authentication_handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_100: [**If `authentication_handle` is NULL, authentication_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_101: [**An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_102: [**If an OPTIONHANDLER_HANDLE instance fails to be created, authentication_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_103: [**Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_104: [**If OptionHandler_AddOption fails, authentication_retrieve_options shall fail and return NULL**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_105: [**If authentication_retrieve_options fails, any allocated memory shall be freed**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_127: [**If no failures occur, authentication_retrieve_options shall return the OPTIONHANDLER_HANDLE instance**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_128: [**If name does not match any supported option, authentication_set_option shall fail and return a non-zero value**]**


### authentication_destroy

```c
void authentication_destroy(AUTHENTICATION_HANDLE authentication_handle)
```

**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_106: [**If authentication_handle is NULL, authentication_destroy() shall return**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_107: [**If `instance->state` is AUTHENTICATION_STATE_STARTING or AUTHENTICATION_STATE_STARTED, authentication_stop() shall be invoked and its result ignored**]**
**SRS_IOTHUBTRANSPORT_AMQP_AUTH_09_108: [**authentication_destroy() shall destroy all resouces used by this module **]**

