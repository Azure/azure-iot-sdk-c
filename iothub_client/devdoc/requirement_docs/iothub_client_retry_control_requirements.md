# iothub_client_retry_control Requirements


## Overview

This library contains functions to assist Azure C SDK APIs control their retry logic, in regards to what time retries should be attempted.


## Exposed API

```c
#include <stdlib.h>
#include <stdbool.h>
#include "azure_c_shared_utility/optionhandler.h"
#include "iothub_client_ll.h"

typedef enum RETRY_ACTION_TAG
{
	RETRY_ACTION_RETRY_NOW,
	RETRY_ACTION_RETRY_LATER,
	RETRY_ACTION_STOP_RETRYING
} RETRY_ACTION;

typedef RETRY_CONTROL_INSTANCE* RETRY_CONTROL_HANDLE;

extern RETRY_CONTROL_HANDLE retry_control_create(IOTHUB_CLIENT_RETRY_POLICY policy, unsigned int max_retry_time_in_secs);
extern int retry_control_should_retry(RETRY_CONTROL_HANDLE retry_control_handle, RETRY_ACTION* retry_action);
extern void retry_control_reset(RETRY_CONTROL_HANDLE retry_control_handle);
extern int retry_control_set_option(RETRY_CONTROL_HANDLE retry_control_handle, const char* name, const void* value);
extern OPTIONHANDLER_HANDLE retry_control_retrieve_options(RETRY_CONTROL_HANDLE retry_control_handle);
extern void retry_control_destroy(RETRY_CONTROL_HANDLE retry_control_handle);

extern int is_timeout_reached(time_t start_time, unsigned int timeout_in_secs, bool* is_timed_out);

```


### retry_control_create

