// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This header implements a simulated thermostat as defined by dtmi:com:example:Thermostat;1.  In particular,
// this Thermostat component is defined to be run as a subcomponent by the temperature controller interface
// defined by https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json.  The 
// temperature controller defines two components that implement dtmi:com:example:Thermostat;1, named thermostat1 and
// thermostat2.  
//
// The code in this header/.c file is designed to be generic so that the calling application can call PnP_ThermostatComponent_CreateHandle
// multiple times and then pass processing of a given component (thermostat1 or thermostat2) to the appropriate helper.
//

#ifndef PNP_THERMOSTAT_CONTROLLER_H
#define PNP_THERMOSTAT_CONTROLLER_H

#include "parson.h"
#include "iothub_device_client.h"

//
// Handle representing a thermostat component.
//
typedef void* PNP_THERMOSTAT_COMPONENT_HANDLE;

//
// PnP_ThermostatComponent_CreateHandle allocates a handle to correspond to the thermostat controller.
// This operation is only for allocation and does NOT invoke any I/O operations.
//
PNP_THERMOSTAT_COMPONENT_HANDLE PnP_ThermostatComponent_CreateHandle(const char* componentName);

//
// PnP_ThermostatComponent_Destroy frees resources associated with pnpThermostatComponentHandle.
//
void PnP_ThermostatComponent_Destroy(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle);

//
// PnP_ThermostatComponent_ProcessCommand is used to process any incoming PnP Commands, transferred via the IoTHub device method channel,
// to the given PNP_THERMOSTAT_COMPONENT_HANDLE.  The function returns an HTTP style return code to indicate success or failure.
//
int PnP_ThermostatComponent_ProcessCommand(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, const char *pnpCommandName, JSON_Value* commandJsonValue, unsigned char** response, size_t* responseSize);

//
// PnP_ThermostatComponent_ProcessPropertyUpdate processes an incoming property update and, if the property is in the thermostat's model, will
// send a reported property acknowledging receipt of the property request from IoTHub.
//
void PnP_ThermostatComponent_ProcessPropertyUpdate(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, const char* propertyName, JSON_Value* propertyValue, int version);

//
// PnP_ThermostatComponent_SendTelemetry sends telemetry indicating the current temperature of the thermostat.
//
void PnP_ThermostatComponent_SendTelemetry(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient);

//
// PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property sends a property indicating maxTempSinceLastReboot.  Since 
// this property is not "writeable" in the DTDL, the application can invoke this at startup.
//
void PnP_TempControlComponent_Report_MaxTempSinceLastReboot_Property(PNP_THERMOSTAT_COMPONENT_HANDLE pnpThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient);

#endif /* PNP_THERMOSTAT_CONTROLLER_H */
