// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Implements a sample DeviceInfo DigitalTwin interface.  As the name implies, this interface returns
// information about the device such as the OS version, amount of storage, etc.
//
// Except under extraordinary conditions (e.g. a firmware update or memory upgrade), this
// information never changes during the lifetime of a device, and almost certainly does not
// change over the lifecycle on a given DigitalTwin session.

#include "digitaltwin_sample_device_info.h"
#include "ops/deviceinfo_ops.h"
#include "azure_c_shared_utility/xlogging.h"
#include <stdlib.h>

//
// Application state associated with the particular interface.  It contains 
// the DIGITALTWIN_INTERFACE_CLIENT_HANDLE used for reporting properties.
//
typedef struct DIGITALTWIN_SAMPLE_DEVICEINFO_STATE_TAG
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle;
} DIGITALTWIN_SAMPLE_DEVICEINFO_STATE;


// State for interface.  For simplicity we set this as a global and set during DigitalTwin_InterfaceClient_Create, but  
// callbacks of this interface don't reference it directly but instead use userContextCallback passed to them.
DIGITALTWIN_SAMPLE_DEVICEINFO_STATE digitaltwinSample_DeviceInfoState;


// DigitalTwin interface name from service perspective.
static const char digitaltwinSampleDeviceInfo_InterfaceId[] = "urn:azureiot:DeviceManagement:DeviceInformation:1";
static const char digitaltwinSampleDeviceInfo_InterfaceName[] = "sampleDeviceInfo";

// DigitalTwinSampleDeviceInfo_PropertyCallback is invoked when a property is updated (or failed) going to server.
// In this sample, we route ALL property callbacks to this function and just have the userContextCallback set
// to the propertyName.  Product code will potentially have context stored in this userContextCallback.
static void DigitalTwinSampleDeviceInfo_PropertyCallback(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    if (dtReportedStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("DEVICE_INFO: Property callback property=<%s> succeeds", (const char*)userContextCallback);
    }
    else
    {
        LogError("DEVICE_INFO: Property callback property=<%s> fails, error=<%s>",  (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtReportedStatus));
    }
}

//
// DigitalTwinSampleDeviceInfo_ReportPropertyAsync is a helper function to report a DeviceInfo's properties.
// It invokes underlying DigitalTwin API for reporting properties and sets up its callback on completion.
//
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportPropertyAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, const char* propertyName, const char* propertyData)
{
    DIGITALTWIN_CLIENT_RESULT result;

    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, propertyName, 
                                                             propertyData, NULL,
                                                             DigitalTwinSampleDeviceInfo_PropertyCallback, (void*)propertyName);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DEVICE_INFO: Reporting property=<%s> failed, error=<%s>", propertyName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        LogInfo("DEVICE_INFO: Queued async report read only property for %s", propertyName);
    }

    return result;

}

