// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_device_client_ll.h
*    @brief     APIs that allow a user (usually a device) to communicate
*             with an Azure IoT Hub.
*
*    @details IoTHubDeviceClient_LL allows a user (usually a
*             device) to communicate with an Azure IoT Hub. It can send events
*             and receive messages. At any given moment in time there can only
*             be at most 1 message callback function.
*
*             This API surface contains a set of APIs that allows the user to
*             interact with the lower layer portion of the IoTHubClient. These APIs
*             contain @c _LL_ in their name, but retain the same functionality like the
*             @c IoTHubDeviceClient_... APIs, with one difference. If the @c _LL_ APIs are
*             used then the user is responsible for scheduling when the actual work done
*             by the IoTHubClient happens (when the data is sent/received on/from the network).
*             This is useful for constrained devices where spinning a separate thread is
*             often not desired.
*    
*     @warning IoTHubDeviceClient_LL_* functions are NOT thread safe.  See
*              https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/threading_notes.md for more details.
* 
*/

#ifndef IOTHUB_DEVICE_CLIENT_LL_H
#define IOTHUB_DEVICE_CLIENT_LL_H

#include <stddef.h>
#include <stdint.h>

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "iothub_transport_ll.h"
#include "iothub_client_core_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 
*  @brief   Handle corresponding to a lower layer (LL) device client instance.
*
*  @warning The API functions that use this handle are not thread safe.  See https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/threading_notes.md for more details.
*/
typedef struct IOTHUB_CLIENT_CORE_LL_HANDLE_DATA_TAG* IOTHUB_DEVICE_CLIENT_LL_HANDLE;

