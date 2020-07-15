// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// PnP is a convention built on top of existing IoTHub device transport primitives.
// This header implements helper functions to aide applications in serializing and deserializing PnP data.
// Note that actually sending and receiving the data from the underlying IoTHub client is the application's responsibility.
//

#ifndef PNP_PROTOCOL_HELPERS_H
#define PNP_PROTOCOL_HELPERS_H

#include "azure_c_shared_utility/strings.h"
#include "iothub_client_core_common.h"
#include "iothub_message.h"
#include "parson.h"

// 
// Status codes for PnP, closely mapping to HTTP status.
//
#define PNP_STATUS_SUCCESS 200
#define PNP_STATUS_BAD_FORMAT 400
#define PNP_STATUS_NOT_FOUND  404
#define PNP_STATUS_INTERNAL_ERROR 500

//
// The PnP convention defines the maximum length of a component 
//
#define PNP_MAXIMUM_COMPONENT_LENGTH 64

//
// PnPHelperPropertyCallbackFunction defines the function prototype the application implements to receive a callback for each PnP property in a given Device Twin.
// 
typedef void (*PnPHelperPropertyCallbackFunction)(const char* componentName, const char* propertyName, JSON_Value* propertyValue, int version, void* userContextCallback);

//
// PnPHelper_CreateReportedProperty returns JSON to report a property's value from the device.  This does NOT contain any metadata such as 
// a result code or version.  It is used when sending properties that are NOT marked as <"writable": true> in the DTDL defining
// the given property.
//
// The application itself needs to send this to Device Twin, using a function such as IoTHubDeviceClient_SendReportedState.
//
STRING_HANDLE PnPHelper_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue);

//
// PnPHelper_CreateReportedProperty returns JSON to report a property's value from the device.  This contains metadata such as 
// a result code or version.  It is used when responding to a desired property change request from the server, and in particular
// for properties marked with <"writable": true>.
// For instance, after processing a thermostat's set point the application acknowledges that it has received the request and can indicate 
// whether it will attempt to honor the requset or whether the request was unsuccessful.
//
// The application itself needs to send this to Device Twin, using a function such as IoTHubDeviceClient_SendReportedState.
//
STRING_HANDLE PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int result, const char* description, int ackVersion);

// 
// PnPHelper_ParseCommandName is invoked by the application when an incoming device method arrives.  This function
// parses the device method name into the targeted (optional) component and PnP specific command.  Note that 
// because we don't want to allocate separate buffers for componentName and pnpCommandName and we can't modify deviceMethodName in place,
// we return the componentName as a non-NULL terminated character array with its length in componentNameSize.
//
void PnPHelper_ParseCommandName(const char* deviceMethodName, unsigned const char** componentName, size_t* componentNameSize, const char** pnpCommandName);

//
// PnPHelper_CreateTelemetryMessageHandle creates an IOTHUB_MESSAGE_HANDLE that contains tho contents of the telemetryData.
// If the optional componentName parameter is specified, the created message will have this as a property.
//
// The application itself needs to send this to IoTHub, using a function such as IoTHubDeviceClient_SendEventAsync.
//
IOTHUB_MESSAGE_HANDLE PnPHelper_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData);

//
// PnPHelper_ProcessTwinData is invoked by the application when a device twin arrives to its device twin processing callback.
// PnPHelper_ProcessTwinData will visit the children of the desired portion of the twin and invoke the device's pnpPropertyCallback
// function for each property that it visits.
// 
bool PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, const char** componentsInModel, size_t numComponentsInModel, PnPHelperPropertyCallbackFunction pnpPropertyCallback, void* userContextCallback);

//
// PnPHelper_CopyTwinPayloadToString takes the payload data, which arrives as a potentially non-NULL terminated string from the IoTHub SDK, and creates
// a new copy of the data with a NULL terminator.  The JSON parser this sample uses, parson, only operates over NULL terminated strings.
//
char* PnPHelper_CopyPayloadToString(const unsigned char* payload, size_t size);

#endif /* PNP_PROTOCOL_HELPERS_H */