// Sends a reported property for Software Version of this device.
static const char digitaltwinSample_SoftwareVersionProperty[] = "swVersion";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportSoftwareVersionAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *swVersionPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_SwVersionProperty(&swVersionPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_SoftwareVersionProperty, 
                                                                 swVersionPropertyValue);
        free(swVersionPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Manufacturer of this device.
static const char digitaltwinSample_ManufacturerProperty[] = "manufacturer";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportManufacturerAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *manufacturerPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_ManufacturerProperty(&manufacturerPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_ManufacturerProperty, 
                                                                 manufacturerPropertyValue);
        free(manufacturerPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Model of this device.
static const char digitaltwinSample_ModelProperty[] = "model";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportModelAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *modelPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_ModelProperty(&modelPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_ModelProperty, 
                                                                 modelPropertyValue);
        free(modelPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Name of this device.
static const char digitaltwinSample_OsNameProperty[] = "osName";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportOSNameAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *osNamePropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_OsNameProperty(&osNamePropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_OsNameProperty, 
                                                                 osNamePropertyValue);
        free(osNamePropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Processor Architecture of this device.
static const char digitaltwinSample_ProcessorArchitectureProperty[] = "processorArchitecture";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportProcessorArchitectureAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *processorArchPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_ProcessorArchitectureProperty(&processorArchPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_ProcessorArchitectureProperty,
                                                                 processorArchPropertyValue);
        free(processorArchPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Processor Manufacturer of this device.
static const char digitaltwinSample_ProcessorManufacturerProperty[] = "processorManufacturer";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportProcessorManufacturerAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *processorManufacturerPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_ProcessorManufacturerProperty(&processorManufacturerPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_ProcessorManufacturerProperty,
                                                                 processorManufacturerPropertyValue);
        free(processorManufacturerPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Total Storage of this device (a long, in kilobytes)
static const char digitaltwinSample_TotalStorageProperty[] = "totalStorage";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportTotalStorageAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *totalStoragePropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_TotalStorageProperty(&totalStoragePropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_TotalStorageProperty,
                                                                 totalStoragePropertyValue);
        free(totalStoragePropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// Sends a reported property for Total Memory of this device (a long, in kilobytes)
static const char digitaltwinSample_TotalMemoryProperty[] = "totalMemory";

static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportTotalMemoryAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    char *totalMemoryPropertyValue = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    
    if(DeviceInformation_TotalMemoryProperty(&totalMemoryPropertyValue) == RESULT_OK)
    {
        result = DigitalTwinSampleDeviceInfo_ReportPropertyAsync(interfaceHandle, digitaltwinSample_TotalMemoryProperty, 
                                                                 totalMemoryPropertyValue);
        free(totalMemoryPropertyValue);
    }
    else
    {
        result = DIGITALTWIN_CLIENT_ERROR;
    }
    
    return result;
}

// DigitalTwinSampleDeviceInfo_ReportDeviceInfoAsync() reports all DeviceInfo properties to the service
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleDeviceInfo_ReportDeviceInfoAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;

    // NOTE: Future versions of SDK will support ability to send mulitiple properties in a single
    // send.  For now, one at a time is sufficient albeit less effecient.
    if (((result = DigitalTwinSampleDeviceInfo_ReportSoftwareVersionAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportManufacturerAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportModelAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportOSNameAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportProcessorArchitectureAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportProcessorManufacturerAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportTotalStorageAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleDeviceInfo_ReportTotalMemoryAsync(interfaceHandle)) != DIGITALTWIN_CLIENT_OK))
    {
        LogError("DEVICE_INFO: Reporting properties failed.");
    }
    else
    {
        LogInfo("DEVICE_INFO: Queing of all properties to be reported has succeeded");
    }

    return result;
}


// DigitalTwinSampleDeviceInfo_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void DigitalTwinSampleDeviceInfo_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_DEVICEINFO_STATE *deviceInfoState = (DIGITALTWIN_SAMPLE_DEVICEINFO_STATE*)userInterfaceContext;
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // Once the interface is registered, send our reported properties to the service.  
        // It *IS* safe to invoke most DigitalTwin API calls from a callback thread like this, though it 
        // is NOT safe to create/destroy/register interfaces now.
        LogInfo("DEVICE_INFO: Interface successfully registered.");
        DigitalTwinSampleDeviceInfo_ReportDeviceInfoAsync(deviceInfoState->interfaceClientHandle);
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("DEVICE_INFO: Interface received unregistering callback.");
    }
    else 
    {
        LogError("DEVICE_INFO: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}


//
// DigitalTwinSampleDeviceInfo_CreateInterface is the initial entry point into the DigitalTwin Sample Device Info interface.
// It simply creates a DIGITALTWIN_INTERFACE_CLIENT_HANDLE that is mapped to the DeviceInfo interface name.
// This call is synchronous, as simply creating an interface only performs initial allocations.
//
// NOTE: The actual registration of this interface is left to the caller, which may register 
// multiple interfaces on one DIGITALTWIN_DEVICE_CLIENT_HANDLE.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleDeviceInfo_CreateInterface(void)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;

    memset(&digitaltwinSample_DeviceInfoState, 0, sizeof(digitaltwinSample_DeviceInfoState));

    if ((result = DigitalTwin_InterfaceClient_Create(digitaltwinSampleDeviceInfo_InterfaceId, digitaltwinSampleDeviceInfo_InterfaceName, DigitalTwinSampleDeviceInfo_InterfaceRegisteredCallback, &digitaltwinSample_DeviceInfoState, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("DEVICE_INFO: Unable to allocate interface client handle for interfaceId=<%s>, interfaceName=<%s>, error=<%s>", digitaltwinSampleDeviceInfo_InterfaceId, digitaltwinSampleDeviceInfo_InterfaceName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else
    {
        digitaltwinSample_DeviceInfoState.interfaceClientHandle = interfaceHandle;
        LogInfo("DEVICE_INFO: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  interfaceId=<%s>, interfaceName=<%s>, handle=<%p>", digitaltwinSampleDeviceInfo_InterfaceId, digitaltwinSampleDeviceInfo_InterfaceName, interfaceHandle);
    }

    return interfaceHandle;
}

// DigitalTwinSampleDeviceInfo_Close is invoked when the sample device is shutting down.
// Although currently this just frees the interfaceHandle, had the interface
// been more complex this would also be a chance to cleanup additional resources it created.
void DigitalTwinSampleDeviceInfo_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(interfaceHandle);

    // After DigitalTwin_InterfaceClient_Destroy returns, it is safe to assume
    // no more callbacks shall arrive for this interface and it is OK to free
    // resources callbacks otherwise may have needed.
    // This is a no-op in this simple sample.
}

