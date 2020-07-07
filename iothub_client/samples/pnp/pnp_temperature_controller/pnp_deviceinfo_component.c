// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// PnP helper routines
#include "pnp_deviceinfo_component.h"
#include "pnp_protocol_helpers.h"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

// Property names along with their simulated values.  
// NOTE: the properties must be legal JSON values.  This means that strings must be encoded, 
// e.g. const char sample="\"value-to-send-in-quotes\"";

#define PNP_ENCODE_STRING_FOR_JSON(str) "\"str\""

static const char PnPDeviceInfo_SoftwareVersionPropertyName[] = "swVersion";
static const char PnPDeviceInfo_SoftwareVersionPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(1.0.0.0);

static const char PnPDeviceInfo_ManufacturerPropertyName[] = "manufacturer";
static const char PnPDeviceInfo_ManufacturerPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(Sample-Manufacturer);

static const char PnPDeviceInfo_ModelPropertyName[] = "model";
static const char PnPDeviceInfo_ModelPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(ample-Model-123);

static const char PnPDeviceInfo_OsNamePropertyName[] = "osName";
static const char PnPDeviceInfo_OsNamePropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(sample-OperatingSystem-name);

static const char PnPDeviceInfo_ProcessorArchitecturePropertyName[] = "processorArchitecture";
static const char PnPDeviceInfo_ProcessorArchitecturePropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(Contoso-Arch-64bit);

static const char PnPDeviceInfo_ProcessorManufacturerPropertyName[] = "processorManufacturer";
static const char PnPDeviceInfo_ProcessorManufacturerPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON(Processor Manufacturer(TM));

// The storage and memory fields below are doubles and hence should NOT be escaped, as these are legal JSON values.
static const char PnPDeviceInfo_TotalStoragePropertyName[] = "totalStorage";
static const char PnPDeviceInfo_TotalStoragePropertyValue[] = "10000";

static const char PnPDeviceInfo_TotalMemoryPropertyName[] = "totalMemory";
static const char PnPDeviceInfo_TotalMemoryPropertyValue[] = "200";


//
// SendReportedPropertyForDeviceInfo sends a property as part of DeviceInfo component.
//
static void SendReportedPropertyForDeviceInfo(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, const char* componentName, const char* propertyName, const char* propertyValue)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    STRING_HANDLE jsonToSend = NULL;

    if ((jsonToSend = PnPHelper_CreateReportedProperty(componentName, propertyName, propertyValue)) == NULL)
    {
        LogError("Unable to build reported property response");
    }
    else
    {
        const char* jsonToSendStr = STRING_c_str(jsonToSend);
        size_t jsonToSendStrLen = strlen(jsonToSendStr);

        if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state.  Error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending acknowledgement of property to IoTHub");
        }
    }

    STRING_delete(jsonToSend);
}

void PnP_DeviceInfoComponent_ReportInfo(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, const char* componentName)
{
    // NOTE: It is possible to put multiple property updates into a single JSON and IoTHubDeviceClient_SendReportedState invocation.
    // This sample does not do so for clarity, though production devices should seriously consider such property update batching to
    // save bandwidth.  The underlying PnP_Helper routines currently do not accept an array.
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_SoftwareVersionPropertyName, PnPDeviceInfo_SoftwareVersionPropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_ManufacturerPropertyName, PnPDeviceInfo_ManufacturerPropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_ModelPropertyName, PnPDeviceInfo_ModelPropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_OsNamePropertyName, PnPDeviceInfo_OsNamePropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_ProcessorArchitecturePropertyName, PnPDeviceInfo_ProcessorArchitecturePropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_ProcessorManufacturerPropertyName, PnPDeviceInfo_ProcessorManufacturerPropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_TotalStoragePropertyName, PnPDeviceInfo_TotalStoragePropertyValue);
    SendReportedPropertyForDeviceInfo(deviceClient, componentName, PnPDeviceInfo_TotalMemoryPropertyName, PnPDeviceInfo_TotalMemoryPropertyValue);
}

