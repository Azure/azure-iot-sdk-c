// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_core_common.h
*    @brief Shared enums, structures, and callback functions for IoT Hub client.
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

    /** @brief           Deprecated callback mechanism for uploading data to a blob.
    *   @deprecated      This callback type is deprecated.  Use the callback IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX() instead.
    */
    typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, void* userContextCallback);

/** @remark `struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_STRUCT` contains information specifically
 *          related to an individual upload request currently active in Azure IoT Hub, mainly
 *          the correlation-id and Azure Blob SAS URI provided by the Azure IoT Hub when a new
 *          upload is started. The `struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE_DATA` on the other hand 
 *          holds common information (independent from individual upload requests) that is used for
 *          upload-to-blob Rest API calls to Azure IoT Hub.
 */
typedef struct IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_STRUCT* IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE;

#define IOTHUB_CLIENT_RESULT_VALUES       \
    IOTHUB_CLIENT_OK,                     \
    IOTHUB_CLIENT_INVALID_ARG,            \
    IOTHUB_CLIENT_ERROR,                  \
    IOTHUB_CLIENT_INVALID_SIZE,           \
    IOTHUB_CLIENT_INDEFINITE_TIME

    /** @brief Enumeration specifying the status of calls to IoT Hub client.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

    /** @brief   Signature of the callback that the application implements to receive notifications of module initiated device method calls.
    *
    *   @param   result              Result of the operation
    *   @param   responseStatus      HTTP status code returned from the module or device.  This is only valid if @c result is @c IOTHUB_CLIENT_OK.
    *   @param   responsePayload     HTTP response payload returned from the module or device.  This is only valid if @c result is @c IOTHUB_CLIENT_OK.
    *   @param   responsePayloadSize Number of bytes in @c responsePayload.  This is only valid if @c result is @c IOTHUB_CLIENT_OK.
    *   @param   context             Context value passed is initial call to IoTHubModuleClient_ModuleMethodInvokeAsync() (e.g.).
    *
    *   @remarks Module clients when hosted in IoT Edge may themselves invoke methods on either modules on the same IoT Edge device
    *            or on downstream devices using APIs such as IoTHubModuleClient_LL_DeviceMethodInvoke() or IoTHubModuleClient_LL_ModuleMethodInvoke().
    *            These APIs operate asynchronously.  When the invoked module or device returns (or times out), the IoT Hub SDK will invoke the 
    *            application's IOTHUB_METHOD_INVOKE_CALLBACK() callback.
    *
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

    /** @brief Enumeration specifying the retry strategy the IoT Hub client should use.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUBTRANSPORT_CONFIG_TAG is internal only and should not be documented. */
    struct IOTHUBTRANSPORT_CONFIG_TAG;
    typedef struct IOTHUBTRANSPORT_CONFIG_TAG IOTHUBTRANSPORT_CONFIG;
    /**  \endcond */

