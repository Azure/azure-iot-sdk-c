// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <azure_c_shared_utility/xlogging.h>

#include "dt_e2e_commands.h"
#include "dt_e2e_util.h"


//*****************************************************************************
// Logic to tie test commands into DigitalTwin framework and the test's main()
//*****************************************************************************
// Interface name for properties testing
static const char DT_E2E_Properties_InterfaceId[] = "dtmi:azureiot:testinterfaces:cdevicesdk:properties;1";
static const char DT_E2E_Properties_InterfaceName[] = "testProperties";

// Command names that the service SDK invokes to invoke tests on this interface
static const char* DT_E2E_properties_commandNames[] = { 
    "SendReportedProperty1",
    "VerifyPropertiesAcknowledged"
};

static void DT_E2E_SendReportedProperty1(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userInterfaceContext);
static void DT_E2E_VerifyPropertiesAcknowledged(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userInterfaceContext);

static void DT_E2E_ProcessUpdatedProperty1(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext);

#define DT_E2E_PROCESS_UPDATED_PROPERTY1_NAME "ProcessUpdatedProperty1"
#define DT_E2E_PROPERTY_VALUE_SET_1 "\"ValueOfPropertyBeingSet_1\""

static const char* DT_E2E_properties_property_names[] = { 
    DT_E2E_PROCESS_UPDATED_PROPERTY1_NAME
};
    
static const DIGITALTWIN_PROPERTY_UPDATE_CALLBACK DT_E2E_properties_Callbacks[] = { 
    DT_E2E_ProcessUpdatedProperty1,
};

// Functions that implement tests service invokes for this interface; maps to DT_E2E_properties_commandNames.
static const DIGITALTWIN_COMMAND_EXECUTE_CALLBACK DT_E2E_properties_commandCallbacks[] = { 
    DT_E2E_SendReportedProperty1,
    DT_E2E_VerifyPropertiesAcknowledged
};

static const int DT_E2E_number_PropertiesCommands = sizeof(DT_E2E_properties_commandNames) / sizeof(DT_E2E_properties_commandNames[0]);

//
// DT_E2E_PropertiesCommandCallbackProcess is invoked by test's main() to setup the DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this test.
// Even though properties obviously have their own channel for interacting with a device, this command callback is used as a back-channel
// for additional control and querying of state.
// 
static void DT_E2E_PropertiesCommandCallbackProcess(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtCommandRequest, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtCommandResponse, void* userInterfaceContext)
{
    DT_E2E_Util_InvokeCommand(DT_E2E_properties_commandNames, DT_E2E_properties_commandCallbacks, DT_E2E_number_PropertiesCommands, dtCommandRequest, dtCommandResponse, userInterfaceContext);
}

static const int DT_E2E_number_PropertiesUpdate = sizeof(DT_E2E_properties_property_names)  / sizeof(DT_E2E_properties_property_names[0]);

//
// DT_E2E_ProcessUpdatedProperty receives all property update requests from the server and routes to the appropriate test function.
//
static void DT_E2E_ProcessUpdatedProperty(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    int i;

    for (i = 0; i < DT_E2E_number_PropertiesUpdate; i++)
    {
        if (strcmp(DT_E2E_properties_property_names[i], dtClientPropertyUpdate->propertyName) == 0)
        {
            DT_E2E_properties_Callbacks[i](dtClientPropertyUpdate, userInterfaceContext);
            break;
        }
    }

    if (i == DT_E2E_number_PropertiesUpdate)
    {
        LogError("Property <%s> is not supported by this test", dtClientPropertyUpdate->propertyName);
        DT_E2E_Util_Fatal_Error_Occurred();
    }
}

