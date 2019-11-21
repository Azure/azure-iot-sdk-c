// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* Shared structures and enums for iothub convenience layer and LL layer */

#ifndef IOTHUB_CLIENT_CORE_COMMON_H
#define IOTHUB_CLIENT_CORE_COMMON_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_transport_ll.h"
#include "iothub_message.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum IOTHUB_CLIENT_FILE_UPLOAD_RESULT_TAG
{
    FILE_UPLOAD_OK = 0,
    FILE_UPLOAD_ERROR = 1
} IOTHUB_CLIENT_FILE_UPLOAD_RESULT;

typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, void* userContextCallback);

typedef enum IOTHUB_CLIENT_RESULT_TAG
{
    IOTHUB_CLIENT_OK = 0,
    IOTHUB_CLIENT_INVALID_ARG = 1,
    IOTHUB_CLIENT_ERROR = 2,
    IOTHUB_CLIENT_INVALID_SIZE = 3,
    IOTHUB_CLIENT_INDEFINITE_TIME = 4
} IOTHUB_CLIENT_RESULT;


typedef void(*IOTHUB_METHOD_INVOKE_CALLBACK)(IOTHUB_CLIENT_RESULT result, int responseStatus, unsigned char* responsePayload, size_t responsePayloadSize, void* context);

typedef enum IOTHUB_CLIENT_RETRY_POLICY_TAG
{
    IOTHUB_CLIENT_RETRY_NONE = 0,
    IOTHUB_CLIENT_RETRY_IMMEDIATE = 1,
    IOTHUB_CLIENT_RETRY_INTERVAL = 2,
    IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF = 3,
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF = 4,
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER = 5,
    IOTHUB_CLIENT_RETRY_RANDOM = 6
} IOTHUB_CLIENT_RETRY_POLICY;


struct IOTHUBTRANSPORT_CONFIG_TAG;
typedef struct IOTHUBTRANSPORT_CONFIG_TAG IOTHUBTRANSPORT_CONFIG;

typedef enum IOTHUB_CLIENT_STATUS_TAG
{
    IOTHUB_CLIENT_SEND_STATUS_IDLE = 0,
    IOTHUB_CLIENT_SEND_STATUS_BUSY = 1
} IOTHUB_CLIENT_STATUS;


typedef enum IOTHUB_IDENTITY_TYPE_TAG
{
    IOTHUB_TYPE_TELEMETRY = 0,
    IOTHUB_TYPE_DEVICE_TWIN = 1,
    IOTHUB_TYPE_DEVICE_METHODS = 2,
    IOTHUB_TYPE_EVENT_QUEUE  = 3
} IOTHUB_IDENTITY_TYPE;


typedef enum IOTHUB_PROCESS_ITEM_RESULT_TAG
{
    IOTHUB_PROCESS_OK = 0,
    IOTHUB_PROCESS_ERROR = 1,
    IOTHUB_PROCESS_NOT_CONNECTED = 2,
    IOTHUB_PROCESS_CONTINUE = 3
} IOTHUB_PROCESS_ITEM_RESULT;


typedef enum IOTHUBMESSAGE_DISPOSITION_RESULT_TAG
{
    IOTHUBMESSAGE_ACCEPTED = 0,
    IOTHUBMESSAGE_REJECTED = 1,
    IOTHUBMESSAGE_ABANDONED = 2

} IOTHUBMESSAGE_DISPOSITION_RESULT;

typedef enum IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_TAG
{
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_SUCCESS = 0,
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_ERROR = 1

} IOTHUB_CLIENT_IOTHUB_METHOD_STATUS;


typedef enum IOTHUB_CLIENT_CONFIRMATION_RESULT_TAG
{
    IOTHUB_CLIENT_CONFIRMATION_OK = 0,
    IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY = 1,
    IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT = 2,
    IOTHUB_CLIENT_CONFIRMATION_ERROR = 3
} IOTHUB_CLIENT_CONFIRMATION_RESULT;

const char * IOTHUB_CLIENT_CONFIRMATION_RESULTStrings(IOTHUB_CLIENT_CONFIRMATION_RESULT value);

typedef enum IOTHUB_CLIENT_CONNECTION_STATUS_TAG
{
    IOTHUB_CLIENT_CONNECTION_AUTHENTICATED = 0,
    IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED = 1

} IOTHUB_CLIENT_CONNECTION_STATUS;