#define IOTHUB_CLIENT_STATUS_VALUES       \
    IOTHUB_CLIENT_SEND_STATUS_IDLE,       \
    IOTHUB_CLIENT_SEND_STATUS_BUSY

    /** @brief Enumeration returned by the GetSendStatus family of APIs (e.g. IoTHubDeviceClient_LL_GetSendStatus())
    *           to indicate the current sending status of the IoT Hub client.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUB_IDENTITY_TYPE and IOTHUB_IDENTITY_TYPE are internal only and should not be documented. */
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

    /** @brief Enumeration returned by application callbacks that receive cloud-to-device messages.
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
    *          initiated by APIs in the SendEventAsync family (e.g. IoTHubDeviceClient_LL_SendEventAsync()).
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_VALUES             \
    IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,                \
    IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED               \

    /** @brief Enumeration passed to the application callback indicating connection status changes
    *          to IoT Hub.
    *   @remark @c IOTHUB_CLIENT_CONNECTION_AUTHENTICATED means the device or module client is ready
    *           to communicate with the Azure IoT Hub, being both connected and authenticated.
    *           @c IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED means the device or module client is not
    *           ready to communicate with the Azure IoT Hub. See the corresponding value of
    *           @c IOTHUB_CLIENT_CONNECTION_STATUS_REASON for more details.
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
    IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE,             \
    IOTHUB_CLIENT_CONNECTION_QUOTA_EXCEEDED                \

    /** @brief Enumeration passed to the application callback indicating reason that connection was unsuccessful.
    *   @remark Each of the values of @c IOTHUB_CLIENT_CONNECTION_STATUS_REASON has a specific semantics
    *           depending on the transport protocol used. Here is a brief description of the reasons for each
    *           transport protocol:
    *           @c IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN
    *               MQTT: The SAS token used in the current MQTT connection is expired
    *                     and the client must reconnect with a new SAS token.
    *               AMQP: Not applicable.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED
    *               MQTT: An MQTT CONNECT to the Azure IoT Hub was rejected.
    *               AMQP: Not applicable.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
    *               MQTT: Not applicable.
    *               AMQP: A SAS-based authentication request failed.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED
    *               MQTT: The MQTT transport has made its maximum number of attempts to reconnect
    *                     to the Azure IoT Hub and it will no longer try.
    *               AMQP: The AMQP transport has made its maximum number of attempts to reconnect
    *                     to the Azure IoT Hub and it will no longer try.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    *               MQTT: An MQTT CONNECT packet failed to be sent to the Azure IoT Hub for any reason.
    *               AMQP: A network connection issue was detected, which includes socket errors,
    *                     failures on AMQP ATTACH to CBS link (for authentication).
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR
    *               MQTT: A telemetry message timed out receiving a PUBACK from the Azure IoT Hub,
    *                     there was an error sending a PUBACK or PUBREC to Azure IoT Hub, or
    *                     an I/O error occurred when using WebSockets.
    *               AMQP: Authentication request timed out, received an unexpected link DETACH from
    *                     Azure IoT Hub, or a link ATTACH timed out.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_OK
    *               Applicable to all transport protocols.
    *               The Azure IoT C SDK client is connected and ready to
    *               communicate with the Azure IoT Hub. This reason is applicable only to
    *               @c IOTHUB_CLIENT_CONNECTION_AUTHENTICATED.
    *           @c IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE
    *               MQTT: The MQTT transport timed out waiting for a ping response from the Azure IoT Hub.
    *               AMQP: Not applicable.
    *               HTTP: Not applicable.
    *           @c IOTHUB_CLIENT_CONNECTION_QUOTA_EXCEEDED
    *               MQTT: Not applicable.
    *               AMQP: The Azure IoT Hub rejected a telemetry message because the maximum daily
    *                     quota of telemetry messages has been reached.
    *               HTTP: Not applicable.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

    /** @brief Enumeration passed to IoT Hub device and module client property 
    *          update callbacks to indicate whether or not the complete set of properties has arrived
    *          or just a partial set.
    */
#define IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE_VALUES \
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, \
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES

    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE, IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE_VALUES);

    /**  \cond DO_NOT_DOCUMENT */
    /* TRANSPORT_TYPE is internal only and should not be documented. */
#define TRANSPORT_TYPE_VALUES \
    TRANSPORT_LL, /*LL comes from "LowLevel" */ \
    TRANSPORT_THREADED

    MU_DEFINE_ENUM_WITHOUT_INVALID(TRANSPORT_TYPE, TRANSPORT_TYPE_VALUES);
    /**  \endcond */


