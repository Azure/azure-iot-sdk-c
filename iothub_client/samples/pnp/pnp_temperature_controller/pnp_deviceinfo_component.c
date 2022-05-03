// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// The DTDL for this interface is defined at https://repo.azureiotrepository.com/Models/dtmi:azure:DeviceManagement:DeviceInformation;1?api-version=2020-05-01-preview

// PnP routines
#include "pnp_deviceinfo_component.h"
#include "iothub_client_properties.h"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

// Property names along with their simulated values.  
// NOTE: the property values must be legal JSON values.  Strings specifically must be enclosed with an extra set of quotes to be legal json string values.
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

void PnP_DeviceInfoComponent_Report_All_Properties(const char* componentName, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    unsigned char* propertiesSerialized = NULL;
    size_t propertiesSerializedLength;
    IOTHUB_CLIENT_PROPERTY_REPORTED properties[] =  {
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_SoftwareVersionPropertyName, PnPDeviceInfo_SoftwareVersionPropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_ManufacturerPropertyName, PnPDeviceInfo_ManufacturerPropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_ModelPropertyName, PnPDeviceInfo_ModelPropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_OsNamePropertyName, PnPDeviceInfo_OsNamePropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_ProcessorArchitecturePropertyName, PnPDeviceInfo_ProcessorArchitecturePropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_ProcessorManufacturerPropertyName, PnPDeviceInfo_ProcessorManufacturerPropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_TotalStoragePropertyName, PnPDeviceInfo_TotalStoragePropertyValue},
        {IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, PnPDeviceInfo_TotalMemoryPropertyName, PnPDeviceInfo_TotalMemoryPropertyValue}
    };

    const size_t numProperties = sizeof(properties) / sizeof(properties[0]);

    // The first step of reporting properties is to serialize IOTHUB_CLIENT_PROPERTY_REPORTED into JSON for sending.
    if ((iothubClientResult = IoTHubClient_Properties_Serializer_CreateReported(properties, numProperties, componentName, &propertiesSerialized, &propertiesSerializedLength)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to serialize reported state, error=%d", iothubClientResult);
    }
    // The output of IoTHubClient_Properties_Serializer_CreateReported is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, propertiesSerialized, propertiesSerializedLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send reported state, error=%d", iothubClientResult);
    }

    IoTHubClient_Properties_Serializer_Destroy(propertiesSerialized);
}
