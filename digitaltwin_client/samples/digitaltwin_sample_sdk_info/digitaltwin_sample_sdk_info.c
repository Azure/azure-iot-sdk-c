// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Implements a sample DigitalTwin interface that reports the SDK information
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "digitaltwin_client_version.h"
#include "digitaltwin_sample_sdk_info.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

// DigitalTwin interface id SDK Information interface ID
static const char DigitalTwinSampleSdkInfo_InterfaceId[] = "dtmi:azure:Client:SDKInformation;1";

// DigitalTwin component name from service perspective.
static const char DigitalTwinSampleSdkInfo_ComponentName[] = "sdkInformation";

//
//  Property names and data for DigitalTwin read-only properties for this interface.
//
static const char DT_SdkLanguage_Property[] = "language";
static const char DT_SdkVersion_Property[] = "version";
static const char DT_SdkVendor_Property[] = "vendor";
static const char DT_SdkLanguage[] = "\"C\"";
static const char DT_SdkVendor[] = "\"Microsoft\"";

//
// Application state associated with the particular interface.  It contains 
// the DIGITALTWIN_INTERFACE_CLIENT_HANDLE used for responses reporting properties.
//
typedef struct DIGITALTWIN_SAMPLE_SDK_INFO_TAG
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceClientHandle;
} DIGITALTWIN_SAMPLE_SDK_INFO_STATE;


// State for interface.  For simplicity we set this as a global and set during DigitalTwin_InterfaceClient_Create, but  
// callbacks of this interface don't reference it directly but instead use userContextCallback passed to them.
static DIGITALTWIN_SAMPLE_SDK_INFO_STATE digitaltwinSample_SdkInfoState;


// DigitalTwinSampleSdkInfo_PropertyCallback is invoked when a property is updated (or failed) going to server.
// In this sample, we route ALL property callbacks to this function and just have the userContextCallback set
// to the propertyName.  Product code will potentially have context stored in this userContextCallback.
static void DigitalTwinSampleSdkInfo_PropertyCallback(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    if (dtReportedStatus == DIGITALTWIN_CLIENT_OK)
    {
        LogInfo("SDK_INFO: Property callback property=<%s> succeeds", (const char*)userContextCallback);
    }
    else
    {
        LogError("SDK_INFO: Property callback property=<%s> fails, error=<%s>", (const char*)userContextCallback, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtReportedStatus));
    }
}


