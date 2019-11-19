// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../digitaltwin_sample_device_info.h"
#include "deviceinfo_ops.h"

#ifndef DIGITALTWIN_SAMPLE_SOFTWARE_VERSION
#define DIGITALTWIN_SAMPLE_SOFTWARE_VERSION "1.0.0.0"
#endif

#ifndef DIGITALTWIN_SAMPLE_MANUFACTURER
#define DIGITALTWIN_SAMPLE_MANUFACTURER "Sample-Manufacturer"
#endif

#ifndef DIGITALTWIN_SAMPLE_MODEL
#define DIGITALTWIN_SAMPLE_MODEL "Sample-Model-123"
#endif

#ifndef DIGITALTWIN_SAMPLE_OS_NAME
#define DIGITALTWIN_SAMPLE_OS_NAME "sample-OperatingSystem-name"
#endif

#ifndef DIGITALTWIN_SAMPLE_PROCESSOR_ARCHITECTURE
#define DIGITALTWIN_SAMPLE_PROCESSOR_ARCHITECTURE "Contoso-Arch-64bit"
#endif

#ifndef DIGITALTWIN_SAMPLE_PROCESSOR_MANUFACTURER
#define DIGITALTWIN_SAMPLE_PROCESSOR_MANUFACTURER "Processor Manufacturer(TM)"
#endif

#ifndef DIGITALTWIN_SAMPLE_TOTAL_STORAGE
#define DIGITALTWIN_SAMPLE_TOTAL_STORAGE 83886080 // 80GB
#endif

#ifndef DIGITALTWIN_SAMPLE_TOTAL_MEMORY
#define DIGITALTWIN_SAMPLE_TOTAL_MEMORY 32768 // 32 MB
#endif

int DeviceInformation_SwVersionProperty(char ** returnValue)
{   
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_SOFTWARE_VERSION, returnValue);
}

int DeviceInformation_ManufacturerProperty(char ** returnValue)
{   
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_MANUFACTURER, returnValue);
}

int DeviceInformation_ModelProperty(char ** returnValue)
{   
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_MODEL, returnValue);
}

int DeviceInformation_OsNameProperty(char ** returnValue)
{   
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_OS_NAME, returnValue);
}

int DeviceInformation_ProcessorArchitectureProperty(char ** returnValue)
{
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_PROCESSOR_ARCHITECTURE, returnValue);
}

int DeviceInformation_ProcessorManufacturerProperty(char ** returnValue)
{   
    return DI_serializeCharData(DIGITALTWIN_SAMPLE_PROCESSOR_MANUFACTURER, returnValue);
}

int DeviceInformation_TotalStorageProperty(char ** returnValue)
{ 
    return DI_serializeIntegerData(DIGITALTWIN_SAMPLE_TOTAL_STORAGE, returnValue);
}

int DeviceInformation_TotalMemoryProperty(char ** returnValue)
{
    return DI_serializeIntegerData(DIGITALTWIN_SAMPLE_TOTAL_MEMORY, returnValue);
}
