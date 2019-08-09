// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header file for sample for manipulating DigitalTwin Interface for device info.

#ifndef DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR
#define DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR

#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Creates a new DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this interface.
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleEnvironmentalSensor_CreateInterface(void);
// Sends DigitalTwin telemetry messages about current environment
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleEnvironmentalSensor_SendTelemetryMessagesAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

// Periodically checks to see if an asychronous command for running diagnostics has been queued, and process it if so.
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleEnvironmentalSensor_ProcessDiagnosticIfNecessaryAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

// Closes down resources associated with device info interface.
void DigitalTwinSampleEnvironmentalSensor_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR
