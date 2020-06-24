// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifndef PNP_PROTOCOL_HELPERS_H
#define PNP_PROTOCOL_HELPERS_H

#include "iothub_client_core_common.h"
#include "iothub_message.h"

// This is prototype for function that application writes to process property changes.
typedef void (*PnPHelperPropertyCallbackFunction)(const char* componentName, const char* propertyName, const char* propertyValue, int version);

STRING_HANDLE PnPHelper_CreateReportedProperty(const char* componentName, const char* propertyName, const char* propertyValue);

STRING_HANDLE PnPHelper_CreateReportedPropertyWithStatus(const char* componentName, const char* propertyName, const char* propertyValue, int ackCode, const char* description, int ackVersion);

void PnPHelper_ParseCommandName(const char* deviceMethodName, const char** componentName, size_t* componentNameLength, const char** commandName, size_t* commandNameLength);

IOTHUB_MESSAGE_HANDLE PnPHelper_CreateTelemetryMessageHandle(const char* componentName, const char* telemetryData);

void PnPHelper_ProcessTwinData(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payLoad, size_t size, PnPHelperPropertyCallbackFunction callbackFromApplication);


#endif /* PNP_PROTOCOL_HELPERS_H */