```c
RETRY_CONTROL_HANDLE retry_control_create(IOTHUB_CLIENT_RETRY_POLICY policy, unsigned int max_retry_time_in_secs);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_002: [**`retry_control_create` shall allocate memory for the retry control instance structure (a.k.a. `retry_control`)**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_003: [**If malloc fails, `retry_control_create` shall fail and return NULL**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_004: [**The parameters passed to `retry_control_create` shall be saved into `retry_control`**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_005: [**If `policy` is IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF or IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, `retry_control->initial_wait_time_in_secs` shall be set to 1**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_006: [**Otherwise `retry_control->initial_wait_time_in_secs` shall be set to 5**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_007: [**`retry_control->max_jitter_percent` shall be set to 5**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_008: [**The remaining fields in `retry_control` shall be initialized according to retry_control_reset()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_009: [**If no errors occur, `retry_control_create` shall return a handle to `retry_control`**]**


### retry_control_should_retry

```c
int retry_control_should_retry(RETRY_CONTROL_HANDLE retry_control_handle, RETRY_ACTION* retry_action);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_010: [**If `retry_control_handle` or `retry_action` are NULL, `retry_control_should_retry` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_027: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_NONE, retry_action shall be set to RETRY_ACTION_STOP_RETRYING and return immediatelly with result 0**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_011: [**If `retry_control->first_retry_time` is INDEFINITE_TIME, it shall be set using get_time()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_012: [**If get_time() fails, `retry_control_should_retry` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_013: [**evaluate_retry_action() shall be invoked**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_014: [**If evaluate_retry_action() fails, `retry_control_should_retry` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_015: [**If `retry_action` is set to RETRY_ACTION_RETRY_NOW, `retry_control->retry_count` shall be incremented by 1**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_016: [**If `retry_action` is set to RETRY_ACTION_RETRY_NOW and policy is not IOTHUB_CLIENT_RETRY_IMMEDIATE, `retry_control->last_retry_time` shall be set using get_time()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_017: [**If `retry_action` is set to RETRY_ACTION_RETRY_NOW and policy is not IOTHUB_CLIENT_RETRY_IMMEDIATE, `retry_control->current_wait_time_in_secs` shall be set using calculate_next_wait_time()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_018: [**If no errors occur, `retry_control_should_retry` shall return 0**]**

Note: INDEFINITE_TIME is defined as ((time_t)-1)


#### evaluate_retry_action

```c
static int evaluate_retry_action(RETRY_CONTROL_INSTANCE* retry_control, RETRY_ACTION* retry_action)
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_019: [**If `retry_control->retry_count` is 0, `retry_action` shall be set to RETRY_ACTION_RETRY_NOW**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_020: [**If `retry_control->last_retry_time` is INDEFINITE_TIME and policy is not IOTHUB_CLIENT_RETRY_IMMEDIATE, the evaluation function shall return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_021: [**`current_time` shall be set using get_time()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_022: [**If get_time() fails, the evaluation function shall return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_023: [**If `retry_control->max_retry_time_in_secs` is not 0 and (`current_time` - `retry_control->first_retry_time`) is greater than or equal to `retry_control->max_retry_time_in_secs`, `retry_action` shall be set to RETRY_ACTION_STOP_RETRYING**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_028: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_IMMEDIATE, retry_action shall be set to RETRY_ACTION_RETRY_NOW**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_024: [**Otherwise, if (`current_time` - `retry_control->last_retry_time`) is less than `retry_control->current_wait_time_in_secs`, `retry_action` shall be set to RETRY_ACTION_RETRY_LATER**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_025: [**Otherwise, if (`current_time` - `retry_control->last_retry_time`) is greater or equal to `retry_control->current_wait_time_in_secs`, `retry_action` shall be set to RETRY_ACTION_RETRY_NOW**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_026: [**If no errors occur, the evaluation function shall return 0**]**

Note: INDEFINITE_TIME is defined as ((time_t)-1)


#### calculate_next_wait_time

```c
static unsigned int calculate_next_wait_time(RETRY_CONTROL_INSTANCE* retry_control);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_029: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_INTERVAL, `calculate_next_wait_time` shall return `retry_control->initial_wait_time_in_secs`**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_030: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF, `calculate_next_wait_time` shall return (`retry_control->initial_wait_time_in_secs` * (`retry_control->retry_count`))**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_031: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, `calculate_next_wait_time` shall return (pow(2, `retry_control->retry_count` - 1) * `retry_control->initial_wait_time_in_secs`)**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_032: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, `calculate_next_wait_time` shall return ((pow(2, `retry_control->retry_count` - 1) * `retry_control->initial_wait_time_in_secs`) * (1 + (`retry_control->max_jitter_percent` / 100) * (rand() / RAND_MAX)))**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_033: [**If `retry_control->policy` is IOTHUB_CLIENT_RETRY_RANDOM, `calculate_next_wait_time` shall return (`retry_control->initial_wait_time_in_secs` * (rand() / RAND_MAX))**]**


### retry_control_reset

```c
void retry_control_reset(RETRY_CONTROL_HANDLE retry_control_handle);
```

Resets the state of the retry control structure as if it was just created, allowing retry_control_should_retry() to respond as if it was called for the first time.

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_034: [**If `retry_control_handle` is NULL, `retry_control_reset` shall return**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_035: [**`retry_control` shall have fields `retry_count` and `current_wait_time_in_secs` set to 0 (zero), `first_retry_time` and `last_retry_time` set to INDEFINITE_TIME**]**

Note: INDEFINITE_TIME is defined as ((time_t)-1)


### retry_control_set_option

```c
int retry_control_set_option(RETRY_CONTROL_HANDLE retry_control_handle, const char* name, const void* value);
```

|Option Name|Value Type|Valid Values|Default Value|
|-----------|-----------|-----------|-----------|
|initial_wait_time_in_secs|unsigned int|Greater than or equal to 1|1 second for EXPONENTIAL policies, 5 seconds for others|
|max_jitter_percent|unsigned int|Any|0 to 100|5|
|retry_control_options|OPTIONHANDLER_HANDLE|Non-NULL|None|


**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_036: [**If `retry_control_handle`, `name` or `value` are NULL, `retry_control_set_option` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_037: [**If `name` is "initial_wait_time_in_secs" and `value` is less than 1, `retry_control_set_option` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_038: [**If `name` is "initial_wait_time_in_secs", `value` shall be saved on `retry_control->initial_wait_time_in_secs`**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_039: [**If `name` is "max_jitter_percent" and `value` is less than 0 or greater than 100, `retry_control_set_option` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_040: [**If `name` is "max_jitter_percent", value shall be saved on `retry_control->max_jitter_percent`**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_041: [**If `name` is "retry_control_options", value shall be fed to `retry_control` using OptionHandler_FeedOptions**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_042: [**If OptionHandler_FeedOptions fails, `retry_control_set_option` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_043: [**If `name` is not a supported option, `retry_control_set_option` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_044: [**If no errors occur, `retry_control_set_option` shall return 0**]**



### retry_control_retrieve_options

```c
OPTIONHANDLER_HANDLE retry_control_retrieve_options(RETRY_CONTROL_HANDLE retry_control_handle);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_045: [**If `retry_control_handle`, `retry_control_retrieve_options` shall fail and return NULL**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_046: [**An instance of OPTIONHANDLER_HANDLE (a.k.a. `options`) shall be created using OptionHandler_Create**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_047: [**If OptionHandler_Create fails, `retry_control_retrieve_options` shall fail and return NULL**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_050: [**`retry_control->initial_wait_time_in_secs` shall be added to `options` using OptionHandler_Add**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_051: [**`retry_control->max_jitter_percent` shall be added to `options` using OptionHandler_Add**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_052: [**If any call to OptionHandler_Add fails, `retry_control_retrieve_options` shall fail and return NULL**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_053: [**If any failures occur, `retry_control_retrieve_options` shall release any memory it has allocated**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_054: [**If no errors occur, `retry_control_retrieve_options` shall return the OPTIONHANDLER_HANDLE instance**]**


### retry_control_destroy

```c
void retry_control_destroy(RETRY_CONTROL_HANDLE retry_control_handle);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_055: [**If `retry_control_handle` is NULL, `retry_control_destroy` shall return**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_056: [**`retry_control_destroy` shall destroy `retry_control_handle` using free()**]**


### is_timeout_reached

```c
int is_timeout_reached(time_t start_time, unsigned int timeout_in_secs, bool* is_timed_out);
```

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_057: [**If `start_time` is INDEFINITE_TIME, `is_timeout_reached` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_058: [**If `is_timed_out` is NULL, `is_timeout_reached` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_059: [**`is_timeout_reached` shall obtain the `current_time` using get_time()**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_060: [**If get_time() fails, `is_timeout_reached` shall fail and return non-zero**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_061: [**If (`current_time` - `start_time`) is greater or equal to `timeout_in_secs`, `is_timed_out` shall be set to true**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_062: [**If (`current_time` - `start_time`) is less than `timeout_in_secs`, `is_timed_out` shall be set to false**]**

**SRS_IOTHUB_CLIENT_RETRY_CONTROL_09_063: [**If no errors occur, `is_timeout_reached` shall return 0**]**

Note: INDEFINITE_TIME is defined as ((time_t)-1)

