// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_module_client_ll.h
*    @brief     APIs that allow a user (usually a module) to communicate
*             with an Azure IoTHub.
*
*    @details IoTHubModuleClient_LL is a module that allows a user (usually a
*             module) to communicate with an Azure IoTHub. It can send events
*             and receive messages. At any given moment in time there can only
*             be at most 1 message callback function.
*
*             This API surface contains a set of APIs that allows the user to
*             interact with the lower layer portion of the IoTHubClient. These APIs
*             contain @c _LL_ in their name, but retain the same functionality like the
*             @c IoTHubModuleClient_... APIs, with one difference. If the @c _LL_ APIs are
*             used then the user is responsible for scheduling when the actual work done
*             by the IoTHubClient happens (when the data is sent/received on/from the wire).
*             This is useful for constrained devices where spinning a separate thread is
*             often not desired.
*/

#ifndef IOTHUB_MODULE_CLIENT_LL_H
#define IOTHUB_MODULE_CLIENT_LL_H

#include <time.h>
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"

#include "iothub_transport_ll.h"
#include "iothub_client_core_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct IOTHUB_MODULE_CLIENT_LL_HANDLE_DATA_TAG* IOTHUB_MODULE_CLIENT_LL_HANDLE;

#ifdef __cplusplus
}
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
    * @brief    Creates a IoT Hub client for communication with an existing
    *             IoT Hub using the specified connection string parameter.
    *
    * @param    connectionString    Pointer to a character string
    * @param    protocol            Function pointer for protocol implementation
    *
    *            Sample connection string:
    *                <blockquote>
    *                    <pre>HostName=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net];DeviceId=[Device ID goes here];SharedAccessKey=[Device key goes here];ModuleId=[Module ID goes here]</pre>
    *                </blockquote>
    *
    * @return    A non-NULL @c IOTHUB_MODULE_CLIENT_LL_HANDLE value that is used when
    *             invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_MODULE_CLIENT_LL_HANDLE, IoTHubModuleClient_LL_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Disposes of resources allocated by the IoT Hub client. This is a
    *             blocking call.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    */
     MOCKABLE_FUNCTION(, void, IoTHubModuleClient_LL_Destroy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubModuleClientHandle         The handle created by a call to the create function.
    * @param    eventMessageHandle               The handle to an IoT Hub message.
    * @param    eventConfirmationCallback        The callback specified by the module for receiving
    *                                            confirmation of the delivery of the IoT Hub message.
    *                                            This callback can be expected to invoke the
    *                                            ::IoTHubModuleClient_LL_SendEventAsync function for the
    *                                            same message in an attempt to retry sending a failing
    *                                            message. The user can specify a @c NULL value here to
    *                                            indicate that no callback is required.
    * @param    userContextCallback              User specified context that will be provided to the
    *                                            callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendEventAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief    This function returns the current sending status for IoTHubClient.
    *
    * @param    iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param    iotHubClientStatus        The sending state is populated at the address pointed
    *                                     at by this parameter. The value will be set to
    *                                     @c IOTHUBCLIENT_SENDSTATUS_IDLE if there is currently
    *                                     no item to be sent and @c IOTHUBCLIENT_SENDSTATUS_BUSY
    *                                     if there are.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetSendStatus, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);

    /**
    * @brief    Sets up the message callback to be invoked when Edge issues a
    *             message to the module. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    messageCallback               The callback specified by the module for receiving
    *                                         messages from IoT Hub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetMessageCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    connectionStatusCallback      The callback specified by the module for receiving
    *                                         updates about the status of the connection to IoT Hub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetConnectionStatusCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    retryPolicy                   The policy to use to reconnect to IoT Hub when a
    *                                         connection drops.
    * @param    retryTimeoutLimitInSeconds    Maximum amount of time(seconds) to attempt reconnection when a
    *                                         connection drops to IOT Hub.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetRetryPolicy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);


    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    retryPolicy                   Out parameter containing the policy to use to reconnect to IoT Hub.
    * @param    retryTimeoutLimitInSeconds    Out parameter containing maximum amount of time in seconds to attempt reconnection
                                              to IOT Hub.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetRetryPolicy, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);

    /**
    * @brief    This function returns in the out parameter @p lastMessageReceiveTime
    *             what was the value of the @c time function when the last message was
    *             received at the client.
    *
    * @param    iotHubModuleClientHandle       The handle created by a call to the create function.
    * @param    lastMessageReceiveTime         Out parameter containing the value of @c time function
    *                                          when the last message was received.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_GetLastMessageReceiveTime, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, time_t*, lastMessageReceiveTime);

    /**
    * @brief    This function is meant to be called by the user when work
    *             (sending/receiving) can be done by the IoTHubClient.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    *
    *            All IoTHubClient interactions (in regards to network traffic
    *            and/or user level callbacks) are the effect of calling this
    *            function and they take place synchronously inside _DoWork.
    */
     MOCKABLE_FUNCTION(, void, IoTHubModuleClient_LL_DoWork, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle);

    /**
    * @brief    This API sets a runtime option identified by parameter @p optionName
    *             to a value pointed to by @p value. @p optionName and the data type
    *             @p value is pointing to are specific for every option.
    *
    * @param    iotHubModuleClientHandle    The handle created by a call to the create function.
    * @param    optionName                  Name of the option.
    * @param    value                       The value.
    *
    *            The options that can be set via this API are:
    *                - @b timeout - the maximum time in milliseconds a communication is
    *                  allowed to use. @p value is a pointer to an @c unsigned @c int with
    *                  the timeout value in milliseconds. This is only supported for the HTTP
    *                  protocol as of now. When the HTTP protocol uses CURL, the meaning of
    *                  the parameter is <em>total request time</em>. When the HTTP protocol uses
    *                  winhttp, the meaning is the same as the @c dwSendTimeout and
    *                  @c dwReceiveTimeout parameters of the
    *                  <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/aa384116(v=vs.85).aspx">
    *                  WinHttpSetTimeouts</a> API.
    *                - @b CURLOPT_LOW_SPEED_LIMIT - only available for HTTP protocol and only
    *                  when CURL is used. It has the same meaning as CURL's option with the same
    *                  name. @p value is pointer to a long.
    *                - @b CURLOPT_LOW_SPEED_TIME - only available for HTTP protocol and only
    *                  when CURL is used. It has the same meaning as CURL's option with the same
    *                  name. @p value is pointer to a long.
    *                - @b CURLOPT_FORBID_REUSE - only available for HTTP protocol and only
    *                  when CURL is used. It has the same meaning as CURL's option with the same
    *                  name. @p value is pointer to a long.
    *                - @b CURLOPT_FRESH_CONNECT - only available for HTTP protocol and only
    *                  when CURL is used. It has the same meaning as CURL's option with the same
    *                  name. @p value is pointer to a long.
    *                - @b CURLOPT_VERBOSE - only available for HTTP protocol and only
    *                  when CURL is used. It has the same meaning as CURL's option with the same
    *                  name. @p value is pointer to a long.
    *                - @b keepalive - available for MQTT protocol.  Integer value that sets the
    *                  interval in seconds when pings are sent to the server.
    *                - @b logtrace - available for MQTT protocol.  Boolean value that turns on and
    *                  off the diagnostic logging.
    *                - @b sas_token_lifetime - available for MQTT & AMQP protocol.  size_t value that that determines the
    *                  sas token timeout length.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetOption, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, optionName, const void*, value);

    /**
    * @brief    This API specifies a call back to be used when the module receives a desired state update.
    *
    * @param    iotHubModuleClientHandle  The handle created by a call to the create function.
    * @param    moduleTwinCallback        The callback specified by the module client to be used for updating
    *                                     the desired state. The callback will be called in response to patch
    *                                     request send by the IoTHub services. The payload will be passed to the
    *                                     callback, along with two version numbers:
    *                                        - Desired:
    *                                        - LastSeenReported:
    * @param    userContextCallback       User specified context that will be provided to the
    *                                     callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetModuleTwinCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, moduleTwinCallback, void*, userContextCallback);

    /**
    * @brief    This API sneds a report of the module's properties and their current values.
    *
    * @param    iotHubModuleClientHandle The handle created by a call to the create function.
    * @param    reportedState            The current module property values to be 'reported' to the IoTHub.
    * @param    reportedStateCallback    The callback specified by the module client to be called with the
    *                                    result of the transaction.
    * @param    userContextCallback      User specified context that will be provided to the
    *                                    callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubModuleClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendReportedState, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);
     /**
     * @brief    This API sets callback for async cloud to module method call.
     *
     * @param    iotHubModuleClientHandle        The handle created by a call to the create function.
     * @param    moduleMethodCallback            The callback which will be called by IoTHub.
     * @param    userContextCallback             User specified context that will be provided to the
     *                                           callback. This can be @c NULL.
     *
     * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetModuleMethodCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, moduleMethodCallback, void*, userContextCallback);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    eventMessageHandle            The handle to an IoT Hub message.
    * @param    outputName                    The name of the queue to send the message to.
    * @param    eventConfirmationCallback     The callback specified by the module for receiving
    *                                         confirmation of the delivery of the IoT Hub message.
    *                                         This callback can be expected to invoke the
    *                                         ::IoTHubClient_LL_SendEventAsync function for the
    *                                         same message in an attempt to retry sending a failing
    *                                         message. The user can specify a @c NULL value here to
    *                                         indicate that no callback is required.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    *            @b NOTE: The application behavior is undefined if the user calls
    *            the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SendEventToOutputAsync, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, const char*, outputName, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief    This API sets callback for  method call that is directed to specified 'inputName' queue (e.g. messages from IoTHubClient_SendEventToOutputAsync)
    *
    * @param    iotHubModuleClientHandle      The handle created by a call to the create function.
    * @param    inputName                     The name of the queue to listen on for this moduleMethodCallback/userContextCallback.
    * @param    eventHandlerCallback          The callback which will be called by IoTHub.
    * @param    userContextCallback           User specified context that will be provided to the
    *                                         callback. This can be @c NULL.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_SetInputMessageCallback, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, inputName, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, eventHandlerCallback, void*, userContextCallback);

#ifdef USE_EDGE_MODULES

    /**
    * @brief    This API creates a module handle based on environment variables set in the Edge runtime.
    *           NOTE: It is *ONLY* valid when the code is running in a container initiated by Edge.
    *
    * @param    protocol            Function pointer for protocol implementation
    *
    * @return   A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_MODULE_CLIENT_LL_HANDLE, IoTHubModuleClient_LL_CreateFromEnvironment, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /*
    * @brief    This API invokes a device method on a specified device
    *
    * @param    iotHubModuleClientHandle        The handle created by a call to a create function
    * @param    deviceId                        The device id of the device to invoke a method on
    * @param    methodName                      The name of the method
    * @param    methodPayload                   The method payload (in json format)
    * @param    timeout                         The time in seconds before a timeout occurs
    * @param    responseStatus                  This pointer will be filled with the response status after invoking the device method
    * @param    responsePayload                 This pointer will be filled with the response payload
    * @param    responsePayloadSize             This pointer will be filled with the response payload size
    *
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_DeviceMethodInvoke, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, deviceId, const char*, methodName, const char*, methodPayload, unsigned int, timeout, int*, responseStatus, unsigned char**, responsePayload, size_t*, responsePayloadSize);

    /*
    * @brief    This API invokes a module method on a specified module
    *
    * @param    iotHubModuleClientHandle        The handle created by a call to a create function
    * @param    deviceId                        The device id of the device to invoke a method on
    * @param    moduleId                        The module id of the module to invoke a method on
    * @param    methodName                      The name of the method
    * @param    methodPayload                   The method payload (in json format)
    * @param    timeout                         The time in seconds before a timeout occurs
    * @param    responseStatus                  This pointer will be filled with the response status after invoking the module method
    * @param    responsePayload                 This pointer will be filled with the response payload
    * @param    responsePayloadSize             This pointer will be filled with the response payload size
    *
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubModuleClient_LL_ModuleMethodInvoke, IOTHUB_MODULE_CLIENT_LL_HANDLE, iotHubModuleClientHandle, const char*, deviceId, const char*, moduleId, const char*, methodName, const char*, methodPayload, unsigned int, timeout, int*, responseStatus, unsigned char**, responsePayload, size_t*, responsePayloadSize);

#endif /*USE_EDGE_MODULES*/

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_MODULE_CLIENT_LL_H */
