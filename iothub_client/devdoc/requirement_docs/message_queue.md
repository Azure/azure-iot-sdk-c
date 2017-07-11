# message_queue Requirements


## Overview

This module implements a generic message queue.  


## Dependencies

azure_c_shared_utility

   
## Exposed API

```c
typedef struct MESSAGE_QUEUE_TAG* MESSAGE_QUEUE_HANDLE;
typedef void* MQ_MESSAGE_HANDLE;
typedef void* USER_DEFINED_REASON;

#define MESSAGE_QUEUE_RESULT_STRINGS  \
    MESSAGE_QUEUE_SUCCESS,            \
    MESSAGE_QUEUE_ERROR,              \
    MESSAGE_QUEUE_RETRYABLE_ERROR,    \
    MESSAGE_QUEUE_TIMEOUT,            \
    MESSAGE_QUEUE_CANCELLED

DEFINE_ENUM(MESSAGE_QUEUE_RESULT, MESSAGE_QUEUE_RESULT_STRINGS);

typedef void(*MESSAGE_PROCESSING_COMPLETED_CALLBACK)(MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, USER_DEFINED_REASON reason, void* user_context);
typedef void(*PROCESS_MESSAGE_COMPLETED_CALLBACK)(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, USER_DEFINED_REASON reason);
typedef void(*PROCESS_MESSAGE_CALLBACK)(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, PROCESS_MESSAGE_COMPLETED_CALLBACK on_process_message_completed_callback, void* user_context);

typedef struct MESSAGE_QUEUE_CONFIG_TAG
{
	PROCESS_MESSAGE_CALLBACK on_process_message_callback;
	size_t max_message_enqueued_time_secs;
	size_t max_message_processing_time_secs;
	size_t max_retry_count;
} MESSAGE_QUEUE_CONFIG;

extern MESSAGE_QUEUE_HANDLE message_queue_create(MESSAGE_QUEUE_CONFIG* config);
extern void message_queue_destroy(MESSAGE_QUEUE_HANDLE message_queue);
extern int message_queue_add(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, MESSAGE_PROCESSING_COMPLETED_CALLBACK on_message_processing_completed_callback, void* user_context)
extern void message_queue_remove_all(MESSAGE_QUEUE_HANDLE message_queue);
extern int message_queue_is_empty(MESSAGE_QUEUE_HANDLE message_queue, bool* is_empty);
extern void message_queue_do_work(MESSAGE_QUEUE_HANDLE message_queue);
extern int message_queue_set_max_message_enqueued_time_secs(MESSAGE_QUEUE_HANDLE message_queue, size_t seconds);
extern int message_queue_set_max_message_processing_time_secs(MESSAGE_QUEUE_HANDLE message_queue, size_t seconds);
extern OPTIONHANDLER_HANDLE message_queue_retrieve_options(MESSAGE_QUEUE_HANDLE message_queue);
```

## message_queue_create
```c
MESSAGE_QUEUE_HANDLE message_queue_create(MESSAGE_QUEUE_CONFIG* config);
```

