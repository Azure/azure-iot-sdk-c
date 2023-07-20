// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_device_client.h
*    @brief Extends the IoTHubDeviceClient_LL with additional features.
*
*    @details IoTHubDeviceClient extends the IoTHubDeviceClient_LL
*             with 2 features:
*                - scheduling the work for the IoTHubDeviceClient from a
*                  thread, so that the user does not need to create their
*                  own thread
*                - thread-safe APIs
*/

#ifndef IOTHUB_DEVICE_CLIENT_H
#define IOTHUB_DEVICE_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#include "umock_c/umock_c_prod.h"
#include "iothub_transport_ll.h"
#include "iothub_client_core_ll.h"
#include "iothub_client_core.h"
#include "iothub_device_client_ll.h"

#ifndef IOTHUB_DEVICE_CLIENT_INSTANCE_TYPE
/**  
* @brief   Handle corresponding to a convenience layer device client instance. 
* 
* @remarks See https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/threading_notes.md for more details about convenience layer versus lower layer (LL) threading models.
*/
typedef IOTHUB_CLIENT_CORE_HANDLE IOTHUB_DEVICE_CLIENT_HANDLE;
#define IOTHUB_DEVICE_CLIENT_INSTANCE_TYPE
#endif // IOTHUB_CLIENT_INSTANCE

#ifndef DONT_USE_UPLOADTOBLOB
/** 
*  @brief   Handle for Upload-to-Blob API Functions.
*/
typedef IOTHUB_CLIENT_LL_UPLOADTOBLOB_CONTEXT_HANDLE IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE;
#endif // DONT_USE_UPLOADTOBLOB

