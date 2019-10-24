# iothub_client_streaming Requirements


## Overview

This module provides functionality for handling Device Stream requests and responses.  


## Exposed API

```c
typedef struct DEVICE_STREAM_C2D_REQUEST_TAG
{
    char* name;
    char* uri;
    char* authorization_token;
    char* request_id;
} DEVICE_STREAM_C2D_REQUEST;

typedef struct DEVICE_STREAM_C2D_RESPONSE_TAG
{
    bool accept;
    char* request_id;
} DEVICE_STREAM_C2D_RESPONSE;

typedef DEVICE_STREAM_C2D_RESPONSE* (*DEVICE_STREAM_C2D_REQUEST_CALLBACK)(DEVICE_STREAM_C2D_REQUEST* request, const void* context);

extern DEVICE_STREAM_C2D_RESPONSE* IoTHubClient_StreamC2DResponseCreate(DEVICE_STREAM_C2D_REQUEST* request, bool accept, char* data, char* content_type, char* content_encoding);
extern DEVICE_STREAM_C2D_REQUEST* IoTHubClient_StreamC2DRequestClone(DEVICE_STREAM_C2D_REQUEST* request);
extern void IoTHubClient_StreamC2DResponseDestroy(DEVICE_STREAM_C2D_RESPONSE* response);
extern void IoTHubClient_StreamC2DRequestDestroy(DEVICE_STREAM_C2D_REQUEST* request);
```


### IoTHubClient_StreamC2DResponseCreate

```c
extern DEVICE_STREAM_C2D_RESPONSE* IoTHubClient_StreamC2DResponseCreate(DEVICE_STREAM_C2D_REQUEST* request, bool accept, char* data, char* content_type, char* content_encoding);
```

**SRS_IOTHUB_CLIENT_STREAMING_09_010: [** If `request` is NULL, the function shall return NULL **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_011: [** The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_RESPONSE (aka `response`) **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_012: [** If malloc fails, the function shall return NULL **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_013: [** `request->request_id` shall be copied into `response->request_id` **]**


### IoTHubClient_StreamC2DRequestClone

```c
extern DEVICE_STREAM_C2D_REQUEST* IoTHubClient_StreamC2DRequestClone(DEVICE_STREAM_C2D_REQUEST* request);
```

**SRS_IOTHUB_CLIENT_STREAMING_09_016: [** If `request` is NULL, the function shall return NULL **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_017: [** The function shall allocate memory for a new instance of DEVICE_STREAM_C2D_REQUEST (aka `clone`) **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_018: [** If malloc fails, the function shall return NULL **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_019: [** All fields of `request` shall be copied into `clone` **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_020: [** If any field fails to be copied, the function shall release the memory allocated and return NULL **]**



### IoTHubClient_StreamC2DResponseDestroy

```c
extern void IoTHubClient_StreamC2DResponseDestroy(DEVICE_STREAM_C2D_RESPONSE* response);
```

**SRS_IOTHUB_CLIENT_STREAMING_09_021: [** If `request` is NULL, the function shall return **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_022: [** The memory allocated for `response` shall be released **]**


### IoTHubClient_StreamC2DRequestDestroy

```c
extern void IoTHubClient_StreamC2DRequestDestroy(DEVICE_STREAM_C2D_REQUEST* request);
```

**SRS_IOTHUB_CLIENT_STREAMING_09_023: [** If `request` is NULL, the function shall return **]**

**SRS_IOTHUB_CLIENT_STREAMING_09_024: [** The memory allocated for `request` shall be released **]**

