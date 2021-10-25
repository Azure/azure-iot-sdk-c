// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_core.h
*    @brief Shared enums, structures, and callback functions for IoT Hub client library.
*
*/


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

#define IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES \
    FILE_UPLOAD_OK, \
    FILE_UPLOAD_ERROR

    /** @brief Enumeration specifying the status of a file upload operation.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_FILE_UPLOAD_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_RESULT_VALUES)
    typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, void* userContextCallback);

#define IOTHUB_CLIENT_RESULT_VALUES       \
    IOTHUB_CLIENT_OK,                     \
    IOTHUB_CLIENT_INVALID_ARG,            \
    IOTHUB_CLIENT_ERROR,                  \
    IOTHUB_CLIENT_INVALID_SIZE,           \
    IOTHUB_CLIENT_INDEFINITE_TIME

    /** @brief Enumeration specifying the status of calls to various APIs in IoT Hub client library.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

    /** @brief Callback applications implement to receive notifications of module initiated device method calls.
    *   @param result Result of the operation
    *   @param responseStatus HTTP status code returned from the module or device.  This is only valid if @result is @c IOTHUB_CLIENT_OK.
    *   @param responsePayload HTTP response payload returned from the module or device.  This is only valid if @result is @c IOTHUB_CLIENT_OK.
    *   @param context Context value passed is initial call to @c IoTHubModuleClient_ModuleMethodInvokeAsync (e.g.).
    *   @remarks Module clients when hosted in IoT Edge may themselves invoke methods on either modules on the same IoT Edge device
    *            or on downstream devices using APIs such as @c IoTHubModuleClient_LL_DeviceMethodInvoke or @c IoTHubModuleClient_LL_ModuleMethodInvoke.
    *            These APIs operate asychronously.  When the invoked module or device returns (or times out), the IoT Hub SDK will invoke the 
    *            application's @c IOTHUB_METHOD_INVOKE_CALLBACK callback.
    *   @warning This API is only applicable to applications running inside a module container hosted by IoT Edge.  Calling outside
    *            a IoT Edge hosted edge container will result in undefined results.
    */
    typedef void(*IOTHUB_METHOD_INVOKE_CALLBACK)(IOTHUB_CLIENT_RESULT result, int responseStatus, unsigned char* responsePayload, size_t responsePayloadSize, void* context);

#define IOTHUB_CLIENT_RETRY_POLICY_VALUES     \
    IOTHUB_CLIENT_RETRY_NONE,                   \
    IOTHUB_CLIENT_RETRY_IMMEDIATE,                  \
    IOTHUB_CLIENT_RETRY_INTERVAL,      \
    IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF,      \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF,                 \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER,                 \
    IOTHUB_CLIENT_RETRY_RANDOM

    /** @brief Enumeration passed in by the IoT Hub when the event confirmation
    *           callback is invoked to indicate status of the event processing in
    *           the hub.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

    struct IOTHUBTRANSPORT_CONFIG_TAG;
    typedef struct IOTHUBTRANSPORT_CONFIG_TAG IOTHUBTRANSPORT_CONFIG;

#define IOTHUB_CLIENT_STATUS_VALUES       \
    IOTHUB_CLIENT_SEND_STATUS_IDLE,       \
    IOTHUB_CLIENT_SEND_STATUS_BUSY

    /** @brief Enumeration returned by the GetSendStatus family of APIs (e.g. @c IoTHubDeviceClient_LL_GetSendStatus)
    *           to indicate the current sending status of the IoT Hub client.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUB_IDENTITY_TYPE and IOTHUB_IDENTITY_TYPE are internally only used enumeration that should not be documented. */
#define IOTHUB_IDENTITY_TYPE_VALUE  \
    IOTHUB_TYPE_TELEMETRY,          \
    IOTHUB_TYPE_DEVICE_TWIN,        \
    IOTHUB_TYPE_DEVICE_METHODS,     \
    IOTHUB_TYPE_EVENT_QUEUE

    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_IDENTITY_TYPE, IOTHUB_IDENTITY_TYPE_VALUE);

#define IOTHUB_PROCESS_ITEM_RESULT_VALUE    \
    IOTHUB_PROCESS_OK,                      \
    IOTHUB_PROCESS_ERROR,                   \
    IOTHUB_PROCESS_NOT_CONNECTED,           \
    IOTHUB_PROCESS_CONTINUE

    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);
    /**  \endcond */

#define IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES \
    IOTHUBMESSAGE_ACCEPTED, \
    IOTHUBMESSAGE_REJECTED, \
    IOTHUBMESSAGE_ABANDONED, \
    IOTHUBMESSAGE_ASYNC_ACK

    /** @brief Enumeration returned by application callbacks that receive Cloud-to-Device messages.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

#define IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_VALUES \
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_SUCCESS,   \
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_ERROR      \

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUB_CLIENT_IOTHUB_METHOD_STATUS is not used by IoT Hub client and should not be documented. */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_IOTHUB_METHOD_STATUS, IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_VALUES);
    /**  \endcond */


