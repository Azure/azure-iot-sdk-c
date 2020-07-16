// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// The DTDL for this interface is defined at https://repo.azureiotrepository.com/Models/dtmi:azure:DeviceManagement:DeviceInformation;1?api-version=2020-05-01-preview

// PnP helper routines
#include "pnp_deviceinfo_component.h"
#include "pnp_protocol_helpers.h"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

// Property names along with their simulated values.  
// NOTE: the property values must be legal JSON values.  Strings specifically must be enclosed with an extra set of quotes to be legal json string values.
// The property names in this sample do not hard-code the extra quotes because the underlying PnP helper adds this to names automatically.
#define PNP_ENCODE_STRING_FOR_JSON(str) #str

static const char PnPDeviceInfo_SoftwareVersionPropertyName[] = "swVersion";
static const char PnPDeviceInfo_SoftwareVersionPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("1.0.0.0");

static const char PnPDeviceInfo_ManufacturerPropertyName[] = "manufacturer";
static const char PnPDeviceInfo_ManufacturerPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("Sample-Manufacturer");

static const char PnPDeviceInfo_ModelPropertyName[] = "model";
static const char PnPDeviceInfo_ModelPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("sample-Model-123");

static const char PnPDeviceInfo_OsNamePropertyName[] = "osName";
static const char PnPDeviceInfo_OsNamePropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("sample-OperatingSystem-name");

static const char PnPDeviceInfo_ProcessorArchitecturePropertyName[] = "processorArchitecture";
static const char PnPDeviceInfo_ProcessorArchitecturePropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("Contoso-Arch-64bit");

static const char PnPDeviceInfo_ProcessorManufacturerPropertyName[] = "processorManufacturer";
static const char PnPDeviceInfo_ProcessorManufacturerPropertyValue[] = PNP_ENCODE_STRING_FOR_JSON("Processor Manufacturer(TM)");

// The storage and memory fields below are doubles.  They should NOT be escaped since they will be legal JSON values when output.
static const char PnPDeviceInfo_TotalStoragePropertyName[] = "totalStorage";
static const char PnPDeviceInfo_TotalStoragePropertyValue[] = "10000";

static const char PnPDeviceInfo_TotalMemoryPropertyName[] = "totalMemory";
static const char PnPDeviceInfo_TotalMemoryPropertyValue[] = "200";

//
// SendReportedPropertyForDeviceInformation sends a property as part of DeviceInfo component.
//
static void SendReportedPropertyForDeviceInformation(IOTHUB_DEVICE_CLIENT_HANDLE deviceClient, const char* componentName, const char* propertyName, const char* propertyValue)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    STRING_HANDLE jsonToSend = NULL;

    if ((jsonToSend = PnPHelper_CreateReportedProperty(componentName, propertyName, propertyValue)) == NULL)
    {
        LogError("Unable to build reported property response for propertyName=%s, propertyValue=%s", propertyName, propertyValue);
    }
    else
    {
        const char* jsonToSendStr = STRING_c_str(jsonToSend);
        size_t jsonToSendStrLen = strlen(jsonToSendStr);

        if ((iothubClientResult = IoTHubDeviceClient_SendReportedState(deviceClient, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state for property=%s, error=%d", propertyName, iothubClientResult);
        }
        else
        {
            LogInfo("Sending device information property to IoTHub.  propertyName=%s, propertyValue=%s", propertyName, propertyValue);
        }
    }

    STRING_delete(jsonToSend);
}

void PnP_DeviceInfoComponent_Report_All_Properties(const char* componentName, IOTHUB_DEVICE_CLIENT_HANDLE deviceClient)
{
    // NOTE: It is possible to put multiple property updates into a single JSON and IoTHubDeviceClient_SendReportedState invocation.
    // This sample does not do so for clarity, though production devices should seriously consider such property update batching to
    // save bandwidth.  The underlying PnP_Helper routines currently do not accept an array.
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_SoftwareVersionPropertyName, PnPDeviceInfo_SoftwareVersionPropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_ManufacturerPropertyName, PnPDeviceInfo_ManufacturerPropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_ModelPropertyName, PnPDeviceInfo_ModelPropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_OsNamePropertyName, PnPDeviceInfo_OsNamePropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_ProcessorArchitecturePropertyName, PnPDeviceInfo_ProcessorArchitecturePropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_ProcessorManufacturerPropertyName, PnPDeviceInfo_ProcessorManufacturerPropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_TotalStoragePropertyName, PnPDeviceInfo_TotalStoragePropertyValue);
    SendReportedPropertyForDeviceInformation(deviceClient, componentName, PnPDeviceInfo_TotalMemoryPropertyName, PnPDeviceInfo_TotalMemoryPropertyValue);
}
