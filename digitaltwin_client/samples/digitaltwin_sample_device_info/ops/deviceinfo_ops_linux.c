// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "azure_c_shared_utility/xlogging.h"
#include "../digitaltwin_sample_device_info.h"
#include "deviceinfo_ops.h"

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

static const char DI_Linux_Source_Path_Manufacturer[] = "/sys/class/dmi/id/sys_vendor";
static const char DI_Linux_Source_Path_Model[] = "/sys/class/dmi/id/product_name";

static const char DI_Linux_Source_Path_ProcessManufacturer[] = "/proc/cpuinfo";
static const char DI_Linux_Source_Field_cpuInfoVendorId[] = "vendor_id\t: ";

int DI_serializeCharDataFromFile(const char* sourcePath, char** returnValue)
{
    char* buf = NULL;
    size_t size = 0;
    bool found = false;
    FILE* f = NULL;

    if ((f = fopen(sourcePath, "r")) != NULL)
    {
        if (getline(&buf, &size, f) == -1)
        {
            LogInfo("Failed to read file - %s.", sourcePath);
            buf = "";
        }
    }
    else
    {
        LogInfo("Failed to open file - %s. Please ensure the sample is running as root user.", sourcePath);
        buf = "";
    }

    if (f != NULL)
    {
        fclose(f);
    }

    return DI_serializeCharData(buf, returnValue);
}

int DeviceInformation_TotalStorageProperty(char** returnValue)
{
    struct statvfs stat;

    if (statvfs("/", &stat) != 0) 
    {
        LogInfo("Failed to retrieve system information - statvfs(). Please ensure the sample is running as root user.");
        return DI_serializeIntegerData(-1, returnValue);
    }

    return DI_serializeIntegerData(stat.f_frsize * stat.f_blocks, returnValue);
}

int DeviceInformation_TotalMemoryProperty(char** returnValue)
{
    struct sysinfo si;

    if (sysinfo(&si) != 0)
    {
        LogInfo("Failed to retrieve system information - sysinfo(). Please ensure the sample is running as root user.");
        return DI_serializeIntegerData(-1, returnValue);
    }

    return DI_serializeIntegerData((si.totalram / 1024), returnValue);
}

int DeviceInformation_SwVersionProperty(char** returnValue)
{
    struct utsname buf;

    if (uname(&buf) != 0)
    {
        LogInfo("Failed to retrieve system information - uname(). Please ensure the sample is running as root user.");
        return DI_serializeCharData("", returnValue);
    }

    return DI_serializeCharData(buf.release, returnValue);
}

int DeviceInformation_OsNameProperty(char** returnValue)
{
    struct utsname buf;

    if (uname(&buf) != 0)
    {
        LogInfo("Failed to retrieve system information - uname(). Please ensure the sample is running as root user.");
        return DI_serializeCharData("", returnValue);
    }
    
    return DI_serializeCharData(buf.sysname, returnValue);
}

int DeviceInformation_ProcessorArchitectureProperty(char** returnValue)
{
    struct utsname buf;

    if (uname(&buf) != 0)
    {
        LogInfo("Failed to retrieve system information - uname(). Please ensure the sample is running as root user.");
        return DI_serializeCharData("", returnValue);
    }

    return DI_serializeCharData(buf.machine, returnValue);
}

int DeviceInformation_ManufacturerProperty(char** returnValue)
{
    return DI_serializeCharDataFromFile(DI_Linux_Source_Path_Manufacturer, returnValue);
}

int DeviceInformation_ModelProperty(char** returnValue)
{
    return DI_serializeCharDataFromFile(DI_Linux_Source_Path_Model, returnValue);
}

int DeviceInformation_ProcessorManufacturerProperty(char** returnValue)
{
    FILE* f;
    char* searchBuf = NULL;
    char* buf = NULL;
    int retVal = -1;
    size_t size = 0;

    // Search for the first vendor_id field in cpuinfo
    if ((f = fopen(DI_Linux_Source_Path_ProcessManufacturer, "r")) != NULL)
    {
        while (getline(&searchBuf, &size, f) != -1)
        {
            if (strstr(searchBuf, DI_Linux_Source_Field_cpuInfoVendorId) != NULL)
            {
                buf = &searchBuf[sizeof(DI_Linux_Source_Field_cpuInfoVendorId) - 1];
                retVal = DI_serializeCharData(buf, returnValue);
                break;
            }
        }

        if(buf == NULL)
        {
            LogInfo("Failed to retrieve system information - %s. Please ensure the sample is running as root user.", DI_Linux_Source_Path_ProcessManufacturer);
            buf = "";
        }
    }

    if (f != NULL)
    {
        fclose(f);
    }

    if (searchBuf != NULL)
    {
        free(searchBuf);
    }

    return retVal;
}