#define DEVICE_TWIN_UPDATE_STATE_VALUES \
    DEVICE_TWIN_UPDATE_COMPLETE, \
    DEVICE_TWIN_UPDATE_PARTIAL

    /** @brief Enumeration passed to application callback to receive IoT Hub device or module twin data indicating whether the full twin or just partial update was received.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

    /** 
    *   @brief Signature of the callback that the application implements to process acknowledgement or failures when sending telemetry to IoT Hub.
    *
    *   @param result                 Result of the telemetry send operation.
    *   @param userContextCallback    Context that application specified during initial API send call.
    */
    typedef void(*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);

    /** 
    *   @brief Signature of the callback that the application implements to process connection status changes between device and IoT Hub.
    *
    *   @param result                  Whether device is successfully connected (@c IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) or not (@c IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED).
    *   @param reason                  More information about @c result, especially if connection was unsuccessful.
    *   @param userContextCallback     Context that application specified during initial API call to receive status change notifications.
    */
    typedef void(*IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);

    /** 
    *   @brief Signature of the callback that the application implements to process incoming cloud-to-device messages.
    *
    *   @param message                 The incoming message received from IoT Hub.
    *   @param userContextCallback     Context that application specified during initial API call to receive incoming cloud-to-device messages.
    *
    *   @return #IOTHUBMESSAGE_DISPOSITION_RESULT indicating how client has acknowledged the incoming cloud-to-device message.
    */
    typedef IOTHUBMESSAGE_DISPOSITION_RESULT (*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);

    /** 
    *   @brief   Signature of the callback that the application implements to process data received from an IoT Hub device or module twin.
    *
    *   @param   update_state          Whether application has received a full twin or just a patch update.
    *   @param   payLoad               Payload of twin received.
    *   @param   size                  Number of bytes in @c payload.
    *   @param   userContextCallback   Context that application specified during initial API call to receive twin data.
    *
    *   @warning The data in @c payLoad is not guaranteed to be a null-terminated string.
    */
    typedef void(*IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);

    /** 
    *   @brief Signature of the callback that the application implements to receive notifications of device or module initiated twin updates.
    *
    *   @param status_code             HTTP style status code indicating success or failure of operation.
    *   @param userContextCallback     Context that application specified during initial API call to update twin data.
    */
    typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);

    /** 
    *   @brief   Signature of the callback that the application implements to receive incoming device or module method invocations from IoT Hub
    *
    *   @param   method_name           Name of method being invoked.
    *   @param   payload               Request payload received from IoT Hub.
    *   @param   size                  Number of bytes in @c payload.
    *   @param   response              Response of the request, as specified by the application.  This should NOT include the null-terminator.
    *   @param   response_size         Number of bytes application specifies in @c response.
    *   @param   userContextCallback   Context that application specified during initial API call to receive device or module method calls.
    *  
    *   @remarks The application should allocate @c response with @c malloc.  The IoT Hub client SDK will @c free the data automatically.
    *
    *   @warning The data in @c payload is not guaranteed to be a null-terminated string.
    *
    *   @return  HTTP style return code to indicate success or failure of the method call.
    */
    typedef int(*IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);

    /**  \cond DO_NOT_DOCUMENT */
    /* IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK is internal only and should not be documented. */
    typedef int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback);
    /**  \endcond */


