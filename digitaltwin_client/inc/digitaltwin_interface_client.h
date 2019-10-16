// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**  
  @file digitaltwin_interface_client.h

  @brief  DigitalTwin InterfaceClient functions.

  @details  Functions in this header are used by Digital Twin interface implementations to receive requests
            on this interface from the server (namely commands and property updates) and to send data from
            the interface to the server (namely reported properties and telemetry).
*/


#ifndef DIGITALTWIN_INTERFACE_CLIENT_H
#define DIGITALTWIN_INTERFACE_CLIENT_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "digitaltwin_client_common.h"

#include "digitaltwin_device_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Version 1 of the structure <c>DIGITALTWIN_CLIENT_COMMAND_REQUEST</c> is being provided to the device application from the Digital Twin SDK. */
#define DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1   1

/** @brief  Structure filled in by the Digital Twin SDK on invoking an interface's command callback routine with information about the request. */
typedef struct DIGITALTWIN_CLIENT_COMMAND_REQUEST_TAG
{
    /** @brief The version *of the structure* that the SDK is passing.  This is *NOT* related to the server version. Currently <c>DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1</c> but would be incremented if new fields are added.  */
    unsigned int version;
    /** @brief Name of the command to execute on this interface.  */
    const char* commandName;
    /** @brief Raw payload of the request. */
    const unsigned char* requestData;
    /** @brief Length of the request specified in requestData */
    size_t requestDataLen;
    /** @brief A server generated string passed as part of the command.  This is used when sending responses to asynchronous commands to act as a correlation Id and/or for diagnostics purposes. */
    const char* requestId;
}
DIGITALTWIN_CLIENT_COMMAND_REQUEST;

/** @brief Command status code set to indicate an asynchronous command has been accepted */
#define DIGITALTWIN_ASYNC_STATUS_CODE_PENDING 202

/** @brief Version 1 of the structure <c>DIGITALTWIN_CLIENT_COMMAND_RESPONSE</c> is being provided by the device application to the Digital Twin SDK. */
#define DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1  1

/** @brief  Structure filled by the device application after processing a command on its interface and returned to the Digital Twin SDK. */
typedef struct DIGITALTWIN_CLIENT_COMMAND_RESPONSE_TAG
{
    /** @brief The version *of the structure* that the SDK is passing.  This is *NOT* related to the server version. Currently <c>DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1</c> but would be incremented if new fields are added.  */
    unsigned int version;
    /** @brief  Status code to map back to the server.  Roughly maps to HTTP status codes.  To indicate that this command has been accepted but that the final response is pending, set this to <c>DIGITALTWIN_ASYNC_STATUS_CODE_PENDING</c> and 
                use <c>DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync</c> API for updates. */
    int status;
    /** @brief  Response payload to send to server.  This *MUST* be allocated with <c>malloc()</c> by the application.  The Digital Twin SDK takes responsibility for calling <c>free()</c> on this value when the structure is returned. */
    /** @warning This data must be valid JSON.  See conceptual documentation on data_format.md for more information. */
    unsigned char* responseData;
    /** @brief Number of bytes specified in responseData. */
    size_t responseDataLen;
}
DIGITALTWIN_CLIENT_COMMAND_RESPONSE;

/** @brief Version 1 of the structure <c>DIGITALTWIN_CLIENT_PROPERTY_UPDATE</c>is being provided to the device application from the Digital Twin SDK. */
#define DIGITALTWIN_CLIENT_PROPERTY_UPDATE_VERSION_1  1

/** @brief Structure filled in by the Digital Twin SDK on invoking an interface's property callback routine with information about the request. */
typedef struct DIGITALTWIN_CLIENT_PROPERTY_UPDATE_TAG
{
    /** @brief The version *of the structure* that the SDK is passing.  This is *NOT* related to the server version. Currently <c>DIGITALTWIN_CLIENT_PROPERTY_UPDATE_VERSION_1</c> but would be incremented if new fields are added.  */
    unsigned int version;
    /** @brief Name of the property being update */
    const char* propertyName;
    /** @brief Value that the device application had previously reported for this property on <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c>.  
               This value may be NULL if the application never reported a property on <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c>.  It will also
               be NULL when an update arrives to the given property *after* the initial callback.
    */
    unsigned const char* propertyReported;
    /** @brief Number of bytes in propertyReported. */
    size_t propertyReportedLen;
    /** @brief Value the service requests the given property to be set to.
    */
    unsigned const char* propertyDesired;
    /** @brief Number of bytes in propertyDesired. */
    size_t propertyDesiredLen;
    /** @brief Version (from the service, NOT the C structure) of this property.  This version should be specified in <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> when updating this property. */
    int desiredVersion;
} DIGITALTWIN_CLIENT_PROPERTY_UPDATE;