**SRS_MESSAGE_QUEUE_09_001: [**If `config` is NULL, message_queue_create shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_002: [**If `config->on_process_message_callback` is NULL, message_queue_create shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_004: [**Memory shall be allocated for the MESSAGE_QUEUE data structure (aka `message_queue`)**]**
**SRS_MESSAGE_QUEUE_09_005: [**If `instance` cannot be allocated, message_queue_create shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_006: [**`message_queue->pending` shall be set using singlylinkedlist_create()**]**
**SRS_MESSAGE_QUEUE_09_007: [**If singlylinkedlist_create fails, message_queue_create shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_008: [**`message_queue->in_progress` shall be set using singlylinkedlist_create()**]**
**SRS_MESSAGE_QUEUE_09_009: [**If singlylinkedlist_create fails, message_queue_create shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_010: [**All arguments in `config` shall be saved into `message_queue`**]**
**SRS_MESSAGE_QUEUE_09_011: [**If any failures occur, message_queue_create shall release all memory it has allocated**]**
**SRS_MESSAGE_QUEUE_09_012: [**If no failures occur, message_queue_create shall return the `message_queue` pointer**]**


## message_queue_destroy
```c
void message_queue_destroy(MESSAGE_QUEUE_HANDLE message_queue);
```

**SRS_MESSAGE_QUEUE_09_013: [**If `message_queue` is NULL, message_queue_destroy shall return immediately**]**
**SRS_MESSAGE_QUEUE_09_014: [**message_queue_destroy shall invoke message_queue_remove_all**]**
**SRS_MESSAGE_QUEUE_09_015: [**message_queue_destroy shall free all memory allocated and pointed by `message_queue`**]**


## message_queue_add
```c
int message_queue_add(MESSAGE_QUEUE_HANDLE message_queue, MESSAGE_HANDLE message);
```

**SRS_MESSAGE_QUEUE_09_016: [**If `message_queue` or `message` are NULL, message_queue_add shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_017: [**message_queue_add shall allocate a structure (aka `mq_item`) to save the `message`**]**
**SRS_MESSAGE_QUEUE_09_018: [**If `mq_item` cannot be allocated, message_queue_add shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_019: [**`mq_item->enqueue_time` shall be set using get_time()**]**
**SRS_MESSAGE_QUEUE_09_020: [**If get_time fails, message_queue_add shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_021: [**`mq_item` shall be added to `message_queue->pending` list**]**
**SRS_MESSAGE_QUEUE_09_022: [**`mq_item` fails to be added to `message_queue->pending`, message_queue_add shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_023: [**`message` shall be saved into `mq_item->message`**]**
**SRS_MESSAGE_QUEUE_09_024: [**If any failures occur, message_queue_add shall release all memory it has allocated**]**
**SRS_MESSAGE_QUEUE_09_025: [**If no failures occur, message_queue_add shall return 0**]**


## message_queue_remove_all
```c
void message_queue_remove_all(MESSAGE_QUEUE_HANDLE message_queue);
```

**SRS_MESSAGE_QUEUE_09_026: [**If `message_queue` is NULL, message_queue_retrieve_options shall return**]**
**SRS_MESSAGE_QUEUE_09_027: [**Each `mq_item` in `message_queue->pending` and `message_queue->in_progress` lists shall be removed**]** 
**SRS_MESSAGE_QUEUE_09_028: [**`message_queue->on_message_processing_completed_callback` shall be invoked with MESSAGE_QUEUE_CANCELLED for each `mq_item` removed**]**
**SRS_MESSAGE_QUEUE_09_029: [**Each `mq_item` shall be freed**]** 


## message_queue_is_empty
```c
int message_queue_is_empty(MESSAGE_QUEUE_HANDLE message_queue, bool* is_empty);
```

**SRS_MESSAGE_QUEUE_09_030: [**If `message_queue` or `is_empty` are NULL, message_queue_is_empty shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_031: [**If `message_queue->pending` and `message_queue->in_progress` are empty, `is_empty` shall be set to true**]**
**SRS_MESSAGE_QUEUE_09_032: [**Otherwise `is_empty` shall be set to false**]**
**SRS_MESSAGE_QUEUE_09_033: [**If no failures occur, message_queue_is_empty shall return 0**]**


## message_queue_do_work
```c
void message_queue_do_work(MESSAGE_QUEUE_HANDLE message_queue);
```

**SRS_MESSAGE_QUEUE_09_034: [**If `message_queue` is NULL, message_queue_do_work shall return immediately**]**

### Message Timeout verifications

**SRS_MESSAGE_QUEUE_09_035: [**If `message_queue->max_message_enqueued_time_secs` is greater than zero, `message_queue->in_progress` and `message_queue->pending` items shall be checked for timeout**]**
**SRS_MESSAGE_QUEUE_09_036: [**If any items are in `message_queue` lists for `message_queue->max_message_enqueued_time_secs` or more, they shall be removed and `message_queue->on_message_processing_completed_callback` invoked with MESSAGE_QUEUE_TIMEOUT**]**
**SRS_MESSAGE_QUEUE_09_037: [**If `message_queue->max_message_processing_time_secs` is greater than zero, `message_queue->in_progress` items shall be checked for timeout**]**
**SRS_MESSAGE_QUEUE_09_038: [**If any items are in `message_queue->in_progress` for `message_queue->max_message_processing_time_secs` or more, they shall be removed and `message_queue->on_message_processing_completed_callback` invoked with MESSAGE_QUEUE_TIMEOUT**]**

### Process pending messages

**SRS_MESSAGE_QUEUE_09_039: [**Each `mq_item` in `message_queue->pending` shall be moved to `message_queue->in_progress`**]**
**SRS_MESSAGE_QUEUE_09_040: [**`mq_item->processing_start_time` shall be set using get_time()**]**
**SRS_MESSAGE_QUEUE_09_041: [**If get_time() fails, `mq_item` shall be removed from `message_queue->in_progress`**]**
**SRS_MESSAGE_QUEUE_09_042: [**If any failures occur, `mq_item->on_message_processing_completed_callback` shall be invoked with MESSAGE_QUEUE_ERROR and `mq_item` freed**]**
**SRS_MESSAGE_QUEUE_09_043: [**If no failures occur, `message_queue->on_process_message_callback` shall be invoked passing `mq_item->message` and `on_process_message_completed_callback`**]**

#### on_process_message_completed_callback
```c
static void on_process_message_completed_callback(MESSAGE_QUEUE_HANDLE message_queue, MQ_MESSAGE_HANDLE message, MESSAGE_QUEUE_RESULT result, USER_DEFINED_REASON reason)
```

**SRS_MESSAGE_QUEUE_09_069: [**If `message` or `message_queue` are NULL, on_process_message_completed_callback shall return immediately**]**
**SRS_MESSAGE_QUEUE_09_044: [**If `message` is not present in `message_queue->in_progress`, it shall be ignored**]**
**SRS_MESSAGE_QUEUE_09_045: [**If `message` is present in `message_queue->in_progress`, it shall be removed**]**
**SRS_MESSAGE_QUEUE_09_047: [**If `result` is MESSAGE_QUEUE_RETRYABLE_ERROR and `mq_item->number_of_attempts` is less than or equal `message_queue->max_retry_count`, the `message` shall be moved to `message_queue->pending` to be re-sent**]**
**SRS_MESSAGE_QUEUE_09_048: [**If `result` is MESSAGE_QUEUE_RETRYABLE_ERROR and `mq_item->number_of_attempts` is greater than `message_queue->max_retry_count`, result shall be changed to MESSAGE_QUEUE_ERROR**]**
**SRS_MESSAGE_QUEUE_09_049: [**Otherwise `mq_item->on_message_processing_completed_callback` shall be invoked passing `mq_item->message`, `result`, `reason` and `mq_item->user_context`**]**
**SRS_MESSAGE_QUEUE_09_050: [**The `mq_item` related to `message` shall be freed**]**


## message_queue_set_max_message_enqueued_time_secs
```c
int message_queue_set_max_message_enqueued_time_secs(MESSAGE_QUEUE_HANDLE message_queue, double seconds);
```

**SRS_MESSAGE_QUEUE_09_051: [**If `message_queue` is NULL, message_queue_set_max_message_enqueued_time_secs shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_053: [**`seconds` shall be saved into `message_queue->max_message_enqueued_time_secs`**]**
**SRS_MESSAGE_QUEUE_09_054: [**If no failures occur, message_queue_set_max_message_enqueued_time_secs shall return 0**]**


## message_queue_set_max_message_processing_time_secs
```c
int message_queue_set_max_message_processing_time_secs(MESSAGE_QUEUE_HANDLE message_queue, double seconds);
```

**SRS_MESSAGE_QUEUE_09_055: [**If `message_queue` is NULL, message_queue_set_max_message_processing_time_secs shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_057: [**`seconds` shall be saved into `message_queue->max_message_processing_time_secs`**]**
**SRS_MESSAGE_QUEUE_09_058: [**If no failures occur, message_queue_set_max_message_processing_time_secs shall return 0**]**


## message_queue_set_max_retry_count
```c
int message_queue_set_max_retry_count(MESSAGE_QUEUE_HANDLE message_queue, unsigned int max_retry_count);
```

**SRS_MESSAGE_QUEUE_09_059: [**If `message_queue` is NULL, message_queue_set_max_retry_count shall fail and return non-zero**]**
**SRS_MESSAGE_QUEUE_09_060: [**`max_retry_count` shall be saved into `message_queue->max_retry_count`**]**
**SRS_MESSAGE_QUEUE_09_061: [**If no failures occur, message_queue_set_max_retry_count shall return 0**]**


## message_queue_retrieve_options

```c
OPTIONHANDLER_HANDLE message_queue_retrieve_options(MESSAGE_QUEUE_HANDLE message_queue);
```

**SRS_MESSAGE_QUEUE_09_062: [**If `message_queue` is NULL, message_queue_retrieve_options shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_063: [**An OPTIONHANDLER_HANDLE instance shall be created using OptionHandler_Create**]**
**SRS_MESSAGE_QUEUE_09_064: [**If an OPTIONHANDLER_HANDLE instance fails to be created, message_queue_retrieve_options shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_065: [**Each option of `instance` shall be added to the OPTIONHANDLER_HANDLE instance using OptionHandler_AddOption**]**
**SRS_MESSAGE_QUEUE_09_066: [**If OptionHandler_AddOption fails, message_queue_retrieve_options shall fail and return NULL**]**
**SRS_MESSAGE_QUEUE_09_067: [**If message_queue_retrieve_options fails, any allocated memory shall be freed**]**
**SRS_MESSAGE_QUEUE_09_068: [**If no failures occur, message_queue_retrieve_options shall return the OPTIONHANDLER_HANDLE instance**]**