#define IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES \
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_OK, \
    IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_ABORT

    /** @brief           Return value applications use in their implementation of IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX() to indicate status.
    */
    MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT_VALUES);

    /** @brief           Deprecated callback mechanism for uploading data to a blob.
    *   @deprecated      This callback type is deprecated.  Use the callback IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX() instead.
    */
    typedef void(*IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK)(IOTHUB_CLIENT_FILE_UPLOAD_RESULT result, unsigned char const ** data, size_t* size, void* context);

    /**
    *  @brief           Signature of the callback that the application implements to process IoT Hub client SDK requesting additional chunks to upload to blob.
    *
    *  @param result    The result of the upload of the previous block of data provided by the user.
    *  @param data      Next block of data to be uploaded, to be provided by the user when this callback is invoked.
    *  @param size      Size of the data parameter.
    *  @param context   User context provided on the call to IoTHubClient_UploadMultipleBlocksToBlobAsync.
    *
    *  @return          #IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_RESULT indicating whether the application is returning data to be sent or not.
    *
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
        /** @brief A function pointer that is passed into IoTHubDeviceClient_CreateWithTransport() or IoTHubDeviceClient_LL_Create().
        *   A function definition for AMQP is defined in the include @c iothubtransportamqp.h.
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

        /** @brief    Optional gateway host to connect to (instead of directly to IoT Hub).  Can be NULL. */
        const char* protocolGatewayHostName;
    } IOTHUB_CLIENT_CONFIG;

    /** @brief    This struct specifies  IoT Hub client device configuration. */
    typedef struct IOTHUB_CLIENT_DEVICE_CONFIG_TAG
    {
        /** @brief A function pointer that is passed into the @c IoTHubClientCreate API.
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

    /** @brief Current version of @p IOTHUB_CLIENT_COMMAND_REQUEST structure.  */
    #define IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1 1

    /** @brief    This struct specifies parameters of an incoming command request */  
    typedef struct IOTHUB_CLIENT_COMMAND_REQUEST_TAG
    {
        /** @brief   Version of the structure.  Currently the SDK sets this to IOTHUB_CLIENT_COMMAND_REQUEST_STRUCT_VERSION_1. */
        int structVersion;
        /** @brief   Name of the component associated with this request.  If this is targeting the root component, this will be NULL. */
        const char* componentName;
        /** @brief   Name of the command associated with this request. */
        const char* commandName;
        /** @brief    Raw payload of the request.  This is NOT guaranteed to be a \0 terminated string. */
        const unsigned char* payload;
        /** @brief    Number of bytes of @p payload. */
        size_t payloadLength;
    } IOTHUB_CLIENT_COMMAND_REQUEST;

    /** @brief Current version of @p #IOTHUB_CLIENT_COMMAND_RESPONSE structure.  */
    #define IOTHUB_CLIENT_COMMAND_RESPONSE_STRUCT_VERSION_1 1

    /** @brief    This struct specifies parameters of an outgoing command response */  
    typedef struct IOTHUB_CLIENT_COMMAND_RESPONSE_TAG
    {
        /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_COMMAND_RESPONSE_STRUCT_VERSION_1. */
        int structVersion;
        /** @brief Response generated by the callback that will be sent to IoT Hub.  This must be allocated using malloc().  
                   The SDK takes ownership of the buffer on callback return and will free() it when the SDK is done with it. */
        unsigned char* payload;
        /** @brief Number of bytes application is returning in payload.  This should *not* include the terminating null. */
        size_t payloadLength;
        /** @brief Status code of the command to return to the IoT Hub service.  This maps to an HTTP style status code. */
        int statusCode;
    } IOTHUB_CLIENT_COMMAND_RESPONSE;

    /**
    * @brief    Function callback application implements to process an incoming command from IoT Hub.
    *
    * @param[in]    commandRequest        Parameters specified by the service application initiating command request.
    * @param[out]   commandResponse       Response filled in by application callback to the command request.
    * @param[in]    userContextCallback   User context pointer set in initial call to @p IoTHubDeviceClient_LL_SubscribeToCommands().
    */
    typedef void(*IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC)(
                const IOTHUB_CLIENT_COMMAND_REQUEST* commandRequest,
                IOTHUB_CLIENT_COMMAND_RESPONSE* commandResponse,    
                void* userContextCallback);

    /**
    * @brief    Function callback application implements to receive notifications when IoT Hub acknowledges telemetry.
    *
    * @param[in]    result                Result of operation sending to IoT Hub.
    * @param[in]    userContextCallback   Optional user specified context set in call that initiated the message send (for example, 
    *                                     @p IoTHubDeviceClient_LL_SendTelemetryAsync()).
    */
    typedef void(*IOTHUB_CLIENT_TELEMETRY_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);
    
    /**
    * @brief    Function callback application implements to receive acknowledgements of properties sent from the client to IoT Hub.  
    *
    * @param[in]   statusCode             Status code IoT Hub returns when receiving a property or writable property response from the device.  
    *                                     This corresponds to HTTP status codes.
    * @param[in]   userContextCallback    Optional user specified context set in the call that sent the properties (for example,
    *                                     @p IoTHubDeviceClient_LL_SendPropertiesAsync()).
    * 
    */
    typedef void(*IOTHUB_CLIENT_PROPERTY_ACKNOWLEDGED_CALLBACK)(int statusCode, void* userContextCallback);

    /**
    * @brief    Function callback application implements to process properties received from IoT Hub.
    *
    * @param[in]   payloadType            Whether the payload contains all properties from IoT Hub or only the ones that have just been updated.
    * @param[in]   payload                Raw payload of the request.  This is NOT guaranteed to be a \0 terminated string.
    * @param[in]   payloadLength          Number of bytes of @p payload.
    * @param[in]   userContextCallback    User context pointer set in initial call to retrieving the properties (for example, 
    *                                     @p IoTHubDeviceClient_LL_GetPropertiesAsync() or @p IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync()).
    */
    typedef void(*IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK)(
                     IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, 
                     const unsigned char* payload,
                     size_t payloadLength,
                     void* userContextCallback);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_CORE_COMMON_H */