#define IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES     \
    IOTHUB_CLIENT_CONFIRMATION_OK,                   \
    IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY,      \
    IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT,      \
    IOTHUB_CLIENT_CONFIRMATION_ERROR                 \

    /** @brief Enumeration passed to the application's callback to process the success or failure of telemetry
    *          initiated by APIs in the SendEventAsync family (e.g. @c IoTHubDeviceClient_LL_SendEventAsync).
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_VALUES             \
    IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,                \
    IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED               \

    /** @brief Enumeration passed to the application callback indicating connection status changes
    *          to IoT Hub.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES      \
    IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN,            \
    IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED,              \
    IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,               \
    IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED,                \
    IOTHUB_CLIENT_CONNECTION_NO_NETWORK,                   \
    IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR,          \
    IOTHUB_CLIENT_CONNECTION_OK,                           \
    IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE              \

    /** @brief Enumeration passed to the application callback indicating reason that connection was unsuccesful.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

    /**  \cond DO_NOT_DOCUMENT */
    /* TRANSPORT_TYPE is only used internally and should not be documented. */
#define TRANSPORT_TYPE_VALUES \
    TRANSPORT_LL, /*LL comes from "LowLevel" */ \
    TRANSPORT_THREADED

    MU_DEFINE_ENUM_WITHOUT_INVALID(TRANSPORT_TYPE, TRANSPORT_TYPE_VALUES);
    /**  \endcond */


#define DEVICE_TWIN_UPDATE_STATE_VALUES \
    DEVICE_TWIN_UPDATE_COMPLETE, \
    DEVICE_TWIN_UPDATE_PARTIAL

    /** @brief Enumeration passed to application callback to receive IoT Hub device twin data.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

    /** @brief Callback application implements to process results of sending telemetry to IoT Hub.
    *   @param result Result of the telemetry send operation.
    *   @param userContextCallback Context that application specified during initial API send call.
    */
    typedef void(*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);

    /** @brief Callback application implements to process connection status changes between device and IoT Hub.
    *   @param result Whether device is successfully connected (@c IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) or not (@c IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED).
    *   @param reason More informatative reason about why connection was unsuccesful.
    *   @param userContextCallback Context that application specified during initial API call to receive status change notifications.
    */
    typedef void(*IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);

    /** @brief Callback application implements to process incoming cloud-to-device messages.
    *   @param message The incoming message received from IoT Hub.
    *   @param userContextCallback Context that application specified during initial API call to receive incoming cloud-to-device messages.
    */
    typedef IOTHUBMESSAGE_DISPOSITION_RESULT (*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);

    /** @brief Callback application implements to process data received from an IoT Hub device or module twin.
    *   @param update_state Whether application has received a full twin or just a patch update.
    *   @param payLoad Payload of device twin received.  
    *   @param size Number of bytes in @c payload.
    *   @userContextCallback Context that application specified during initial API call to receive twin data.
    *
    *   @warning The data in @c payLoad is not guaranteed to be a null-terminated string.
    */
    typedef void(*IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);

    /** @brief Callback application implements to receive notifications of device or module initiated twin updates.
    *   @param status_code HTTP style status code indicating success or failure of operation.
    *   @param userContextCallback Context that application specified during initial API call to update twin data.
    */
    typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);

    /** @brief Callback application implements to receive incoming device or module method invocations from IoT Hub
    *   @param method_name Name of method being invoked.
    *   @param payload Request payload received from IoT Hub.
    *   @param size_t size Number of bytes in @c payload.
    *   @param response Response of the requset, as specified by the application.  This should NOT include the null-terminator.
    *   @param response_size Number of bytes application specifies in @response.
    *   @param userContextCallback Context that application specified during initial API call to receive device or module method calls.
    *  
    *   @remarks The application should allocate @c response with @c malloc.  The IoT Hub client SDK will @c free the data automatically.
    *
    *   @warning The data in @c payLoad is not guaranteed to be a null-terminated string.
    *
    *   @return HTTP style return code to indicate success or failure of the request.
    */
    typedef int(*IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK is only used internally and should not be documented. */
    typedef int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback);
    /**  \endcond */


#define IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES \
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK, \
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT

    /** @brief           Return value applications use in their implementation of @c IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX to indicate status.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES);

    /** @brief           Deprecated callback mechanism for uploading data to a blob.
    *   @warning         This function return type is deprecated.  Use the callback @c IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX instead.
    */
    typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);

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
    typedef IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);

    /** @brief    This struct specifies IoT Hub client configuration. */
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

    /** @brief    This struct specifies  IoT Hub client device configuration. */
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