//
// DigitalTwinSampleSdkInfo_ReportPropertyAsync is a helper function to report a SdkInfo's properties.
// It invokes underlying DigitalTwin API for reporting properties and sets up its callback on completion.
//
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleSdkInfo_ReportPropertyAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle, const char* propertyName, const char* propertyData)
{
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;

    result = DigitalTwin_InterfaceClient_ReportPropertyAsync(interfaceHandle, propertyName,
        (const unsigned char*)propertyData, strlen(propertyData), NULL,
        DigitalTwinSampleSdkInfo_PropertyCallback, (void*)propertyName);

    if (result != DIGITALTWIN_CLIENT_OK)
    {
        LogError("SDK_INFO: Reporting property=<%s> failed, error=<%s>", propertyName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
    }
    else
    {
        LogInfo("SDK_INFO: Queued async report read only property for %s", propertyName);
    }

    return result;
}


// DigitalTwinSampleSdkInfo_ReportSdkInfoAsync() reports all SdkInfo properties to the service
static DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleSdkInfo_ReportSdkInfoAsync(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    DIGITALTWIN_CLIENT_RESULT result;

    char versionString[32];
    sprintf(versionString, "\"%s\"", DigitalTwin_Client_GetVersionString());

    // NOTE: Future versions of SDK will support ability to send mulitiple properties in a single
    // send.  For now, one at a time is sufficient albeit less effecient.

    if (((result = DigitalTwinSampleSdkInfo_ReportPropertyAsync(interfaceHandle, DT_SdkLanguage_Property, DT_SdkLanguage)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleSdkInfo_ReportPropertyAsync(interfaceHandle, DT_SdkVersion_Property, versionString)) != DIGITALTWIN_CLIENT_OK) ||
        ((result = DigitalTwinSampleSdkInfo_ReportPropertyAsync(interfaceHandle, DT_SdkVendor_Property, DT_SdkVendor)) != DIGITALTWIN_CLIENT_OK))
    {
        LogError("SDK_INFO: Reporting properties failed.");
    }
    else
    {
        LogInfo("SDK_INFO: Queing of all properties to be reported has succeeded");
    }

    return result;
}


// DigitalTwinSampleSdkInfo_InterfaceRegisteredCallback is invoked when this interface
// is successfully or unsuccessfully registered with the service, and also when the interface is deleted.
static void DigitalTwinSampleSdkInfo_InterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    DIGITALTWIN_SAMPLE_SDK_INFO_STATE *sdkInfoState = (DIGITALTWIN_SAMPLE_SDK_INFO_STATE*)userInterfaceContext;
    if (dtInterfaceStatus == DIGITALTWIN_CLIENT_OK)
    {
        // Once the interface is registered, send our reported properties to the service.  
        // It *IS* safe to invoke most DigitalTwin API calls from a callback thread like this, though it 
        // is NOT safe to create/destroy/register interfaces now.
        LogInfo("SDK_INFO: Interface successfully registered.");
        DigitalTwinSampleSdkInfo_ReportSdkInfoAsync(sdkInfoState->interfaceClientHandle);
    }
    else if (dtInterfaceStatus == DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING)
    {
        // Once an interface is marked as unregistered, it cannot be used for any DigitalTwin SDK calls.
        LogInfo("SDK_INFO: Interface received unregistering callback.");
    }
    else 
    {
        LogError("SDK_INFO: Interface received failed, status=<%s>.", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus));
    }
}


//
// DigitalTwinSampleSdkInfo_CreateInterface is the initial entry point into the DigitalTwin Sample Sdk Information Interface.
// It simply creates a DIGITALTWIN_INTERFACE_CLIENT_HANDLE that is mapped to the sdk information component name.
// This call is synchronous, as simply creating an interface only performs initial allocations.
//
// NOTE: The actual registration of this interface is left to the caller, which may register 
// multiple interfaces on one DIGITALTWIN_DEVICE_CLIENT_HANDLE.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DigitalTwinSampleSdkInfo_CreateInterface(void)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;

    memset(&digitaltwinSample_SdkInfoState, 0, sizeof(digitaltwinSample_SdkInfoState));
    
    if ((result =  DigitalTwin_InterfaceClient_Create(DigitalTwinSampleSdkInfo_InterfaceId, DigitalTwinSampleSdkInfo_ComponentName, DigitalTwinSampleSdkInfo_InterfaceRegisteredCallback, (void*)&digitaltwinSample_SdkInfoState, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("SDK_INFO: Unable to allocate interface client handle for interfaceId=<%s>, componentName=<%s>, error=<%s>", DigitalTwinSampleSdkInfo_InterfaceId, DigitalTwinSampleSdkInfo_ComponentName, MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        interfaceHandle = NULL;
    }
    else
    {
        LogInfo("SDK_INFO: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  interfaceId=<%s>, componentName=<%s>, handle=<%p>", DigitalTwinSampleSdkInfo_InterfaceId, DigitalTwinSampleSdkInfo_ComponentName, interfaceHandle);
        digitaltwinSample_SdkInfoState.interfaceClientHandle = interfaceHandle;
    }

    return interfaceHandle;
}


//
// DigitalTwinSampleSdkInformation_Close is invoked when the sample device is shutting down.
//
void DigitalTwinSampleSdkInfo_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    // On shutdown, in general the first call made should be to DigitalTwin_InterfaceClient_Destroy.
    // This will block if there are any active callbacks in this interface, and then
    // mark the underlying handle such that no future callbacks shall come to it.
    DigitalTwin_InterfaceClient_Destroy(interfaceHandle);

    // After DigitalTwin_InterfaceClient_Destroy returns, it is safe to assume
    // no more callbacks shall arrive for this interface and it is OK to free
    // resources callbacks otherwise may have needed.
}