// DT_E2E_PROPERTIES_TEST_CONTEXT specifies the context passed to device method callback routines for property testing.
typedef struct DT_E2E_PROPERTIES_TEST_CONTEXT_TAG
{
    // Interface handle associated with this callback
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    // Number of times the property from DT_E2E_SendReportedProperty1 has been reported
    unsigned int numTimesProp1Reported;
    // Number of times the property from DT_E2E_SendReportedProperty1 has been acknowledged
    unsigned int numTimesProp1Acknowledged;
} DT_E2E_PROPERTIES_TEST_CONTEXT;

DT_E2E_PROPERTIES_TEST_CONTEXT DT_E2E_PropertiesTest_Context;

//
// DT_E2E_Properties_CreateInterface is invoked by test's main() to setup the DIGITALTWIN_INTERFACE_CLIENT_HANDLE for this test.
//
DIGITALTWIN_INTERFACE_CLIENT_HANDLE DT_E2E_Properties_CreateInterface(void)
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle;
    DIGITALTWIN_CLIENT_RESULT result;

    memset(&DT_E2E_PropertiesTest_Context, 0, sizeof(DT_E2E_PropertiesTest_Context));
   
    if ((result =  DigitalTwin_InterfaceClient_Create(DT_E2E_Properties_InterfaceId, DT_E2E_Properties_InterfaceName, NULL, &DT_E2E_PropertiesTest_Context, &interfaceHandle)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_PROPERTIES_INTERFACE: Unable to allocate interface client handle for <%s>, error=<%d>", DT_E2E_Properties_InterfaceName, result);
        interfaceHandle = NULL;
    }
    else if ((result =  DigitalTwin_InterfaceClient_SetCommandsCallback(interfaceHandle, DT_E2E_PropertiesCommandCallbackProcess, &DT_E2E_PropertiesTest_Context)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_PROPERTIES_INTERFACE: DigitalTwin_InterfaceClient_SetCommandsCallbacks failed, error=<%d>", result);
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
        interfaceHandle = NULL;
    }
    else if ((result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(interfaceHandle, DT_E2E_ProcessUpdatedProperty, &DT_E2E_PropertiesTest_Context)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("ENVIRONMENTAL_SENSOR_INTERFACE: DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallbacks failed. error=<%d>", result);
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
        interfaceHandle = NULL;
    }    
    else
    {
        DT_E2E_PropertiesTest_Context.interfaceHandle = interfaceHandle;
        LogInfo("TEST_PROPERTIES_INTERFACE: Created DIGITALTWIN_INTERFACE_CLIENT_HANDLE.  InterfaceName=<%s>, handle=<%p>", DT_E2E_Properties_InterfaceName, interfaceHandle);
    }

    return interfaceHandle;
}

//
// DT_E2E_Properties_DestroyInterface frees interface handle created at test initialization
//
void DT_E2E_Properties_DestroyInterface(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle)
{
    if (interfaceHandle != NULL)
    {
        DigitalTwin_InterfaceClient_Destroy(interfaceHandle);
    }
}


//*****************************************************************************
// Helpers
//*****************************************************************************

// Values to send service when a test fails on device side
static const char* DT_E2E_PropertiesSucceededMessage = "\"Successfully processed property test request.\"";
static const int DT_E2E_PropertiesSucceededStatus = 200;

static const char* DT_E2E_PropertiesFailedMessage = "\"Test failed on device side.\"";
static const int DT_E2E_PropertiesFailedStatus = 500;

//
// VerifyExpectedProperty makes sure that the property data from service matches expected for given test case.
//
static int VerifyExpectedProperty(const char* testName, const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, const char* expectedPayload, void* userInterfaceContext)
{
    int result;

    if (userInterfaceContext != &DT_E2E_PropertiesTest_Context)
    {
        LogError("TEST_PROPERTY_INTERFACE:: Test <%s> fails.  Requested payload user context is <%p> but expected is <%p>",
                 testName, userInterfaceContext, &DT_E2E_PropertiesTest_Context);
        DT_E2E_Util_Fatal_Error_Occurred();
        result = MU_FAILURE;
    }
    else if (strncmp((const char*)dtClientPropertyUpdate->propertyDesired, expectedPayload, strlen(expectedPayload)) != 0)
    {
        LogError("TEST_PROPERTY_INTERFACE:: Test <%s> fails.  Requested payload is <%s> but expected is <%s>", 
                 testName, (const char*)dtClientPropertyUpdate->propertyDesired, expectedPayload);
        DT_E2E_Util_Fatal_Error_Occurred();
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

//*****************************************************************************
// Actual test routines and data
//*****************************************************************************

//
// DT_E2E_SendReportedProperty_Callback is invoked when the property reported by test DT_E2E_SendReportedProperty1 is ack'd, either 
// successfully or else on failure.
// 

static const char* DT_E2E_SendProperty1_PropertyName = "SendProperty1_Name";
static const char* DT_E2E_SendProperty1_PropertyData = "\"SendProperty1_Data\"";

static void DT_E2E_SendReportedProperty_Callback(DIGITALTWIN_CLIENT_RESULT dtReportedStatus, void* userContextCallback)
{
    if (dtReportedStatus != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_PROPERTIES_INTERFACE: DT_E2E_SendReportedProperty_Callback callback received error=<%d>", dtReportedStatus);
        DT_E2E_Util_Fatal_Error_Occurred();
    }
    else
    {
        DT_E2E_PROPERTIES_TEST_CONTEXT* propertiesTestContext = (DT_E2E_PROPERTIES_TEST_CONTEXT*)userContextCallback;
        propertiesTestContext->numTimesProp1Acknowledged++;
        LogInfo("TEST_PROPERTIES_INTERFACE: DT_E2E_SendReportedProperty_Callback received successful ack.  <%d> messages successfully ACK'd so far", propertiesTestContext->numTimesProp1Acknowledged);
    }
}

//
// DT_E2E_SendReportedProperty1 will trigger a property of DT_E2E_SendProperty1_PropertyName to be reported to the service.
//
static void DT_E2E_SendReportedProperty1(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userInterfaceContext)
{
    (void)dtClientCommandContext;
    DIGITALTWIN_CLIENT_RESULT result;
    DT_E2E_PROPERTIES_TEST_CONTEXT* propertiesTestContext = (DT_E2E_PROPERTIES_TEST_CONTEXT*)userInterfaceContext;

    if ((result = DigitalTwin_InterfaceClient_ReportPropertyAsync(propertiesTestContext->interfaceHandle, DT_E2E_SendProperty1_PropertyName, (const unsigned char*)DT_E2E_SendProperty1_PropertyData, strlen(DT_E2E_SendProperty1_PropertyData), NULL, DT_E2E_SendReportedProperty_Callback, propertiesTestContext)) != DIGITALTWIN_CLIENT_OK)
    {
        LogError("TEST_PROPERTIES_INTERFACE: DigitalTwin_InterfaceClient_ReportPropertyAsync failed, error=<%d>", result);
        DT_E2E_Util_Fatal_Error_Occurred();
        SetCommandResponse(dtClientCommandResponseContext, DT_E2E_PropertiesFailedMessage, DT_E2E_PropertiesFailedStatus);
    }
    else
    {
        propertiesTestContext->numTimesProp1Reported++;
        LogInfo("DigitalTwin_InterfaceClient_ReportPropertyAsync succeeded.  Number of times Property1 reported so far = <%d>", propertiesTestContext->numTimesProp1Reported);
        SetCommandResponse(dtClientCommandResponseContext, DT_E2E_PropertiesSucceededMessage, DT_E2E_PropertiesSucceededStatus);
    }
}

//
// DT_E2E_VerifyPropertiesAcknowledged makes sure that for each property reported from device=> service, an acknowledegment has been received.
// Because properties may take time to be ack'd after initial send, the initiator may need to call this function multiple times as ack's come in.
//

static const char* DT_E2E_VerifyReportedPropertiesAcknowledgedMessage = "\"All reported properties from device have been acknowledged\"";
static const char* DT_E2E_NotAllReportedPropertiesAcknowledgedMessage = "\"Not all reported properties from device have been acknowledged\"";
// Use a 202 here to indicate that this status is not necessarily fatal (remote caller will need to poll again)
static const int DT_E2E_NotAllReportedPropertiesAcknowledgedStatus = 202;

static void DT_E2E_VerifyPropertiesAcknowledged(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userInterfaceContext)
{
    (void)dtClientCommandContext;
    DT_E2E_PROPERTIES_TEST_CONTEXT* propertiesTestContext = (DT_E2E_PROPERTIES_TEST_CONTEXT*)userInterfaceContext;

    if (propertiesTestContext->numTimesProp1Reported != propertiesTestContext->numTimesProp1Acknowledged)
    {
        // The use of LogInfo() is intentional; the server will need to call this function multiple times to poll
        // for all messages being reported.  While technically this is a verification failure, given how test fits into e2e 
        // framework this state is not considered fatal.
        LogInfo("TEST_PROPERTY_INTERFACE: Number of reported messages <%d> is NOT identical to number of ack'd messages <%d>", propertiesTestContext->numTimesProp1Reported, propertiesTestContext->numTimesProp1Acknowledged);
        SetCommandResponse(dtClientCommandResponseContext, DT_E2E_NotAllReportedPropertiesAcknowledgedMessage, DT_E2E_NotAllReportedPropertiesAcknowledgedStatus);
    }
    else
    {
        LogInfo("TEST_PROPERTY_INTERFACE: Number of times prop1 reported <%d> is identical to number of ack'd messages", propertiesTestContext->numTimesProp1Reported);
        SetCommandResponse(dtClientCommandResponseContext, DT_E2E_VerifyReportedPropertiesAcknowledgedMessage, DT_E2E_PropertiesSucceededStatus);
    }
}

//
// DT_E2E_ProcessUpdatedProperty1 receives a property update for 'DT_E2E_PROCESS_UPDATED_PROPERTY1_NAME', checks payload against expected,
// and invokes DigitalTwin_InterfaceClient_ReportPropertyAsync to set a response.
//
void DT_E2E_ProcessUpdatedProperty1(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    DT_E2E_PROPERTIES_TEST_CONTEXT* propertiesTestContext = (DT_E2E_PROPERTIES_TEST_CONTEXT*)userInterfaceContext;

    if (VerifyExpectedProperty(__FUNCTION__, dtClientPropertyUpdate, DT_E2E_PROPERTY_VALUE_SET_1, userInterfaceContext) == 0)
    {
        DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
        propertyResponse.version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
        propertyResponse.statusCode = 200;
        propertyResponse.responseVersion = dtClientPropertyUpdate->desiredVersion;
        propertyResponse.statusDescription = "Processing Updated Property1";

        DIGITALTWIN_CLIENT_RESULT result;

        result = DigitalTwin_InterfaceClient_ReportPropertyAsync(propertiesTestContext->interfaceHandle, DT_E2E_PROCESS_UPDATED_PROPERTY1_NAME, 
                                                                 (const unsigned char *)DT_E2E_PROPERTY_VALUE_SET_1, strlen(DT_E2E_PROPERTY_VALUE_SET_1), &propertyResponse, NULL, NULL);
        if (result != DIGITALTWIN_CLIENT_OK)
        {
            LogError("DigitalTwin_InterfaceClient_ReportPropertyAsync failed, error = <%d>", result);
            DT_E2E_Util_Fatal_Error_Occurred();
        }
        else
        {   
            LogInfo("Successfully processed incoming property and queued response");
        }
    }
    else
    {
        // Currently there is no mechanism to indicate invalid property in DigitalTwin design.  The test will fail in this case, though, as
        // its expected data set via DigitalTwin_InterfaceClient_ReportPropertyAsync won't be set in this path.
        ;
    }
}