#ifndef DONT_USE_UPLOADTOBLOB
/** 
*  @brief   Handle for Upload-to-Blob API Functions.
*/
typedef IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE;
#endif // DONT_USE_UPLOADTOBLOB



    /**
    * @brief    Creates a IoT Hub client for communication with an existing
    *           IoT Hub using the specified connection string parameter.
    *
    * @param    connectionString    Pointer to a character string
    * @param    protocol            Function pointer for protocol implementation
    *
    *            Sample connection string:
    *                <blockquote>
    *                    <pre>HostName=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net];DeviceId=[Device ID goes here];SharedAccessKey=[Device key goes here];</pre>
    *                </blockquote>
    *
    * @return    A non-NULL #IOTHUB_DEVICE_CLIENT_LL_HANDLE value that is used when
    *             invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_LL_HANDLE, IoTHubDeviceClient_LL_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Creates a IoT Hub client for communication with an existing IoT
    *           Hub using the specified parameters.
    *
    * @param    config    Pointer to an #IOTHUB_CLIENT_CONFIG structure
    *
    *           The API does not allow sharing of a connection across multiple
    *           devices. This is a blocking call.
    *
    * @return    A non-NULL #IOTHUB_DEVICE_CLIENT_LL_HANDLE value that is used when
    *            invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_LL_HANDLE, IoTHubDeviceClient_LL_Create, const IOTHUB_CLIENT_CONFIG*, config);

    /**
    * @brief    Creates a IoT Hub client for communication with an existing IoT
    *           Hub using an existing transport.
    *
    * @param    config    Pointer to an #IOTHUB_CLIENT_DEVICE_CONFIG structure
    *
    *           The API *allows* sharing of a connection across multiple
    *           devices. This is a blocking call.
    *
    * @return   A non-NULL #IOTHUB_DEVICE_CLIENT_LL_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_LL_HANDLE, IoTHubDeviceClient_LL_CreateWithTransport, const IOTHUB_CLIENT_DEVICE_CONFIG*, config);

     /**
     * @brief    Creates a IoT Hub client for communication with an existing IoT
     *           Hub using the device auth.
     *
     * @param    iothub_uri             Pointer to an IoT Hub hostname received in the registration process
     * @param    device_id              Pointer to the device Id of the device
     * @param    protocol               Function pointer for protocol implementation
     *
     * @return   A non-NULL #IOTHUB_DEVICE_CLIENT_LL_HANDLE value that is used when
     *           invoking other functions for IoT Hub client and @c NULL on failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_LL_HANDLE, IoTHubDeviceClient_LL_CreateFromDeviceAuth, const char*, iothub_uri, const char*, device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Disposes of resources allocated by the IoT Hub client. This is a
    *           blocking call.
    *
    * @param    iotHubClientHandle    The handle created by a call to the create function.
    *
    * @warning  Do not call this function from inside any application callbacks from this SDK, e.g. your IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK handler.
    */
     MOCKABLE_FUNCTION(, void, IoTHubDeviceClient_LL_Destroy, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    eventMessageHandle              The handle to an IoT Hub message.
    * @param    eventConfirmationCallback       The callback specified by the device for receiving
    *                                           confirmation of the delivery of the IoT Hub message.
    *                                           This callback can be expected to invoke the
    *                                           IoTHubDeviceClient_LL_SendEventAsync function for the
    *                                           same message in an attempt to retry sending a failing
    *                                           message. The user can specify a @c NULL value here to
    *                                           indicate that no callback is required.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @remarks
    *           The IOTHUB_MESSAGE_HANDLE instance provided as argument is copied by the function,
    *           so this argument can be destroyed by the calling application right after IoTHubDeviceClient_LL_SendEventAsync returns.
    *           The copy of @c eventMessageHandle is later destroyed by the iothub client when the message is effectively sent, if a failure sending it occurs, or if the client is destroyed.
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SendEventAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief    This function returns the current sending status for IoTHubClient.
    *
    * @param    iotHubClientHandle        The handle created by a call to the create function.
    * @param    iotHubClientStatus        The sending state is populated at the address pointed
    *                                     at by this parameter. The value will be set to
    *                                     @c IOTHUB_CLIENT_SEND_STATUS_IDLE if there is currently
    *                                     no item to be sent and @c IOTHUB_CLIENT_SEND_STATUS_BUSY
    *                                     if there are.
    *
    * @remark    Does not return information related to uploads initiated by IoTHubDeviceClient_LL_UploadToBlob() or IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob().
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetSendStatus, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);

    /**
    * @brief    Sets up the message callback to be invoked when IoT Hub issues a
    *           message to the device. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    messageCallback                 The callback specified by the device for receiving
    *                                           messages from IoT Hub.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetMessageCallback, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    connectionStatusCallback        The callback specified by the device for receiving
    *                                           updates about the status of the connection to IoT Hub.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @remark   Callback specified will not receive connection status change notifications for upload connections created with IoTHubDeviceClient_LL_UploadToBlob() or IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob().
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetConnectionStatusCallback, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    retryPolicy                     The policy to use to reconnect to IoT Hub when a
    *                                           connection drops.
    * @param    retryTimeoutLimitInSeconds      Maximum amount of time(seconds) to attempt reconnection when a
    *                                           connection drops to IOT Hub.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @remark   Uploads initiated by IoTHubDeviceClient_LL_UploadToBlob() or IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob() do not have automatic retries and do not honor the retryPolicy settings.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetRetryPolicy, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);


    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle                  The handle created by a call to the create function.
    * @param    retryPolicy                         Out parameter containing the policy to use to reconnect to IoT Hub.
    * @param    retryTimeoutLimitInSeconds          Out parameter containing maximum amount of time in seconds to attempt reconnection
                                                    to IOT Hub.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetRetryPolicy, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);

    /**
    * @brief    This function returns in the out parameter @p lastMessageReceiveTime
    *           what was the value of the @c time function when the last message was
    *           received at the client.
    *
    * @param    iotHubClientHandle          The handle created by a call to the create function.
    * @param    lastMessageReceiveTime      Out parameter containing the value of @c time function
    *                                       when the last message was received.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetLastMessageReceiveTime, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, time_t*, lastMessageReceiveTime);

    /**
    * @brief    This function MUST be called by the user so work (sending/receiving data on the network,
    *           computing and enforcing timeout controls, managing the connection to the IoT Hub) can
    *           be done by the IoTHubClient.
    *           The recommended call frequency is at least once every 100 milliseconds.
    *
    * @param    iotHubClientHandle    The handle created by a call to the create function.
    *
    *           All IoTHubClient interactions (in regards to network traffic
    *           and/or user level callbacks) are the effect of calling this
    *           function and they take place synchronously inside _DoWork.
    *
    * @warning  Do not call this function from inside any application callbacks from this SDK, e.g. your IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK handler.
    */
     MOCKABLE_FUNCTION(, void, IoTHubDeviceClient_LL_DoWork, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle);

    /**
    * @brief    This API sets a runtime option identified by parameter @p optionName
    *           to a value pointed to by @p value. @p optionName and the data type
    *           @p value is pointing to are specific for every option.
    *
    * @param    iotHubClientHandle      The handle created by a call to the create function.
    * @param    optionName              Name of the option.
    * @param    value                   The value.
    *
    * @remarks  Documentation for configuration options is available at https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/Iothub_sdk_options.md.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetOption, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, optionName, const void*, value);

    /**
    * @brief   This API specifies a callback to be used when the device receives a desired state update.
    *
    * @param   iotHubClientHandle        The handle created by a call to the create function.
    * @param   deviceTwinCallback        The callback specified by the device client to be used for updating
    *                                    the desired state. The callback will be called in response to patch
    *                                    request send by the IoT Hub services. The payload will be passed to the
    *                                    callback, along with two version numbers:
    *                                        - Desired:
    *                                        - LastSeenReported:
    * @param   userContextCallback       User specified context that will be provided to the
    *                                    callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetDeviceTwinCallback, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

    /**
    * @brief    This API sends a report of the device's properties and their current values.
    *
    * @param    iotHubClientHandle        The handle created by a call to the create function.
    * @param    reportedState             The current device property values to be 'reported' to the IoT Hub.
    * @param    size                      Number of bytes in @c reportedState.
    * @param    reportedStateCallback     The callback specified by the device client to be called with the
    *                                     result of the transaction.
    * @param    userContextCallback       User specified context that will be provided to the
    *                                     callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SendReportedState, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);

     /**
     * @brief	This API enabled the device to request the full device twin (with all the desired and reported properties) on demand.
     *
     * @param	iotHubClientHandle		The handle created by a call to the create function.
     * @param	deviceTwinCallback	    The callback specified by the device client to receive the Twin document.
     * @param	userContextCallback		User specified context that will be provided to the
     * 									callback. This can be @c NULL.
     *
     * @warning: Do not call IoTHubDeviceClient_LL_Destroy() or IoTHubDeviceClient_LL_DoWork() from inside your application's callback.
     *
     * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetTwinAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

     /**
     * @brief    This API sets the callback for async cloud to device method calls.
     *
     * @param    iotHubClientHandle                 The handle created by a call to the create function.
     * @param    deviceMethodCallback               The callback which will be called by IoT Hub.
     * @param    userContextCallback                User specified context that will be provided to the
     *                                              callback. This can be @c NULL.
     *
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SetDeviceMethodCallback, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, deviceMethodCallback, void*, userContextCallback);

     /**
     * @brief    This API responds to an asnyc method callback identified the methodId.
     *
     * @param    iotHubClientHandle      The handle created by a call to the create function.
     * @param    methodId                The methodId of the Device Method callback.
     * @param    response                The response data for the method callback.
     * @param    respSize                The size of the response data buffer.
     * @param    statusCode              The status response of the method callback.
     *
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_DeviceMethodResponse, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, respSize, int, statusCode);

#ifndef DONT_USE_UPLOADTOBLOB
    /**
    * @brief    This API uploads to Azure Storage the content pointed to by @p source having the size @p size
    *           under the blob name @p devicename/destinationFileName
    *
    * @param    iotHubClientHandle      The handle created by a call to the create function.
    * @param    destinationFileName     name of the file.
    * @param    source                  pointer to the source for file content (can be NULL)
    * @param    size                    the size of the source in memory (if @p source is NULL then size needs to be 0).
    * 
    * @warning  Other _LL_ functions such as IoTHubDeviceClient_LL_SendEventAsync() queue work to be performed later and do not block.  IoTHubDeviceClient_LL_UploadToBlob
    *           will block however until the upload is completed or fails, which may take a while.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_UploadToBlob, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, destinationFileName, const unsigned char*, source, size_t, size);

     /**
     * @brief    This API uploads to Azure Storage the content provided block by block by @p getDataCallback
     *           under the blob name @p devicename/destinationFileName
     *
     * @param    iotHubClientHandle      The handle created by a call to the create function.
     * @param    destinationFileName     name of the file.
     * @param    getDataCallbackEx       A callback to be invoked to acquire the file chunks to be uploaded, as well as to indicate the status of the upload of the previous block.
     * @param    context                 Any data provided by the user to serve as context on getDataCallback.
     *
     * @warning  Other _LL_ functions such as IoTHubDeviceClient_LL_SendEventAsync() queue work to be performed later and do not block.  IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob
     *           will block however until the upload is completed or fails, which may take a while.
     *
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, getDataCallbackEx, void*, context);

     /**
     * @brief    This API creates a new upload within Azure IoT Hub, getting back a correlation-id and a
     *           SAS URI for the Blob access to the Azure Storage associated with the Azure IoT Hub.
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStoragePutBlock`
     *           `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`
     *           `IoTHubDeviceClient_LL_AzureStorageDestroyClient`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *
     * @param    iotHubClientHandle      The handle created by a call to the create function.
     * @param    destinationFileName     Name of the file in blob storage.
     * @param    uploadCorrelationId     Variable where to store the correlation-id of the new upload.
     * @param    azureBlobSasUri         Variable where to store the Azure Storage SAS URI for the new upload.
     *
     * @warning  This is a synchronous/blocking function.
     *           This function only attempts to send HTTP requests once, it does not retry in case of failure.
     *           Any retries must be explicitly implemented by the calling application.
     *           `uploadCorrelationId` and `azureBlobSasUri` must be freed by the calling application
     *           after the blob upload process is done (e.g., after calling `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`).
     *
     * @return   An IOTHUB_CLIENT_RESULT value indicating the success or failure of the API call.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_AzureStorageInitializeBlobUpload, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, destinationFileName, char**, uploadCorrelationId, char**, azureBlobSasUri);

     /**
     * @brief    This API creates a client for a new blob upload to Azure Storage.
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStoragePutBlock`
     *           `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`
     *           `IoTHubDeviceClient_LL_AzureStorageDestroyClient`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *
     * @param    iotHubClientHandle      The handle created by a call to the create function.
     * @param    azureBlobSasUri         The Azure Storage Blob SAS uri obtained with `IoTHubDeviceClient_LL_AzureStorageInitializeBlobUpload`.
     *
     * @warning  This is a synchronous/blocking function.
     *
     * @return   A `IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE` on success or NULL if the function fails.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE, IoTHubDeviceClient_LL_AzureStorageCreateClient, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, azureBlobSasUri);

     /**
     * @brief    This API uploads a single blob block to Azure Storage (performs a PUT BLOCK operation).
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStorageCreateClient`
     *           `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`
     *           `IoTHubDeviceClient_LL_AzureStorageDestroyClient`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *           For more information about Azure Storage PUT BLOCK, its parameters and behavior, please refer to
     *           https://learn.microsoft.com/en-us/rest/api/storageservices/put-block
     *
     * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_LL_AzureStorageCreateClient`.
     * @param    blockNumber            Number of the block being uploaded.
     * @param    dataPtr                Pointer to the block data to be uploaded to Azure Storage blob.
     * @param    dataSize               Size of the block data pointed by `dataPtr`.
     *
     * @warning  This is a synchronous/blocking function.
     *           This function only attempts to send HTTP requests once, it does not retry in case of failure.
     *           Any retries must be explicitly implemented by the calling application.
     *
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_AzureStoragePutBlock, IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle, uint32_t, blockNumber, const uint8_t*, dataPtr, size_t, dataSize);

     /**
     * @brief    This API performs an Azure Storage PUT BLOCK LIST operation.
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStorageCreateClient`
     *           `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`
     *           `IoTHubDeviceClient_LL_AzureStorageDestroyClient`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *           For more information about Azure Storage PUT BLOCK LIST, please refer to
     *           https://learn.microsoft.com/en-us/rest/api/storageservices/put-block-list
     *
     * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_LL_AzureStorageCreateClient`.
     *
     * @warning  This is a synchronous/blocking function.
     *           This function only attempts to send HTTP requests once, it does not retry in case of failure.
     *           Any retries must be explicitly implemented by the calling application.
     *
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_AzureStoragePutBlockList, IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle);

     /**
     * @brief    This API destroys an instance previously created with `IoTHubDeviceClient_LL_AzureStorageCreateClient` .
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStorageCreateClient`
     *           `IoTHubDeviceClient_LL_AzureStoragePutBlock`
     *           `IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *
     * @param    azureStorageClientHandle     The handle created with `IoTHubDeviceClient_LL_AzureStorageCreateClient`.
     *
     * @warning  This is a synchronous/blocking function.
     *
     * @return   Nothing.
     */
     MOCKABLE_FUNCTION(, void, IoTHubDeviceClient_LL_AzureStorageDestroyClient, IOTHUB_CLIENT_LL_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle);

     /**
     * @brief    This API notifies Azure IoT Hub of the upload completion.
     * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
     *           This function is expected to be used along with:
     *           `IoTHubDeviceClient_LL_AzureStorageCreateClient`
     *           `IoTHubDeviceClient_LL_AzureStoragePutBlock`
     *           `IoTHubDeviceClient_LL_AzureStorageDestroyClient`
     *           For simpler/less-granular control of uploads to Azure blob storage please use either
     *           `IoTHubDeviceClient_LL_UploadToBlob` or `IoTHubDeviceClient_LL_UploadMultipleBlocksToBlob`.
     *           If this function fails (due to any HTTP error to either Azure Storage or Azure IoT Hub) it can
     *           be run again for a discretionary number of times in an attempt to succeed after, for example,
     *           an internet connectivity disruption is over.  
     *
     * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_LL_AzureStorageCreateClient`.
     * @param    uploadCorrelationId    Upload correlation-id obtained with `IoTHubDeviceClient_LL_AzureStorageInitializeBlobUpload`.
     * @param    isSuccess              A boolean value indicating if the call(s) to `IoTHubDeviceClient_LL_AzureStoragePutBlock` succeeded.
     * @param    responseCode           An user-defined code to signal the status of the upload (e.g., 200 for success, or -1 for abort).
     * @param    responseMessage        An user-defined status message to go along with `responseCode` on the notification to Azure IoT Hub.
     *
     * @warning  This is a synchronous/blocking function.
     *           This function only attempts to send HTTP requests once, it does not retry in case of failure.
     *           Any retries must be explicitly implemented by the calling application.
     * 
     * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_AzureStorageNotifyBlobUploadCompletion, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, uploadCorrelationId, bool, isSuccess, int, responseCode, const char*, responseMessage);
#endif /*DONT_USE_UPLOADTOBLOB*/

    /**
    * @brief    This API sends an acknowledgement to Azure IoT Hub that a cloud-to-device message has been received and frees resources associated with the message.
    *
    * @param    device_ll_handle                The handle created by a call to a create function.
    * @param    message                         The cloud-to-device message received through the callback provided to IoTHubDeviceClient_LL_SetMessageCallback.
    * @param    disposition                     Acknowledgement option for the message.
    *
    * @warning  This function is to be used only when IOTHUBMESSAGE_ASYNC_ACK is used in the callback for incoming cloud-to-device messages.
    * @remarks
    *           If your cloud-to-device message callback returned IOTHUBMESSAGE_ASYNC_ACK, it MUST call this API eventually.
    *           Beyond sending acknowledgment to the service, this method also handles freeing message's memory.
    *           Not calling this function will result in memory leaks.
    *           Depending on the protocol used, this API will acknowledge cloud-to-device messages differently:
    *           AMQP: A MESSAGE DISPOSITION is sent using the `disposition` option provided.
    *           MQTT: A PUBACK is sent if `disposition` is `IOTHUBMESSAGE_ACCEPTED`. Passing any other option results in no PUBACK sent for the message.
    *           HTTP: A HTTP request is sent using the `disposition` option provided.
    * @return   IOTHUB_CLIENT_OK upon success, or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SendMessageDisposition, IOTHUB_DEVICE_CLIENT_LL_HANDLE, device_ll_handle, IOTHUB_MESSAGE_HANDLE, message, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);

    /**
    * @brief      Asynchronous call to send the telemetry message specified by @p telemetryMessageHandle.
    *
    * @param[in]  iotHubClientHandle             The handle created by a call to the create function.
    * @param[in]  telemetryMessageHandle         The handle to an IoT Hub message.
    * @param[in]  telemetryConfirmationCallback  Optional callback specified by the device for receiving
    *                                            confirmation of the delivery of the telemetry.
    * @param[in]  userContextCallback            User specified context that will be provided to the
    *                                            callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *
    *             The IOTHUB_MESSAGE_HANDLE instance provided as an argument is copied by the function,
    *             so this argument can be destroyed by the calling application right after IoTHubDeviceClient_LL_SendTelemetryAsync returns.
    *             The copy of @p telemetryMessageHandle is later destroyed by the iothub client when the message is successfully sent, if a failure sending it occurs, or if the client is destroyed.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SendTelemetryAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, telemetryMessageHandle, IOTHUB_CLIENT_TELEMETRY_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);
 
    /**
    * @brief      Subscribe to incoming commands from IoT Hub.
    *
    * @param[in]  iotHubClientHandle        The handle created by a call to the create function.
    * @param[in]  commandCallback           The callback which will be called when a command request arrives.
    * @param[in]  userContextCallback       User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *    
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SubscribeToCommands, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC, commandCallback,  void*, userContextCallback);
 
    /**
    * @brief      Sends device properties to IoT Hub.
    *
    * @param[in]  iotHubClientHandle           The handle created by a call to the create function.
    * @param[in]  properties                   Serialized property data to be sent to IoT Hub.  This buffer can either be
    *                                          manually serialized created with IoTHubClient_Properties_Serializer_CreateReported() 
    *                                          or IoTHubClient_Properties_Serializer_CreateWritableResponse().
    * @param[in]  propertiesLength             Number of bytes in the properties buffer.
    * @param[in]  propertyAcknowledgedCallback Optional callback specified by the application to be called with the
    *                                          result of the transaction.
    * @param[in]  userContextCallback          User specified context that will be provided to the
    *                                          callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_SendPropertiesAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle,  const unsigned char*, properties, size_t, propertiesLength, IOTHUB_CLIENT_PROPERTY_ACKNOWLEDGED_CALLBACK, propertyAcknowledgedCallback, void*, userContextCallback);
 
    /**
    * @brief      Retrieves all device properties from IoT Hub.
    *
    * @param[in]  iotHubClientHandle    The handle created by a call to the create function.
    * @param[in]  propertyCallback      Callback invoked when properties are retrieved.
    *                                   The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                   payload stream.
    * @param[in]  userContextCallback   User specified context that will be provided to the
    *                                   callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetPropertiesAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle,  IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyCallback, void*, userContextCallback);
 
    /**
    * @brief       Retrieves all device properties from IoT Hub and also subscribes for updates to writable properties.
    *
    * @param[in]   iotHubClientHandle      The handle created by a call to the create function.
    * @param[in]   propertyUpdateCallback  Callback both on initial retrieval of properties stored on IoT Hub
    *                                      and subsequent service-initiated modifications of writable properties.
    *                                      The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                      payload stream.
    * @param[in]   userContextCallback     User specified context that will be provided to the
    *                                      callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *
    * @return     IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync, IOTHUB_DEVICE_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyUpdateCallback, void*, userContextCallback);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_DEVICE_CLIENT_LL_H */