typedef enum IOTHUB_CLIENT_CONNECTION_STATUS_REASON_TAG
{
    IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN = 0,
    IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED = 1,
    IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL = 2,
    IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED = 3,
    IOTHUB_CLIENT_CONNECTION_NO_NETWORK = 4,
    IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR = 5, 
    IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE = 6,
    IOTHUB_CLIENT_CONNECTION_OK = 7
} IOTHUB_CLIENT_CONNECTION_STATUS_REASON;

typedef enum TRANSPORT_TYPE_TAG
{
    TRANSPORT_LL = 0,
    TRANSPORT_THREADED = 1
} TRANSPORT_TYPE;


typedef enum DEVICE_TWIN_UPDATE_STATE_TAG
{
    DEVICE_TWIN_UPDATE_COMPLETE = 0,
    DEVICE_TWIN_UPDATE_PARTIAL = 1
} DEVICE_TWIN_UPDATE_STATE;

    typedef void(*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);
    typedef void(*IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
    typedef IOTHUBMESSAGE_DISPOSITION_RESULT (*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);

    typedef void(*IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
    typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);
    typedef int(*IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);
    typedef int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback);


typedef enum IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_TAG
{
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK = 0,
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT = 1
} IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT;

    /**
    *  @brief           Callback invoked by IoTHubClient_UploadMultipleBlocksToBlobAsync requesting the chunks of data to be uploaded.
    *  @param result    The result of the upload of the previous block of data provided by the user (IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX only)
    *  @param data      Next block of data to be uploaded, to be provided by the user when this callback is invoked.
    *  @param size      Size of the data parameter.
    *  @param context   User context provided on the call to IoTHubClient_UploadMultipleBlocksToBlobAsync.
    *  @remarks         If the user wants to abort the upload, the callback should return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT
    *                   It should return IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK otherwise.
    *                   If a NULL is provided for parameter "data" and/or zero is provided for "size", the user indicates to the client that the complete file has been uploaded.
    *                   In such case this callback will be invoked only once more to indicate the status of the final block upload.
    *                   If result is not FILE_UPLOAD_OK, the upload is cancelled and this callback stops being invoked.
    *                   When this callback is called for the last time, no data or size is expected, so data and size are NULL
    */
    typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);
    typedef IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);

    /** @brief    This struct captures IoTHub client configuration. */
    typedef struct IOTHUB_CLIENT_CONFIG_TAG
    {
        /** @brief A function pointer that is passed into the @c IoTHubClientCreate.
        *    A function definition for AMQP is defined in the include @c iothubtransportamqp.h.
        *   A function definition for HTTP is defined in the include @c iothubtransporthttp.h
        *   A function definition for MQTT is defined in the include @c iothubtransportmqtt.h */
        IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

        /** @brief    A string that identifies the device. */
        const char* deviceId;

        /** @brief    The device key used to authenticate the device.
        If neither deviceSasToken nor deviceKey is present then the authentication is assumed x509.*/
        const char* deviceKey;

        /** @brief    The device SAS Token used to authenticate the device in place of device key.
        If neither deviceSasToken nor deviceKey is present then the authentication is assumed x509.*/
        const char* deviceSasToken;

        /** @brief    The IoT Hub name to which the device is connecting. */
        const char* iotHubName;

        /** @brief    IoT Hub suffix goes here, e.g., private.azure-devices-int.net. */
        const char* iotHubSuffix;

        const char* protocolGatewayHostName;
    } IOTHUB_CLIENT_CONFIG;

    /** @brief    This struct captures IoTHub client device configuration. */
    typedef struct IOTHUB_CLIENT_DEVICE_CONFIG_TAG
    {
        /** @brief A function pointer that is passed into the @c IoTHubClientCreate.
        *    A function definition for AMQP is defined in the include @c iothubtransportamqp.h.
        *   A function definition for HTTP is defined in the include @c iothubtransporthttp.h
        *   A function definition for MQTT is defined in the include @c iothubtransportmqtt.h */
        IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

        /** @brief a transport handle implementing the protocol */
        void * transportHandle;

        /** @brief    A string that identifies the device. */
        const char* deviceId;

        /** @brief    The device key used to authenticate the device.
        x509 authentication is is not supported for multiplexed connections*/
        const char* deviceKey;

        /** @brief    The device SAS Token used to authenticate the device in place of device key.
        x509 authentication is is not supported for multiplexed connections.*/
        const char* deviceSasToken;
    } IOTHUB_CLIENT_DEVICE_CONFIG;

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_CORE_COMMON_H */