/** @brief Version 1 of the structure <c>DIGITALTWIN_CLIENT_PROPERTY_RESPONSE</c> is being provided by the device application to the Digital Twin SDK. */
#define DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1  1

/** @brief    Structure filled in by the device application when it is responding to a server initiated request to update a property in <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c>.
*/
typedef struct DIGITALTWIN_CLIENT_PROPERTY_RESPONSE
{
    /** @brief    The version of this structure (not the server version).  Currently must be <c>DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1</c>. */
    unsigned int version;
    /** @brief    responseVersion.  This is used for server to disambiguate calls for given property.  It should match the *desiredVersion* field in the structure <c>DIGITALTWIN_CLIENT_PROPERTY_UPDATE</c>that this is responding to. */
    int responseVersion;
    /** @brief    statusCode - which should map to appropriate HTTP status code - of property update.*/
    int statusCode;
    /** @brief    Friendly description string of current status of update. */
    const char* statusDescription;
} DIGITALTWIN_CLIENT_PROPERTY_RESPONSE;

/** @brief Version 1 of the structure <c>DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE</c> is being provided by the device application to the Digital Twin SDK. */
#define DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_VERSION_1 1 

/** @brief  Structure filled in by the device application when it is updating an asynchronous command's status in the call to <c>DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync</c>. */
typedef struct DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_TAG
{
    /** @brief  The version of this structure (not the server version).  Currently must be <c>DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_VERSION_1</c>. */
    unsigned int version;
    /** @brief  The command from the server that initiated the request that we are updating.  This comes from the structure <c>DIGITALTWIN_CLIENT_COMMAND_REQUEST</c> passed to the device application's command callback handler.  */
    const char* commandName;
    /** @brief  The requestId from the server that initiated the request that we are updating.  This comes from the structure <c>DIGITALTWIN_CLIENT_COMMAND_REQUEST</c> passed to the device application's command callback handler.  */
    const char* requestId;
    /** @brief  Payload that the device should send to the service. */
    /** @warning This data must be valid JSON.  See conceptual documentation on data_format.md for more information. */
    const unsigned char* propertyData;
    /** @brief Length of the property specified in propertyData */
    size_t propertyDataLen;
    /** @brief  Status code to map back to the server.  Roughly maps to HTTP status codes.*/
    int statusCode;
} DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE;


/** 
  @brief Function signature for callback that is invoked by the Digital Twin SDK when a desired property is available from the service.

  @remarks There are two scenarios where this callback may be invoked.  After this interface is initially registered, the Digital Twin SDK will query all desired properties on
           it and invoke the callback.  The SDK will invoke this callback even if the property has not changed since any previous invocations, since the SDK does not
           have a persistent cache of state.

  @remarks After this initial update, the SDK will also invoke this callback again whenever any properties change.

  @remarks If multiple properties are available from the service simultaneously, this callback will be called once for each updated property.  There is no attempt to batch multiple
           properties into one call.

  @remarks The callback is specified initially during the call to <c>DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback</c>.

  @param[in] dtClientPropertyUpdate             <c>DIGITALTWIN_CLIENT_PROPERTY_UPDATE</c>structure filled in by the SDK with information about the updated property.  This memory is owned by the SDK.
  @param[in] propertyCallbackContext            Context pointer that was specified in the parameter <c>propertyCallbackContext</c> during the device application's <c>DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback</c> call.

  @return N/A.
*/
typedef void(*DIGITALTWIN_PROPERTY_UPDATE_CALLBACK)(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* propertyCallbackContext);

/** 
  @brief Function signature for the callback that is invoked by the Digital Twin SDK when a command is invoked from the service.

  @remarks When a command arrives from the service for a particular interface, <c>DIGITALTWIN_PROPERTY_UPDATE_CALLBACK</c> specifies its callback signature.

  @remarks The callback itself is set during the call to <c>DigitalTwin_InterfaceClient_SetCommandsCallback</c>.

  @remarks This callback must execute fairly quickly; in a matter of seconds.  The underlying transport will timeout commands if they take too long to execute on the device, with the default being 60 seconds.

  @param[in] dtClientCommandContext                 <c>DIGITALTWIN_CLIENT_COMMAND_REQUEST</c> structure filled in by the SDK with information about the command.  This memory is owned by the SDK.
  @param[in, out] dtClientCommandResponseContext    <c>DIGITALTWIN_CLIENT_COMMAND_RESPONSE</c> to be filled in by the application with the response code and payload to be returned to the service.  
                                                    The memory for <c>dtClientCommandResponseContext</c> is owned by the SDK.  See <c>DIGITALTWIN_CLIENT_COMMAND_RESPONSE</c> documentation as some fields 
                                                    of the structure itself are passed from the application to the SDK.
  @param[in] commandCallbackContext                 Context pointer that was specified in the parameter <c>commandCallbackContext</c> during the device application's <c>DigitalTwin_InterfaceClient_SetCommandsCallback</c> call.

  @return N/A
*/
typedef void(*DIGITALTWIN_COMMAND_EXECUTE_CALLBACK)(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* commandCallbackContext);