#ifdef __cplusplus
extern "C"
{
#endif

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
    *                   <pre>HostName=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net];DeviceId=[Device ID goes here];SharedAccessSignature=SharedAccessSignature sr=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net]/devices/[Device ID goes here]&sig=[SAS Token goes here]&se=[Expiry Time goes here];</pre>
    *                </blockquote>
    *
    * @return    A non-NULL #IOTHUB_DEVICE_CLIENT_HANDLE value that is used when
    *             invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_HANDLE, IoTHubDeviceClient_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Creates a IoT Hub client for communication with an existing IoT
    *           Hub using the specified parameters.
    *
    * @param    config    Pointer to an #IOTHUB_CLIENT_CONFIG structure
    *
    *           The API does not allow sharing of a connection across multiple
    *           devices. This is a blocking call.
    *
    * @return   A non-NULL #IOTHUB_DEVICE_CLIENT_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_HANDLE, IoTHubDeviceClient_Create, const IOTHUB_CLIENT_CONFIG*, config);

    /**
    * @brief    Creates a IoT Hub client for communication with an existing IoT
    *           Hub using the specified parameters.
    *
    * @param    transportHandle     TRANSPORT_HANDLE which represents a connection.
    * @param    config              Pointer to an #IOTHUB_CLIENT_CONFIG structure
    *
    *           The API allows sharing of a connection across multiple
    *           devices. This is a blocking call.
    *
    * @return   A non-NULL #IOTHUB_DEVICE_CLIENT_HANDLE value that is used when
    *           invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_HANDLE, IoTHubDeviceClient_CreateWithTransport, TRANSPORT_HANDLE, transportHandle, const IOTHUB_CLIENT_CONFIG*, config);

    /**
    * @brief    Creates a IoT Hub client for communication with an existing IoT
    *           Hub using the device auth.
    *
    * @param    iothub_uri      Pointer to an ioThub hostname received in the registration process
    * @param    device_id       Pointer to the device Id of the device
    * @param    protocol        Function pointer for protocol implementation
    *
    * @return    A non-NULL #IOTHUB_DEVICE_CLIENT_HANDLE value that is used when
    *            invoking other functions for IoT Hub client and @c NULL on failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_DEVICE_CLIENT_HANDLE, IoTHubDeviceClient_CreateFromDeviceAuth, const char*, iothub_uri, const char*, device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief    Disposes of resources allocated by the IoT Hub client. This is a
    *           blocking call.
    *
    * @param    iotHubClientHandle    The handle created by a call to the create function.
    *
    * @warning  Do not call this function from inside any application callbacks from this SDK, e.g. your IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK handler.
    */
    MOCKABLE_FUNCTION(, void, IoTHubDeviceClient_Destroy, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle);

    /**
    * @brief    Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    eventMessageHandle              The handle to an IoT Hub message.
    * @param    eventConfirmationCallback       The callback specified by the device for receiving
    *                                           confirmation of the delivery of the IoT Hub message.
    *                                           This callback can be expected to invoke the
    *                                           IoTHubDeviceClient_SendEventAsync function for the
    *                                           same message in an attempt to retry sending a failing
    *                                           message. The user can specify a @c NULL value here to
    *                                           indicate that no callback is required.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @remarks
    *           The IOTHUB_MESSAGE_HANDLE instance provided as argument is copied by the function,
    *           so this argument can be destroyed by the calling application right after IoTHubDeviceClient_SendEventAsync returns.
    *           The copy of @c eventMessageHandle is later destroyed by the iothub client when the message is effectively sent, if a failure sending it occurs, or if the client is destroyed.
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SendEventAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

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
    * @remark    Does not return information related to uploads initiated by IoTHubDeviceClient_UploadToBlob or IoTHubDeviceClient_UploadMultipleBlocksToBlob.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetSendStatus, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);

    /**
    * @brief    Sets up the message callback to be invoked when IoT Hub issues a
    *           message to the device. This is a blocking call.
    *
    * @param    iotHubClientHandle          The handle created by a call to the create function.
    * @param    messageCallback             The callback specified by the device for receiving
    *                                       messages from IoT Hub.
    * @param    userContextCallback         User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetMessageCallback, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    *           the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    connectionStatusCallback        The callback specified by the device for receiving
    *                                           updates about the status of the connection to IoT Hub.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @remark   Callback specified will not receive connection status change notifications for upload connections created with IoTHubDeviceClient_UploadToBlob or IoTHubDeviceClient_UploadMultipleBlocksToBlob.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetConnectionStatusCallback, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    *           the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    retryPolicy                     The policy to use to reconnect to IoT Hub when a
    *                                           connection drops.
    * @param    retryTimeoutLimitInSeconds      Maximum amount of time(seconds) to attempt reconnection when a
    *                                           connection drops to IOT Hub.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @remark   Uploads initiated by IoTHubDeviceClient_UploadToBlob or IoTHubDeviceClient_UploadMultipleBlocksToBlob do not have automatic retries and do not honor the retryPolicy settings.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetRetryPolicy, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);

    /**
    * @brief    Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    retryPolicy                     Out parameter containing the policy to use to reconnect to IoT Hub.
    * @param    retryTimeoutLimitInSeconds      Out parameter containing maximum amount of time in seconds to attempt reconnection
    *                                           to IOT Hub.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetRetryPolicy, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);

    /**
    * @brief    This function returns in the out parameter @p lastMessageReceiveTime
    *           what was the value of the @c time function when the last message was
    *           received at the client.
    *
    * @param    iotHubClientHandle           The handle created by a call to the create function.
    * @param    lastMessageReceiveTime       Out parameter containing the value of @c time function
    *                                        when the last message was received.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetLastMessageReceiveTime, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, time_t*, lastMessageReceiveTime);

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
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetOption, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, optionName, const void*, value);

    /**
    * @brief    This API specifies a callback to be used when the device receives a state update.
    *
    * @param    iotHubClientHandle          The handle created by a call to the create function.
    * @param    deviceTwinCallback          The callback specified by the device client to be used for updating
    *                                       the desired state. The callback will be called in response to a
    *                                       request send by the IoT Hub services. The payload will be passed to the
    *                                       callback, along with two version numbers:
    *                                           - Desired:
    *                                           - LastSeenReported:
    * @param    userContextCallback         User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetDeviceTwinCallback, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

    /**
    * @brief    This API sends a report of the device's properties and their current values.
    *
    * @param    iotHubClientHandle          The handle created by a call to the create function.
    * @param    reportedState               The current device property values to be 'reported' to the IoT Hub.
    * @param    size                        Number of bytes in @c reportedState.
    * @param    reportedStateCallback       The callback specified by the device client to be called with the
    *                                       result of the transaction.
    * @param    userContextCallback         User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SendReportedState, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);

    /**
    * @brief    This API provides a way to retrieve the complete device Twin properties on-demand.
    *
    * @param    iotHubClientHandle       The handle created by a call to the create function.
    * @param    deviceTwinCallback       The callback invoked to provide the complete Device Twin properties once its retrieval is completed by the client.
    *                                    If any failures occur, the callback is invoked passing @c NULL as payLoad and zero as size.
    * @param    userContextCallback      User specified context that will be provided to the
    *                                    callback. This can be @c NULL.
    *
    * @warning: Do not call IoTHubDeviceClient_Destroy() from inside your application's callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetTwinAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

    /**
    * @brief    This API sets the callback for async cloud to device method calls.
    *
    * @param    iotHubClientHandle              The handle created by a call to the create function.
    * @param    deviceMethodCallback            The callback which will be called by IoT Hub.
    * @param    userContextCallback             User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SetDeviceMethodCallback, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, deviceMethodCallback, void*, userContextCallback);

    /**
    * @brief    This API responds to an asnyc method callback identified the methodId.
    *
    * @param    iotHubClientHandle      The handle created by a call to the create function.
    * @param    methodId                The methodId of the Device Method callback.
    * @param    response                The response data for the method callback.
    * @param    response_size           The size of the response data buffer.
    * @param    statusCode              The status response of the method callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_DeviceMethodResponse, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, response_size, int, statusCode);

#ifndef DONT_USE_UPLOADTOBLOB
    /**
    * @brief    IoTHubDeviceClient_UploadToBlobAsync uploads data from memory to a file in Azure Blob Storage.
    *
    * @param    iotHubClientHandle                  The handle created by a call to the IoTHubDeviceClient_Create function.
    * @param    destinationFileName                 The name of the file to be created in Azure Blob Storage.
    * @param    source                              The source of data.
    * @param    size                                The size of data.
    * @param    iotHubClientFileUploadCallback      A callback to be invoked when the file upload operation has finished.
    * @param    context                             A user-provided context to be passed to the file upload callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_UploadToBlobAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, destinationFileName, const unsigned char*, source, size_t, size, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK, iotHubClientFileUploadCallback, void*, context);

    /**
    * @brief                          Uploads a file to a Blob storage in chunks, fed through the callback function provided by the user.
    * @remarks                        This function allows users to upload large files in chunks, not requiring the whole file content to be passed in memory.
    * @param iotHubClientHandle       The handle created by a call to the IoTHubDeviceClient_Create function.
    * @param destinationFileName      The name of the file to be created in Azure Blob Storage.
    * @param getDataCallbackEx        A callback to be invoked to acquire the file chunks to be uploaded, as well as to indicate the status of the upload of the previous block.
    * @param context                  Any data provided by the user to serve as context on getDataCallback.
    * @returns                        An IOTHUB_CLIENT_RESULT value indicating the success or failure of the API call.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, destinationFileName, IOTHUB_CLIENT_FILE_UPLOAD_GET_DATA_CALLBACK_EX, getDataCallbackEx, void*, context);

     /**
    * @brief    This API creates a new upload within Azure IoT Hub, getting back a correlation-id and a
    *           SAS URI for the Blob upload to the Azure Storage associated with the Azure IoT Hub.
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *
    * @param    iotHubClientHandle      The handle created by a call to the create function.
    * @param    destinationFileName     Name of the file in blob storage.
    * @param    uploadCorrelationId     Variable where to store the correlation-id of the new upload.
    * @param    azureBlobSasUri         Variable where to store the Azure Storage SAS URI for the new upload.
    *
    * @warning  This is a synchronous/blocking function.
    *           `uploadCorrelationId` and `azureBlobSasUri` must be freed by the calling application
    *           after the blob upload process is done (e.g., after calling `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`).
    *
    * @return   An IOTHUB_CLIENT_RESULT value indicating the success or failure of the API call.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_AzureStorageInitializeBlobUpload, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, destinationFileName, char**, uploadCorrelationId, char**, azureBlobSasUri);

     /**
    * @brief    This API creates a client for a new blob upload to Azure Storage.
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *
    * @param    iotHubClientHandle      The handle created by a call to the create function.
    * @param    azureBlobSasUri         The Azure Storage Blob SAS uri obtained with `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`.
    *
    * @warning  This is a synchronous/blocking function.
    *
    * @return   A `IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE` on success or NULL if the function fails.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE, IoTHubDeviceClient_AzureStorageCreateClient, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, azureBlobSasUri);

    /**
    * @brief    This API uploads a single blob block to Azure Storage (performs a PUT BLOCK operation).
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *           For more information about Azure Storage PUT BLOCK, its parameters and behavior, please refer to
    *           https://learn.microsoft.com/en-us/rest/api/storageservices/put-block
    *
    * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_AzureStorageCreateClient`.
    * @param    blockNumber            Number of the block being uploaded.
    * @param    dataPtr                Pointer to the block data to be uploaded to Azure Storage blob.
    * @param    dataSize               Size of the block data pointed by `dataPtr`.
    *
    * @warning  This is a synchronous/blocking function.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_AzureStoragePutBlock, IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle, uint32_t, blockNumber, const uint8_t*, dataPtr, size_t, dataSize);

    /**
    * @brief    This API performs an Azure Storage PUT BLOCK LIST operation.
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *           If this function fails (due to any HTTP error to Azure Storage) it can
    *           be run again for a discretionary number of times in an attempt to succeed after, for example,
    *           an internet connectivity disruption is over.
    *           For more information about Azure Storage PUT BLOCK LIST, please refer to
    *           https://learn.microsoft.com/en-us/rest/api/storageservices/put-block-list
    *
    * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_AzureStorageCreateClient`.
    *
    * @warning  This is a synchronous/blocking function.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_AzureStoragePutBlockList, IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle);

    /**
    * @brief    This API destroys an instance previously created with `IoTHubDeviceClient_AzureStorageCreateClient`.
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *
    * @param    azureStorageClientHandle     The handle created with `IoTHubDeviceClient_AzureStorageCreateClient`.
    *
    * @warning  This is a synchronous/blocking function.
    *
    * @return   Nothing.
    */
    MOCKABLE_FUNCTION(, void, IoTHubDeviceClient_AzureStorageDestroyClient, IOTHUB_CLIENT_AZURE_STORAGE_CLIENT_HANDLE, azureStorageClientHandle);

    /**
    * @brief    This API notifies Azure IoT Hub of the upload completion.
    * @remark   It is part of a set of functions for more granular control over Azure IoT-based blob uploads.
    *           This function is expected to be used along with:
    *           `IoTHubDeviceClient_AzureStorageCreateClient`
    *           `IoTHubDeviceClient_AzureStoragePutBlock`
    *           `IoTHubDeviceClient_AzureStoragePutBlockList`
    *           `IoTHubDeviceClient_AzureStorageDestroyClient`
    *           `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`
    *           `IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion`
    *           For simpler/less-granular control of uploads to Azure blob storage please use either
    *           `IoTHubDeviceClient_UploadToBlobAsync` or `IoTHubDeviceClient_UploadMultipleBlocksToBlobAsync`.
    *           If this function fails (due to any HTTP error to either Azure Storage or Azure IoT Hub) it can
    *           be run again for a discretionary number of times in an attempt to succeed after, for example,
    *           an internet connectivity disruption is over.  
    *
    * @param    azureStorageClientHandle    The handle created with `IoTHubDeviceClient_AzureStorageCreateClient`.
    * @param    uploadCorrelationId    Upload correlation-id obtained with `IoTHubDeviceClient_AzureStorageInitializeBlobUpload`.
    * @param    isSuccess              A boolean value indicating if the call(s) to `IoTHubDeviceClient_AzureStoragePutBlock` succeeded.
    * @param    responseCode           An user-defined code to signal the status of the upload (e.g., 200 for success, or -1 for abort).
    * @param    responseMessage        An user-defined status message to go along with `responseCode` on the notification to Azure IoT Hub.
    *
    * @warning  This is a synchronous/blocking function.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_AzureStorageNotifyBlobUploadCompletion, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, const char*, uploadCorrelationId, bool, isSuccess, int, responseCode, const char*, responseMessage);
#endif /* DONT_USE_UPLOADTOBLOB */

    /**
    * @brief    This API sends an acknowledgement to Azure IoT Hub that a cloud-to-device message has been received and frees resources associated with the message.
    *
    * @param    iotHubClientHandle              The handle created by a call to a create function.
    * @param    message                         The cloud-to-device message received through the callback provided to IoTHubDeviceClient_SetMessageCallback.
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
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SendMessageDisposition, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, message, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);

    /**
    * @brief    Asynchronous call to send the telemetry message specified by @p telemetryMessageHandle.
    *
    * @param[in]  iotHubClientHandle             The handle created by a call to the create function.
    * @param[in]  telemetryMessageHandle         The handle to an IoT Hub message.
    * @param[in]  telemetryConfirmationCallback  Optional callback specified by the device for receiving
    *                                            confirmation of the delivery of the telemetry.
    * @param[in]  userContextCallback            User specified context that will be provided to the
    *                                            callback. This can be @c NULL.
    *
    * @remarks  The application behavior is undefined if the user calls
    *           the IoTHubDeviceClient_Destroy function from within any callback.
    *
    *           The IOTHUB_MESSAGE_HANDLE instance provided as an argument is copied by the function,
    *           so this argument can be destroyed by the calling application right after IoTHubDeviceClient_SendTelemetryAsync returns.
    *           The copy of @p telemetryMessageHandle is later destroyed by the iothub client when the message is successfully sent, if a failure sending it occurs, or if the client is destroyed.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SendTelemetryAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, telemetryMessageHandle, IOTHUB_CLIENT_TELEMETRY_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);
    
    /**
    * @brief    Subscribe to incoming commands from IoT Hub.
    *
    * @param[in]  iotHubClientHandle        The handle created by a call to the create function.
    * @param[in]  commandCallback           The callback which will be called when a command request arrives.
    * @param[in]  userContextCallback       User specified context that will be provided to the
    *                                       callback. This can be @c NULL.
    *
    * @remarks    The application behavior is undefined if the user calls
    *             the IoTHubDeviceClient_LL_Destroy function from within any callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SubscribeToCommands, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_COMMAND_CALLBACK_ASYNC, commandCallback,  void*, userContextCallback);

    /**
    * @brief    Sends device properties to IoT Hub.
    *
    * @param[in]  iotHubClientHandle            The handle created by a call to the create function.
    * @param[in]  properties                    Serialized property data to be sent to IoT Hub.  This buffer can either be
    *                                           manually serialized created with IoTHubClient_Properties_Serializer_CreateReported() 
    *                                           or IoTHubClient_Properties_Serializer_CreateWritableResponse().
    * @param[in]  propertiesLength              Number of bytes in the properties buffer.
    * @param[in]  propertyAcknowledgedCallback  Optional callback specified by the application to be called with the
    *                                           result of the transaction.
    * @param[in]  userContextCallback           User specified context that will be provided to the
    *                                           callback. This can be @c NULL.
    *
    * @remarks   The application behavior is undefined if the user calls
    *            the IoTHubDeviceClient_Destroy function from within any callback.
    *
    * @return    IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_SendPropertiesAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle,  const unsigned char*, properties, size_t, propertiesLength, IOTHUB_CLIENT_PROPERTY_ACKNOWLEDGED_CALLBACK, propertyAcknowledgedCallback, void*, userContextCallback);

    /**
    * @brief   Retrieves all device properties from IoT Hub.
    *
    * @param[in]  iotHubClientHandle    The handle created by a call to the create function.
    * @param[in]  propertyCallback      Callback invoked when properties are retrieved.
    *                                   The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                   payload stream.
    * @param[in]  userContextCallback   User specified context that will be provided to the
    *                                   callback. This can be @c NULL.
    *
    * @remarks  The application behavior is undefined if the user calls
    *           the IoTHubDeviceClient_Destroy function from within any callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetPropertiesAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle,  IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyCallback, void*, userContextCallback);
    
    /**
    * @brief   Retrieves all device properties from IoT Hub and also subscribes for updates to writable properties.
    *
    * @param[in]  iotHubClientHandle      The handle created by a call to the create function.
    * @param[in]  propertyUpdateCallback  Callback both on initial retrieval of properties stored on IoT Hub
    *                                     and subsequent service-initiated modifications of writable properties.
    *                                     The API IoTHubClient_Deserialize_Properties() can help deserialize the raw 
    *                                     payload stream.
    * @param[in]  userContextCallback     User specified context that will be provided to the
    *                                     callback. This can be @c NULL.
    *
    * @remarks  The application behavior is undefined if the user calls
    *           the IoTHubDeviceClient_Destroy function from within any callback.
    *
    * @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubDeviceClient_GetPropertiesAndSubscribeToUpdatesAsync, IOTHUB_DEVICE_CLIENT_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK, propertyUpdateCallback, void*, userContextCallback);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_DEVICE_CLIENT_H */
