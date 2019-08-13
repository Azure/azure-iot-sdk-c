// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "azure_c_shared_utility/xlogging.h"
#include "../digitaltwin_sample_device_info.h"
#include "deviceinfo_ops.h"

// Sample values
static const char digitaltwinSample_SoftwareVersionData[] = "software-revision-1.0.2A";
static const char digitaltwinSample_ManufacturerData[] = "Sample Manufacturer for Azure IoT C SDK enabled device";
static const char digitaltwinSample_ModelData[] = "Sample Model 123";
static const char digitaltwinSample_OsNameData[] = "sample-OperatingSystem-name";
static const char digitaltwinSample_ProcessorArchitectureData[] = "Contoso-Arch-64bit";
static const char digitaltwinSample_ProcessorManufacturerData[] = "Processor Manufacturer(TM)";
static const int  digitaltwinSample_TotalStorageData = 83886080; // 80GB
static const int  digitaltwinSample_TotalMemoryData = 32768; // 32 MB

int DeviceInformation_SwVersionProperty(char ** returnValue)
{
    const char* swVersion = digitaltwinSample_SoftwareVersionData;
    
    return DI_serializeCharData(swVersion, returnValue);
}

int DeviceInformation_ManufacturerProperty(char ** returnValue)
{
    const char* manufacturer = digitaltwinSample_ManufacturerData;
    
    return DI_serializeCharData(manufacturer, returnValue);
}

int DeviceInformation_ModelProperty(char ** returnValue)
{
    const char* model = digitaltwinSample_ModelData;
    
    return DI_serializeCharData(model, returnValue);
}

int DeviceInformation_OsNameProperty(char ** returnValue)
{
    const char* osName = digitaltwinSample_OsNameData;
    
    return DI_serializeCharData(osName, returnValue);
}

int DeviceInformation_ProcessorArchitectureProperty(char ** returnValue)
{
    const char* processArch = digitaltwinSample_ProcessorArchitectureData;
    
    return DI_serializeCharData(processArch, returnValue);
}

int DeviceInformation_ProcessorManufacturerProperty(char ** returnValue)
{
    const char* processorManufacturer = digitaltwinSample_ProcessorManufacturerData;
    
    return DI_serializeCharData(processorManufacturer, returnValue);
}

int DeviceInformation_TotalStorageProperty(char ** returnValue)
{
    const int totalStorage = digitaltwinSample_TotalStorageData;
    
    return DI_serializeIntegerData(totalStorage, returnValue);
}

int DeviceInformation_TotalMemoryProperty(char ** returnValue)
{
    const int totalMemory = digitaltwinSample_TotalMemoryData;
    
    return DI_serializeIntegerData(totalMemory, returnValue);
}