/** 
  @brief Function signature for the callback that is invoked by the Digital Twin SDK when a client initiated property update succeeds or fails.

  @remarks When a device application invokes <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c>, it may optionally include a function pointer callback with this signature.

  @remarks When the Digital Twin either successfully updates the property or fails, it will invoke this callback.

  @remarks Note that this is distinct from <c>DIGITALTWIN_PROPERTY_UPDATE_CALLBACK</c>.  <c>DIGITALTWIN_PROPERTY_UPDATE_CALLBACK</c> is invoked when the server initiates a property change on the device, such as
           a server application requesting a desired temperature to be set to 80 degrees.  This <c>DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK</c> is in response to the device 
           initiated reporting of a property, e.g. a device reporting its firmware version was accepted by the server.

  @param[in] dtReportedStatus                   Status of the property update operation.
  @param[in] userContextCallback                Context pointer that was specified in the parameter <c>userContextCallback</c> during the device application's <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> call.

  @return N/A
*/
typedef void(*DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK)(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback);

/** 
  @brief Function signature for the callback that is invoked by the Digital Twin SDK when a telemetry send succeeds or fails.

  @remarks When a device application invokes <c>DigitalTwin_InterfaceClient_SendTelemetryAsync</c>, it may optionally include a function pointer callback with this signature.

  @remarks When the Digital Twin either successfully sends the telemetry to the service or fails, it will invoke this callback.

  @param[in] dtTelemetryStatus                  Status of the telemetry send operation.
  @param[in] userContextCallback                Context pointer that was specified in the parameter <c>userContextCallback</c> during the device application's <c>DigitalTwin_InterfaceClient_SendTelemetryAsync</c> call.

  @return N/A
*/
typedef void(*DIGITALTWIN_CLIENT_TELEMETRY_CONFIRMATION_CALLBACK)(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback);

/** 
  @brief Function signature for the callback that is invoked by the Digital Twin SDK when updating command's status asynchronously succeeds or fails.

  @remarks When a device application invokes <c>DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync</c>, it may optionally include a function pointer callback with this signature.

  @remarks When the Digital Twin either successfully sends the command status update to the service or fails, it will invoke this callback.

  @param[in] dtUpdateAsyncCommandStatus         Status of the update operation.
  @param[in] userContextCallback                Context pointer that was specified in the parameter <c>userContextCallback</c> during the device application's <c>DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync</c> call.

  @return N/A
*/
typedef void(*DIGITALTWIN_CLIENT_UPDATE_ASYNC_COMMAND_CALLBACK)(DIGITALTWIN_CLIENT_RESULT dtUpdateAsyncCommandStatus, void* userContextCallback);


/** 
  @brief Creates a <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>

  @remarks The first step in setting up an interface is to create the underlying handle for it, using <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> returned in <c>dtInterfaceClientHandle</c>.
           This handle cannot be used until it is registered with the service.

  @param[in]  interfaceId                       The interfaceId of interface to be registered.  For example, urn:contoso:com:EnvironmentalSensor:1.
  @param[in]  componentName                     The component name associated with this interface Id.  For example, environmentalSensor.
  @param[in]  dtInterfaceRegisteredCallback     (Optional) Function pointer to be invoked when the interface status (e.g. is registered or fails to register) changes.
  @param[in]  userInterfaceContext              (Optional) Context pointer passed to dtInterfaceRegisteredCallback function on interface changes.
  @param[out] dtInterfaceClientHandle           Handle created in this call. 

  @see DIGITALTWIN_INTERFACE_CLIENT_HANDLE
  @see DIGITALTWIN_PROPERTY_UPDATE_CALLBACK

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_Create, const char*, interfaceId, const char*, componentName, DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK, dtInterfaceRegisteredCallback, void*, userInterfaceContext, DIGITALTWIN_INTERFACE_CLIENT_HANDLE*, dtInterfaceClientHandle);

/** 
  @brief Specifies callback function to process property updates from the server.

  @remarks The device application invokes <c>DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback</c> to provide the callback function to process property updates for this interface.
           This *must* be called prior to the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> being registered and may only be called once for the lifetime of the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>.

  @param[in] dtInterfaceClientHandle    Handle for the interface client.
  @param[in] dtPropertyUpdatedCallback  Function pointer to invoke the callback on when commands for this interface arrive. 
  @param[in] propertyCallbackContext    (Optional) Context pointer passed to dtInterfaceRegisteredCallback function on property update callback.

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_PROPERTY_UPDATE_CALLBACK, dtPropertyUpdatedCallback, void*, propertyCallbackContext);

/** 
  @brief Specifies callback function to process commands from the server.

  @remarks The device application invokes <c>DigitalTwin_InterfaceClient_SetCommandsCallback</c> to provide the callback function to process commands for this interface.
           This *must* be called prior to the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c> being registered and may only be called once for the lifetime of the <c>DIGITALTWIN_INTERFACE_CLIENT_HANDLE</c>.

  @param[in] dtInterfaceClientHandle            Handle for the interface client.
  @param[in] dtPropertyUpdatedCallback          Function pointer to invoke the callback on when commands for this interface arrive. 
  @param[in] commandCallbackContext             (Optional) Context pointer passed to dtInterfaceRegisteredCallback function on command callback.

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_SetCommandsCallback, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, DIGITALTWIN_COMMAND_EXECUTE_CALLBACK, dtCommandExecuteCallback, void*, commandCallbackContext);

/** 
  @brief Sends a Digital Twin telemetry message to the server.

  @remarks The device application invokes <c>DigitalTwin_InterfaceClient_SendTelemetryAsync</c> to send telemetry to the server.  The call returns immediately and puts
           the data to send on a pending queue the SDK manages.  The application is notified of success or failure of the send by passing in a callback <c>telemetryConfirmationCallback</c>.

  @remarks The application may invoke <c>DigitalTwin_InterfaceClient_SendTelemetryAsync</c> as many times as it wants, subject to the device's resource availability.  The application does
           NOT need to wait for each send's <c>telemetryConfirmationCallback</c> to arrive between sending.

  @remarks The SDK will automatically attempt to retry the telemetry send on transient errors.

  @warning The data specified in <c>messageData</c> must be valid JSON.  See conceptual documentation on data_format.md for more information.

  @param[in] dtInterfaceClientHandle            Handle for the interface client.
  @param[in] messageData                        Value of the telemetry data to send.  The schema must match the data specified in the model document.
  @param[in] messageDataLen                     Length of the telemetry data to send.
  @param[in] telemetryConfirmationCallback      (Optional) Function pointer to be invoked when the telemetry message is successfully delivered or fails.
  @param[in] userContextCallback                (Optional) Context pointer passed to telemetryConfirmationCallback function when it is invoked.

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/
MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_SendTelemetryAsync, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, const unsigned char*, messageData, size_t, messageDataLen, DIGITALTWIN_CLIENT_TELEMETRY_CONFIRMATION_CALLBACK, telemetryConfirmationCallback, void*, userContextCallback);

/** 
  @brief Sends a Digital Twin property to the server.

  @remarks The device application invokes <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> to update a property on the server.

  @remarks There are two types of properties that can be modeled in the Digital Twin Definition Language (DTDL).  They are either configurable from the server
           (referred to as "writeable" in the Digital Twin Definition Language)   An example is a thermostat exposing its desired temperature setting.  In this case,
           the device also can indicate whether it has accepted the property or whether it has failed (setting the temperature above some firmware limit, for instance).

  @remarks The other class of properties are not configurable/"writeable" from the service application's perspective.  Only the device can set these properties.  An 
           example is the device's manufacturer in the DeviceInformation interface.

  @remarks Configurable properties are tied to receive updates from the server via <c>DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback</c>.

  @remarks Both classes of properties use <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> to report the status of the property; e.g. whether the temperature was
           accepted in the thermostat example or simple the value of the manufacturer for DeviceInformation.  The only difference is that the configurable property
           must fill in the <c>dtResponse</c> parameter so the server knows additional status/server version/etc. of the property.

  @remarks <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> may be invoked at any time after the interface has been successfully registered per the (<c>DIGITALTWIN_INTERFACE_REGISTERED_CALLBACK</c>) and before
           the interface handle is destroyed.  It may be invoked on a callback - in particular on the application's <c>DIGITALTWIN_PROPERTY_UPDATE_CALLBACK</c> - though it does not have to be.             

  @remarks The call returns immediately and the puts the data to send on a pending queue that the SDK manages.  The application is notified of success or failure of the send by passing in a callback <c>dtReportedPropertyCallback</c>.

  @remarks The application may <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> as many times as it wants, subject obviously to the device's resource availability, and does
           NOT need to wait for each send's <c>telemetryConfirmationCallback</c> to arrive between sending.  If <c>DigitalTwin_InterfaceClient_ReportPropertyAsync</c> is invoked multiple times
           on the same <c>propertyName</c>, the server has a last-writer wins algorithm and will not persist previous property values.

  @remarks The SDK will automatically attempt to retry reporting properties on transient errors.

  @warning The data specified in <c>propertyData</c> must be valid JSON.  See conceptual documentation on data_format.md for more information.

  @param[in] dtInterfaceClientHandle        Handle for the interface client.
  @param[in] propertyName                   Name of the property to report.  This should match the model associated with this interface.
  @param[in] propertyData                   Value of the property to report.
  @param[in] propertyDataLen                Length of the property to report.
  @param[in] dtReportedPropertyCallback     (Optional) Function pointer to be invoked when the property is successfully reported or fails.
  @param[in] userContextCallback            (Optional) Context pointer passed to dtReportedPropertyCallback function when it is invoked.

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_ReportPropertyAsync, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, const char*, propertyName, const unsigned char*, propertyData, size_t, propertyDataLen, const DIGITALTWIN_CLIENT_PROPERTY_RESPONSE*, dtResponse, DIGITALTWIN_REPORTED_PROPERTY_UPDATED_CALLBACK, dtReportedPropertyCallback, void*, userContextCallback);

/** 
  @brief Sends an update of the status of a pending asynchronous command.

  @remarks Devices must return quickly while processing command execution callbacks in their <c>DIGITALTWIN_COMMAND_EXECUTE_CALLBACK</c> callback.  Commands that
           take longer to run - such as running a diagnostic - may be modeled as "asynchronous" commands in the Digital Twin Definition Language.

  @remarks The device application invokes <c>DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync</c> to update the status of an asynchronous command.  This status
           could indicate a success, a fatal failure, or else that the command is still running and provide some simple progress.

  @remarks Values specified in the <c>DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE</c> - in particular the <c>commandName</c> that initiated the request and its <c>requestId</c> - are
           specified in the initial command callback's passed in <c>DIGITALTWIN_CLIENT_COMMAND_REQUES</c>T.

  @param[in] dtInterfaceClientHandle        Handle for the interface client.
  @param[in] dtClientAsyncCommandUpdate     <c>DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE</c> containing updates about the status to send to the server.
  @param[in] dtUpdateAsyncCommandCallback   (Optional) Function pointer to be invoked when the update is successfully delivered or fails.
  @param[in] userContextCallback            (Optional) Context pointer passed to dtUpdateAsyncCommandCallback function when it is invoked.

  @returns  A DIGITALTWIN_CLIENT_RESULT.
*/

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle, const DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE*, dtClientAsyncCommandUpdate, DIGITALTWIN_CLIENT_UPDATE_ASYNC_COMMAND_CALLBACK, dtUpdateAsyncCommandCallback, void*, userContextCallback);

/** 
  @brief Destroys interface handle

  @remarks <c>DigitalTwin_InterfaceClient_Destroy</c> destroys the interface handle.  Device applications *MUST NOT* call *any* DigitalTwin_InterfaceClient_* API's on the handle
           after calling Destroy.

  @remarks This call does *NOT* cause any network I/O to occur.  This means that the server still considers the interface has having been registered.  We do not delete
           the interface from the server because otherwise service applications would be unable to update properties on this interface when the device application
           is not connected.  Updating properties on disconnected devices is a supported.

  @remarks After <c>DigitalTwin_InterfaceClient_Destroy</c> is called, no further callbacks to the interface client will arrive.  It as safe to free resources associated with it.

  @warning If an application accesses the dtInterfaceClient handle after calling <c>DigitalTwin_InterfaceClient_Destroy</c>, the application may crash.

  @param[in] dtInterfaceClientHandle     Handle for the interface client.

  @returns  N/A.
*/
MOCKABLE_FUNCTION(, void, DigitalTwin_InterfaceClient_Destroy, DIGITALTWIN_INTERFACE_CLIENT_HANDLE, dtInterfaceClientHandle);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_INTERFACE_CLIENT_H
