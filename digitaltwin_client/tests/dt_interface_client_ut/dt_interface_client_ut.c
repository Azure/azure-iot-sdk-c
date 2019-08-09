// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include <ctype.h>


#include "testrunnerswitcher.h"

#include "real_parson.h"
#include "../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.h"


#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}


#define ENABLE_MOCKS
#include "internal/dt_lock_thread_binding.h"
#include "internal/dt_client_core.h"
#include "internal/dt_raw_interface.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uuid.h"
#include "iothub_message.h"
#include "parson.h"
#include "digitaltwin_client_version.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, const char*, json_object_dotget_string, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, JSON_Array*, json_array_get_array, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_array_clear, JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Status, json_object_clear, JSON_Object*, object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value *, value);
MOCKABLE_FUNCTION(, char *, json_serialize_to_string_pretty, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_dotset_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object *, json_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object *, object, const char *, name, JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Object *, json_object_dotget_object, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, size_t, json_object_get_count, const JSON_Object *, object);
MOCKABLE_FUNCTION(, JSON_Value *, json_object_get_value_at, const JSON_Object *, object, size_t, index);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_null, JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object *, object, const char *, name); 
MOCKABLE_FUNCTION(, double, json_object_dotget_number, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, const char *, json_object_get_name, const JSON_Object *, object, size_t, index);
MOCKABLE_FUNCTION(, JSON_Value *, json_object_dotget_value, const JSON_Object *, object, const char *, name);


MOCKABLE_FUNCTION(, LOCK_HANDLE, testBindingLockInit);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingUnlock, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, testBindingLockDeinit, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, void, testBindingThreadSleep, unsigned int, milliseconds);

MOCKABLE_FUNCTION(, void, testInterfaceRegisteredCallback, DIGITALTWIN_CLIENT_RESULT, dtInterfaceStatus, void*, userInterfaceContext);


#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

#include "dt_interface_private.h"
#include "digitaltwin_device_client.h"
#include "../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.h"
#include "real_strings.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, testDTClientTelemetryConfirmationCallback, DIGITALTWIN_CLIENT_RESULT, dtTelemetryStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, testDTClientReportedPropertyCallback, DIGITALTWIN_CLIENT_RESULT, dtReportedStatus, void*, userContextCallback);
MOCKABLE_FUNCTION(, void, testDTClientCommandCallback, const DIGITALTWIN_CLIENT_COMMAND_REQUEST*, dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE*, dtClientCommandResponseContext, void*, userInterfaceContext);
MOCKABLE_FUNCTION(, void, testDTClientPropertyUpdate, const DIGITALTWIN_CLIENT_PROPERTY_UPDATE*, dtClientPropertyUpdate, void*, userInterfaceContext);
#undef ENABLE_MOCKS

static void* testBindingIotHubBindingLockHandle = (void*)0x1221;

// TODO: In general code needs a review to add additional forced allocations.
LOCK_HANDLE impl_testBindingLockInit(void)
{
    return (LOCK_HANDLE)my_gballoc_malloc(1);
}

LOCK_RESULT impl_testBindingLockDeinit(LOCK_HANDLE bindingLock)
{
    my_gballoc_free((void*)bindingLock);
    return LOCK_OK;
}

static void impl_testInterfaceRegisteredCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus, void* userInterfaceContext)
{
    (void)dtInterfaceStatus;
    (void)userInterfaceContext;
}

static const DT_CLIENT_CORE_HANDLE testDTClientCoreHandle = (DT_CLIENT_CORE_HANDLE)0x1230;

#define DT_TEST_INTERFACE_NOT_REGISTERED_NAME "thisInterfaceIsNotRegisteredWithHandle"

TEST_DEFINE_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_RESULT_VALUES);

#define DT_TEST_MESSAGE_DATA_VALUE "MessageData"
static const char* dtTestTelemetryName = "TestDTTelemetryName";
static const char* dtTestMessageData = DT_TEST_MESSAGE_DATA_VALUE;
static const size_t dtTestMessageDataLen = sizeof(DT_TEST_MESSAGE_DATA_VALUE) - 1;

static void* testDTInterfaceCallbackContext = (void*)0x1236;

#define DT_TEST_COMMAND_NAME   "DTTestCallbackCommand"

static const char DT_TEST_ASYNC_UPDATING_COMMAND_NAME[] = "TestAsyncUpdatingCommand";

typedef enum DT_TEST_EXPECTED_COMMAND_CALLBACK_TAG
{
    DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK,
    DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE,
    DT_TEST_EXPECTED_INTERFACE_NO_MATCH,
} DT_TEST_EXPECTED_COMMAND_CALLBACK;

typedef enum DT_TEST_PROPERTY_UPDATE_TEST_TAG
{
    // Reported properties are expected with this update
    DT_TEST_PROPERTY_UPDATE_TEST_REPORTED_EXPECTED,
    // No reported properties are expected with this update
    DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED,
    // This test is running as part of a failure test; skip parameter validation
    // because reported may or may not be present based on which call had failed
    DT_TEST_PROPERTY_UPDATE_TEST_FAILURE_TEST
} DT_TEST_PROPERTY_UPDATE_TEST;


#define DT_TEST_MAX_PROPERTIES 3


// DT_TEST_EXPECTED_PROPERTY_STATUS is used to track property test status.
// It is initialized prior to a property update test run with the
// expected order of property names (e.g. "DTTestProperty1", "DTTestProperty3").
// The mocked property update processor function then compares what it's
// expected to receive at any given state with the data actually arriving.
typedef struct DT_TEST_EXPECTED_PROPERTY_STATUS_TAG
{
    const char* expectedPropertyName[DT_TEST_MAX_PROPERTIES];
    const char* expectedDesiredData[DT_TEST_MAX_PROPERTIES];
    const char* expectedReportedData[DT_TEST_MAX_PROPERTIES];
    int currentProperty;
    int maxProperty;
} DT_TEST_EXPECTED_PROPERTY_STATUS;

DT_TEST_EXPECTED_PROPERTY_STATUS dtTestExpectedPropertyStatus;


#define DT_TEST_PROPERTY_NAME1   "DTTestProperty1"
#define DT_TEST_PROPERTY_NAME2   "DTTestProperty2"
#define DT_TEST_PROPERTY_NAME3   "DTTestProperty3"

// Note that strings have the " " around value passed in so it is treated as a json value
#define DT_TEST_PROPERTY_DESIRED_VALUE1   "\"Prop1-Value\""
#define DT_TEST_PROPERTY_DESIRED_VALUE2   "\"Prop2-Value\""
#define DT_TEST_PROPERTY_DESIRED_VALUE3   "{\"ComplexValue\":1234}"

#define DT_TEST_PROPERTY_REPORTED_VALUE1   "\"PreviouslyReported1\""
#define DT_TEST_PROPERTY_REPORTED_VALUE2   "\"PreviouslyReported2\""
#define DT_TEST_PROPERTY_REPORTED_VALUE3   "{\"ComplexValue\":0}"

static const int DT_TEST_PROPERTY_VERSION = 12;
static DT_TEST_PROPERTY_UPDATE_TEST dtTestPropertyUpdateTest;

static const char* dtTestOneProperty[1] = { DT_TEST_PROPERTY_NAME1 };
static const char* dtTestTwoProperties[2] = { DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_NAME2  };
static const char* dtTestThreeProperties[3] = { DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_NAME2, DT_TEST_PROPERTY_NAME3 };

// Builds up DT-ified command by concatenating interfaceInstanceName, separator, and commandName into a string.
#define DT_TEST_BUILD_COMMAND_NAME(interfaceInstanceName, commandName) "$iotin:" interfaceInstanceName "*" commandName

#define DT_TEST_PAYLOAD_PASSED_TO_CALLBACK "{\"foo\":1234}"
static const size_t dtTestPayloadPassedToCallbackLen = sizeof(DT_TEST_PAYLOAD_PASSED_TO_CALLBACK) - 1;

#define DT_TEST_COMMAND_REQUEST_ID "\"a5973dcb-8bb8-44ff-817a-6907d2c1492c\""


static const char dtTestCommandPayload[] =  "{\"commandRequest\": {\"value\":" DT_TEST_PAYLOAD_PASSED_TO_CALLBACK ", \"requestId\":" DT_TEST_COMMAND_REQUEST_ID  "}}";
static const size_t dtTestCommandPayloadLen = sizeof(dtTestCommandPayload) - 1;

static char dtTestCommandResponse1[] = "Payload of response for command1";
static const size_t dtTestCommandResponseLen1 = sizeof(dtTestCommandResponse1) - 1;
static const int dtTestCommandResponseStatus1 = 1;
static char dtTestCommandResponse2[] = "Payload of response for command2";
static const size_t dtTestCommandResponseLen2 = sizeof(dtTestCommandResponse2) - 1;
static const int dtTestCommandResponseStatus2 = 2;
static char dtTestCommandResponse3[] = "Payload of response for command3";
static const size_t dtTestCommandResponseLen3 = sizeof(dtTestCommandResponse3) - 1;
static const int dtTestCommandResponseStatus3 = 3;
static char dtTestJsonNull[] = "null";


static const char dtTestAsyncCommandRequestId[] = DT_TEST_COMMAND_REQUEST_ID;

#define DT_TEST_ASYNC_COMMAND_RESPONSE_1 "Payload of response for async command1"
static char dtTestAsyncCommandResponse1[] = DT_TEST_ASYNC_COMMAND_RESPONSE_1;
static const size_t dtTestAsyncCommandResponseLen1 = sizeof(dtTestAsyncCommandResponse1) - 1;
static const int dtTestAsyncCommandResponseStatus1 = DIGITALTWIN_ASYNC_STATUS_CODE_PENDING;
static const char dtTestAsyncCommandResponseExpected1[] = "{\"payload\":" DT_TEST_ASYNC_COMMAND_RESPONSE_1 "," "\"requestId\":" DT_TEST_COMMAND_REQUEST_ID "}";

#define DT_TEST_ASYNC_COMMAND_RESPONSE_2 "Payload of response for async command2"
static char dtTestAsyncCommandResponse2[] = DT_TEST_ASYNC_COMMAND_RESPONSE_2;
static const size_t dtTestAsyncCommandResponseLen2 = sizeof(dtTestAsyncCommandResponse2) - 1;
static const int dtTestAsyncCommandResponseStatus2 = 5;
static const char dtTestAsyncCommandResponseExpected2[] = "{\"payload\":" DT_TEST_ASYNC_COMMAND_RESPONSE_2 "," "\"requestId\":" DT_TEST_COMMAND_REQUEST_ID "}";

#define DT_TEST_ASYNC_COMMAND_RESPONSE_3 "Payload of response for async command3"
static char dtTestAsyncCommandResponse3[] = DT_TEST_ASYNC_COMMAND_RESPONSE_3;
static const size_t dtTestAsyncCommandResponseLen3 = sizeof(dtTestAsyncCommandResponse3) - 1;
static const int dtTestAsyncCommandResponseStatus3 = DIGITALTWIN_ASYNC_STATUS_CODE_PENDING;
static const char dtTestAsyncCommandResponseExpected3[] = "{\"payload\":" DT_TEST_ASYNC_COMMAND_RESPONSE_3 "," "\"requestId\":" DT_TEST_COMMAND_REQUEST_ID "}";

#define DT_TEST_PROPERTY_1_CONTENT_NO_BRACES "\"DTTestProperty1\": { \"value\":  \"Prop1-Value\" }"
#define DT_TEST_PROPERTY_2_CONTENT_NO_BRACES "\"DTTestProperty2\": { \"value\":  \"Prop2-Value\" }"
#define DT_TEST_PROPERTY_3_CONTENT_NO_BRACES "\"DTTestProperty3\": { \"value\":  { \"ComplexValue\": 1234 } } "

#define DT_TEST_PROPERTY_1_CONTENT "{" DT_TEST_PROPERTY_1_CONTENT_NO_BRACES "}"
#define DT_TEST_PROPERTY_2_CONTENT "{" DT_TEST_PROPERTY_2_CONTENT_NO_BRACES "}"
#define DT_TEST_PROPERTY_3_CONTENT "{" DT_TEST_PROPERTY_3_CONTENT_NO_BRACES "}"

#define DT_TEST_PROPERTY_1_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES  "\"DTTestProperty1\": { \"value\":  \"PreviouslyReported1\"  } "
#define DT_TEST_PROPERTY_2_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "\"DTTestProperty2\": { \"value\":  \"PreviouslyReported2\"  }"
#define DT_TEST_PROPERTY_3_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "\"DTTestProperty3\": { \"value\":  { \"ComplexValue\": 0 } }"

#define DT_TEST_PROPERTY_1_PREVIOUSLY_REPORTED_CONTENT  "{" DT_TEST_PROPERTY_1_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "}"
#define DT_TEST_PROPERTY_2_PREVIOUSLY_REPORTED_CONTENT "{" DT_TEST_PROPERTY_2_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "}"
#define DT_TEST_PROPERTY_3_PREVIOUSLY_REPORTED_CONTENT "{" DT_TEST_PROPERTY_3_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "}"

#define DT_TEST_INTERFACE_ID_1 "urn:testonly:testinterface:1"
#define DT_TEST_INTERFACE_NAME_1 "testonly_testinterface"


static const char* dtTestInterfaceId1 = DT_TEST_INTERFACE_ID_1;
static const char* dtTestInterfaceInstanceName1 = DT_TEST_INTERFACE_NAME_1;


#define DT_TEST_DESIRED_VERSION_JSON ", \"$version\": 12"
#define DT_TEST_TWIN_VERSION_JSON "}, \"$version\": 18"

#define DT_TEST_DESIRED_JSON "{  \"desired\": { "
#define DT_TEST_REPORTED_JSON "}, \"reported\": { "

#define DT_TEST_INTERFACE_NAME_1_OBJECT "\"$iotin:"  DT_TEST_INTERFACE_NAME_1 "\": "

// Property DTTestProperty1 is being updated for this interface, as a full twin.
static const char updatePropertiesPropertyFullTwin1[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_1_CONTENT DT_TEST_DESIRED_VERSION_JSON  DT_TEST_REPORTED_JSON DT_TEST_TWIN_VERSION_JSON "}";
static const size_t updatePropertiesPropertyFullTwin1Len= sizeof(updatePropertiesPropertyFullTwin1) - 1;

// Property DTTestProperty2 is being updated for this interface, as a full twin.
static const char updatePropertiesPropertyFullTwin2[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_2_CONTENT  DT_TEST_DESIRED_VERSION_JSON DT_TEST_REPORTED_JSON DT_TEST_TWIN_VERSION_JSON "}";
static const size_t updatePropertiesPropertyFullTwin2Len= sizeof(updatePropertiesPropertyFullTwin2) - 1;

// Property DTTestProperty3 is being updated for this interface, as a full twin.  Note DTTestProperty3 uses a complex JSON value for additional testing, here & throughout.
static const char updatePropertiesPropertyFullTwin3[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_3_CONTENT  DT_TEST_DESIRED_VERSION_JSON  DT_TEST_REPORTED_JSON DT_TEST_TWIN_VERSION_JSON  "}";
static const size_t updatePropertiesPropertyFullTwin3Len= sizeof(updatePropertiesPropertyFullTwin3) - 1;

// Property DTTestProperty1 is being updated for this interface, as a twin update.
static const char updatePropertiesPropertyUpdateTwin1[] = "{" DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_1_CONTENT DT_TEST_DESIRED_VERSION_JSON "}";
static const size_t updatePropertiesPropertyUpdateTwin1Len= sizeof(updatePropertiesPropertyUpdateTwin1) - 1;

// Property DTTestProperty2 is being updated for this interface, as a twin update.
static const char updatePropertiesPropertyUpdateTwin2[] = "{" DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_2_CONTENT DT_TEST_DESIRED_VERSION_JSON "}";
static const size_t updatePropertiesPropertyUpdateTwin2Len= sizeof(updatePropertiesPropertyUpdateTwin2) - 1;

// Property DTTestProperty3 is being updated for this interface, as a twin update.
static const char updatePropertiesPropertyUpdateTwin3[] = "{" DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_3_CONTENT DT_TEST_DESIRED_VERSION_JSON "}";
static const size_t updatePropertiesPropertyUpdateTwin3Len= sizeof(updatePropertiesPropertyUpdateTwin3) - 1;

// Property DTTestProperty1 is being updated *but for a different interface* than we're testing.  So should be ignored.
static const char updatePropertiesDifferentInterfaceFullTwin[] = "{  \"$iotin:DIFFERENT-INTERFACE\":" DT_TEST_PROPERTY_3_CONTENT DT_TEST_DESIRED_VERSION_JSON "}";
static const size_t updatePropertiesDifferentInterfaceFullTwinLen= sizeof(updatePropertiesDifferentInterfaceFullTwin) - 1;

// Property DTTestProperty1 is being updated for this interface, as a full twin.  There is a totally unrelated interface included which should be ignored.
static const char updatePropertiesPropertyFullTwin1AndRandomInterfaces[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT DT_TEST_PROPERTY_1_CONTENT ", \"$iotin:DIFFERENT-INTERFACE\":" DT_TEST_PROPERTY_2_CONTENT  DT_TEST_DESIRED_VERSION_JSON DT_TEST_REPORTED_JSON DT_TEST_TWIN_VERSION_JSON "}";
static const size_t updatePropertiesPropertyFullTwin1AndRandomInterfacesLen= sizeof(updatePropertiesPropertyFullTwin1AndRandomInterfaces) - 1;

// All desired properties are updated, full twin
static const char updatePropertiesPropertyFullTwinAll[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT "{" DT_TEST_PROPERTY_1_CONTENT_NO_BRACES ","  DT_TEST_PROPERTY_2_CONTENT_NO_BRACES "," DT_TEST_PROPERTY_3_CONTENT_NO_BRACES "}" DT_TEST_DESIRED_VERSION_JSON DT_TEST_REPORTED_JSON DT_TEST_TWIN_VERSION_JSON "} }";
static const size_t updatePropertiesPropertyFullTwinAllLen= sizeof(updatePropertiesPropertyFullTwinAll) - 1;

// All desired and reported properties are updated, full twin
static const char updatePropertiesDesiredAndReportedFullTwinAll[] = DT_TEST_DESIRED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT "{" DT_TEST_PROPERTY_1_CONTENT_NO_BRACES "," DT_TEST_PROPERTY_2_CONTENT_NO_BRACES "," DT_TEST_PROPERTY_3_CONTENT_NO_BRACES "}" DT_TEST_DESIRED_VERSION_JSON  DT_TEST_REPORTED_JSON DT_TEST_INTERFACE_NAME_1_OBJECT "{"  DT_TEST_PROPERTY_1_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "," DT_TEST_PROPERTY_2_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES "," DT_TEST_PROPERTY_3_PREVIOUSLY_REPORTED_CONTENT_NO_BRACES DT_TEST_TWIN_VERSION_JSON "} }";
static const size_t updatePropertiesDesiredAndReportedFullTwinAllLen= sizeof(updatePropertiesDesiredAndReportedFullTwinAll) - 1;

// JSon is not legal
static const char updatePropertiesInvalidJson[] = "{Not-valid-json";
static const size_t updatePropertiesInvalidJsonLen= sizeof(updatePropertiesInvalidJson) - 1;

static const char dtTestExpectedSdkInfo[] = "{\"$iotin:urn_azureiot_Client_SDKInformation\":  { \"language\":{ \"value\":\"C\"},\"version\":{ \"value\":\"0.9.0\"},\"vendor\":{ \"value\":\"Microsoft\"}}}";

// Valid interface Ids
static const char* DT_TEST_Valid_InterfaceIds[] = {
    "urn:goodInterface:1",
    "urn:good:Interface:1",
    "urn:good:Interface:Intf:1",
    "urn:good:_Interface_:Intf:Intf2:1",
    "urn:good_Name_012356789_aA_zZ:3",
    "urn:good:abcdefghijklmnopqrstuvwxyz_:ABCDEFGHIJKLMNOPQRSTUVWXYZ_:0123456789",
    "urn:maximum_length_0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000:1"
};

static const size_t DT_TEST_Valid_InterfaceIdsLen = sizeof(DT_TEST_Valid_InterfaceIds) / sizeof(DT_TEST_Valid_InterfaceIds[0]);

// Valid interface names
static const char* DT_TEST_Valid_InterfaceInstanceNames[] = {
    "goodName",
    "good_Name",
    "good_Name_Name2",
    "good_Name_012356789_aA_zZ",
    "good_abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789",
    "good_maximum_length_000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000_1"
};

static const size_t DT_TEST_Valid_InterfaceInstanceNamesLen = sizeof(DT_TEST_Valid_InterfaceInstanceNames) / sizeof(DT_TEST_Valid_InterfaceInstanceNames[0]);

// Interface IDs that are missing the "urn:" of some flavor
static const char* DT_TEST_MissingUrn_InterfaceIds[] = {
    "",
    "u",
    "ur",
    "urn",
    "someString"
};

static const size_t DT_TEST_MissingUrn_InterfaceIdsLen = sizeof(DT_TEST_MissingUrn_InterfaceIds) / sizeof(DT_TEST_MissingUrn_InterfaceIds[0]);

// Interfaces that are too long
static const char DT_Test_InterfaceIdTooLong[]   = "urn:too_long_length_000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000:10";
static const char DT_Test_InterfaceInstanceNameTooLong[] = "urn:too_long_length_000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000_10";

// If mocked command callback is invoked, whether or not it should set a response or else leave it as NULL
static bool intefaceClient_CommandCallbackSetsData;


// What we expect DigitalTwin SDK to serialize to dtTestAsyncCommandExpectedResponseFromDT when doing async/pending results.
static void dtTestVerifyCommandCallbackRequest(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, const char* expectedCommandName, void* userInterfaceContext)
{
    ASSERT_ARE_EQUAL(int, DIGITALTWIN_CLIENT_COMMAND_REQUEST_VERSION_1, dtClientCommandContext->version, "Client command version doesn't match");
    ASSERT_ARE_EQUAL(char_ptr, expectedCommandName, dtClientCommandContext->commandName);
    ASSERT_ARE_EQUAL(char_ptr, DT_TEST_PAYLOAD_PASSED_TO_CALLBACK, (const char*)dtClientCommandContext->requestData);
    ASSERT_ARE_EQUAL(int, (int)dtTestPayloadPassedToCallbackLen, (int)dtClientCommandContext->requestDataLen);
    ASSERT_ARE_EQUAL(void_ptr, testDTInterfaceCallbackContext, userInterfaceContext, "User callback context does not match expected");
}

static void test_Impl_DT_CommandCallback(const DIGITALTWIN_CLIENT_COMMAND_REQUEST* dtClientCommandContext, DIGITALTWIN_CLIENT_COMMAND_RESPONSE* dtClientCommandResponseContext, void* userInterfaceContext)
{
    dtTestVerifyCommandCallbackRequest(dtClientCommandContext, DT_TEST_COMMAND_NAME, userInterfaceContext);
    dtClientCommandResponseContext->version = DIGITALTWIN_CLIENT_COMMAND_RESPONSE_VERSION_1;
    dtClientCommandResponseContext->status = dtTestCommandResponseStatus1;

    if (intefaceClient_CommandCallbackSetsData == true)
    {
        ASSERT_ARE_EQUAL(int, 0, real_mallocAndStrcpy_s((char**)&dtClientCommandResponseContext->responseData, dtTestCommandResponse1));
        dtClientCommandResponseContext->responseDataLen = dtTestCommandResponseLen1;
    }
    else
    {
        dtClientCommandResponseContext->responseData = NULL;
        dtClientCommandResponseContext->responseDataLen = 0;
    }
}

static void test_Impl_DT_PropertyUpdate(const DIGITALTWIN_CLIENT_PROPERTY_UPDATE* dtClientPropertyUpdate, void* userInterfaceContext)
{
    if (dtTestPropertyUpdateTest != DT_TEST_PROPERTY_UPDATE_TEST_FAILURE_TEST)
    {
        const int currentProperty = dtTestExpectedPropertyStatus.currentProperty;
        ASSERT_IS_TRUE(currentProperty < dtTestExpectedPropertyStatus.maxProperty,
                       "Only expected %d property callbacks but received more", dtTestExpectedPropertyStatus.maxProperty);

        const char* expectedPropertyName = dtTestExpectedPropertyStatus.expectedPropertyName[currentProperty];
        const char* expectedDesiredPropertyData = dtTestExpectedPropertyStatus.expectedDesiredData[currentProperty];

        ASSERT_ARE_EQUAL(int, 0, strcmp(expectedPropertyName, (char*)dtClientPropertyUpdate->propertyName),
                         "Expected property name <%s> does not match actual name <%s>", expectedPropertyName, (char*)dtClientPropertyUpdate->propertyDesired);
        ASSERT_ARE_EQUAL(size_t, strlen(expectedDesiredPropertyData), dtClientPropertyUpdate->propertyDesiredLen,
                         "Expected desired property length <%d> does not match actual <%d>", strlen(expectedDesiredPropertyData), dtClientPropertyUpdate->propertyDesiredLen);
        ASSERT_ARE_EQUAL(int, 0, strncmp(expectedDesiredPropertyData, (char*)dtClientPropertyUpdate->propertyDesired, dtClientPropertyUpdate->propertyDesiredLen),
                         "Expected desired property data <%s> does not match actual <%.*s>", expectedPropertyName, (int)dtClientPropertyUpdate->propertyDesiredLen, (char*)dtClientPropertyUpdate->propertyDesired);
        ASSERT_ARE_EQUAL(void_ptr, testDTInterfaceCallbackContext, userInterfaceContext, 
                         "User callback context does not match expected");
        ASSERT_ARE_EQUAL(int, DT_TEST_PROPERTY_VERSION, dtClientPropertyUpdate->desiredVersion, 
                         "Desired version does not match expected");

        if (dtTestPropertyUpdateTest == DT_TEST_PROPERTY_UPDATE_TEST_REPORTED_EXPECTED)
        {
            const char* expectedReportedPropertyData = dtTestExpectedPropertyStatus.expectedReportedData[currentProperty];
            ASSERT_IS_NOT_NULL(dtClientPropertyUpdate->propertyReported, "Reported property is NULL");
            ASSERT_ARE_EQUAL(int, strlen(expectedReportedPropertyData), dtClientPropertyUpdate->propertyReportedLen, 
                             "Expected propertyReportedLen <%d> does not match actual <%d>", strlen(expectedReportedPropertyData), dtClientPropertyUpdate->propertyReportedLen);
            ASSERT_ARE_EQUAL(int, 0, strcmp(expectedReportedPropertyData, (char*)dtClientPropertyUpdate->propertyReported),
                             "Expected propertyReported <%s> does not match expected <%.*s>", expectedReportedPropertyData, dtClientPropertyUpdate->propertyReportedLen, dtClientPropertyUpdate->propertyReported);
        }
        else if (dtTestPropertyUpdateTest == DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED)
        {
            ASSERT_IS_NULL(dtClientPropertyUpdate->propertyReported);
            ASSERT_ARE_EQUAL(size_t, 0, dtClientPropertyUpdate->propertyReportedLen);
        }

        // Increment current property we're examining
        dtTestExpectedPropertyStatus.currentProperty++;
    }
}

static void testDTClientSendTelemetryCallback(DIGITALTWIN_CLIENT_RESULT dtTelemetryStatus, void* userContextCallback)
{
    (void)dtTelemetryStatus;
    (void)userContextCallback;
}

static void* testDTClientSendTelemetryCallbackContext = (void*)0x1259;

static void* dtClientReportedStatusCallbackContext = (void*)0x1260;

static IOTHUB_MESSAGE_HANDLE my_IoTHubMessage_CreateFromByteArray(const unsigned char* byteArray, size_t size)
{
    (void)byteArray;
    (void)size;
    return (IOTHUB_MESSAGE_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    my_gballoc_free(iotHubMessageHandle);
}


// Mock out DT_ClientCoreSendTelemetryAsync so that the framework can intercept the usercontext (which in this case comes from DT_InterfaceClient_Core layer, *not* actual developer).
// This context is then passed down on completing a send telemetry
static void* clientCore_SendTelemetryAsync_userContext;

static DIGITALTWIN_CLIENT_RESULT test_Impl_DT_ClientCoreSendTelemetryAsync(DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, IOTHUB_MESSAGE_HANDLE telemetryMessageHandle, void* userContextCallback)
{
    (void)dtClientCoreHandle;
    (void)dtInterfaceClientHandle;
    (void)telemetryMessageHandle;
    clientCore_SendTelemetryAsync_userContext = userContextCallback;
    return DIGITALTWIN_CLIENT_OK;
}

// Mock out DT_ClientCoreReportPropertyStatusAsync so that the framework can intercept the usercontext (which in this case comes from DT_InterfaceClient_Core layer, *not* actual developer).
// This context is then passed down on completing a reported property.
static void* clientCore_ReportProperty_user_context;

static DIGITALTWIN_CLIENT_RESULT test_Impl_DT_ClientCoreReportPropertyStatusAsync(DT_CLIENT_CORE_HANDLE dtClientCoreHandle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE dtInterfaceClientHandle, const unsigned char* dataToSend, size_t dataToSendLen, void* userContextCallback)
{
    (void)dtClientCoreHandle;
    (void)dtInterfaceClientHandle;
    (void)dataToSend;
    (void)dataToSendLen;
    // The caller allocates a context pointer and in product code, would rely in ClientCore layer giving a callback on completion to free.  Since we're mocking, just free here.
    clientCore_ReportProperty_user_context = userContextCallback;
    return DIGITALTWIN_CLIENT_OK;
}

static STRING_HANDLE my_STRING_from_byte_array(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static char* my_UUID_to_string(const UUID_T* uuid)
{
    (void)uuid;
    char* result;
    // We allocate for real because the SDK caller frees.  The UT itself checks that the request ID
    // is put into the json envelope correctly so cannot just return random data here.
    ASSERT_ARE_EQUAL(int, 0, real_mallocAndStrcpy_s(&result, dtTestAsyncCommandRequestId));
    return result;
}

static const char dtTestPropertyDataToReport[] = "Data to report for UT";
static const size_t dtTestPropertyDataToReportLen = sizeof(dtTestPropertyDataToReport) - 1;

static const char dtTestpropertyDesired[] = "Updated property data";
static const size_t dtTestpropertyDesiredLen = sizeof(dtTestpropertyDesired) - 1;
static const int dtTestPropertyUpdatedResponseVersion = 1;
static const int dtTestPropertyUpdatedStatus = 200;
static const char dtTestExpectedPropertyStatusDescription[] = "Status description";

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
MU_DEFINE_ENUM_STRINGS(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_RESULT_VALUES)


#ifdef __cplusplus
extern "C"
{
#endif

static DT_LOCK_THREAD_BINDING testLockThreadBinding;

#ifdef __cplusplus
}
#endif


BEGIN_TEST_SUITE(dt_interface_client_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(DT_CLIENT_CORE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_COMMAND_EXECUTE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_INTERFACE_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(DIGITALTWIN_CLIENT_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_init_object, real_json_value_init_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object, real_json_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotset_value, real_json_object_dotset_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, real_json_value_free);
    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, real_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);   
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_object, real_json_value_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);   
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotset_string, real_json_object_dotset_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotset_string, JSONFailure);   
    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_value, real_json_object_set_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotget_object, real_json_object_dotget_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_object, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_count, real_json_object_get_count);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value_at, real_json_object_get_value_at);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value_at, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_string, real_json_object_get_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_object, real_json_object_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotget_number, real_json_object_dotget_number);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value, real_json_object_get_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL); 
    REGISTER_GLOBAL_MOCK_HOOK(json_free_serialized_string, real_json_free_serialized_string);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotget_string, real_json_object_dotget_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_string, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_name, real_json_object_get_name);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_name, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_dotget_value, real_json_object_dotget_value);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_dotget_value, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_set_null, real_json_object_set_null);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_null, JSONFailure);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);    

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);   

    REGISTER_GLOBAL_MOCK_RETURN(UUID_generate, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UUID_generate, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(UUID_to_string, my_UUID_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(UUID_to_string, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_from_byte_array, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetProperty, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(testBindingLock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLock, LOCK_ERROR);
    
    REGISTER_GLOBAL_MOCK_RETURN(testBindingUnlock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingUnlock, LOCK_ERROR);
  
    REGISTER_GLOBAL_MOCK_HOOK(testBindingLockDeinit, impl_testBindingLockDeinit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLockDeinit, LOCK_ERROR);
    
    REGISTER_GLOBAL_MOCK_HOOK(testBindingLockInit, impl_testBindingLockInit);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(testBindingLockInit, NULL);
    
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_CreateFromByteArray, my_IoTHubMessage_CreateFromByteArray);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_CreateFromByteArray, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_Destroy, my_IoTHubMessage_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(DT_ClientCoreSendTelemetryAsync, test_Impl_DT_ClientCoreSendTelemetryAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_ClientCoreSendTelemetryAsync, DIGITALTWIN_CLIENT_ERROR);
    
    REGISTER_GLOBAL_MOCK_HOOK(DT_ClientCoreReportPropertyStatusAsync, test_Impl_DT_ClientCoreReportPropertyStatusAsync);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DT_ClientCoreReportPropertyStatusAsync, DIGITALTWIN_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(testDTClientCommandCallback, test_Impl_DT_CommandCallback);
    REGISTER_GLOBAL_MOCK_HOOK(testDTClientPropertyUpdate, test_Impl_DT_PropertyUpdate);
    REGISTER_GLOBAL_MOCK_HOOK(testInterfaceRegisteredCallback, impl_testInterfaceRegisteredCallback);

    REGISTER_GLOBAL_MOCK_RETURN(DigitalTwin_Client_GetVersionString, DIGITALTWIN_CLIENT_SDK_VERSION);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;
}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
    memset(&testLockThreadBinding, 0, sizeof(testLockThreadBinding));

    testLockThreadBinding.dtBindingLockHandle = testBindingIotHubBindingLockHandle;
    testLockThreadBinding.dtBindingLockInit = testBindingLockInit;
    testLockThreadBinding.dtBindingLock = testBindingLock;
    testLockThreadBinding.dtBindingUnlock = testBindingUnlock;
    testLockThreadBinding.dtBindingLockDeinit = testBindingLockDeinit;
    testLockThreadBinding.dtBindingThreadSleep = testBindingThreadSleep;

    clientCore_SendTelemetryAsync_userContext = NULL;
    clientCore_ReportProperty_user_context = NULL;
    intefaceClient_CommandCallbackSetsData = true;

    memset(&dtTestExpectedPropertyStatus, 0, sizeof(dtTestExpectedPropertyStatus));
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}


///////////////////////////////////////////////////////////////////////////////
// General utility used across UT's
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_IsInterfaceRegisteredWithCloud()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
}

static void test_free_bound_interface_handle(DIGITALTWIN_INTERFACE_CLIENT_HANDLE h)
{
    DT_InterfaceClient_UnbindFromClientHandle(h);
    DigitalTwin_InterfaceClient_Destroy(h);
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_Create
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DigitalTwin_InterfaceClient_Create()
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_ok)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    set_expected_calls_for_DigitalTwin_InterfaceClient_Create();

    //act
    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, NULL, NULL, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_register_callback_ok)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_CLIENT_RESULT result;
    set_expected_calls_for_DigitalTwin_InterfaceClient_Create();

    //act
    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, testInterfaceRegisteredCallback, NULL, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        
    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_NULL_interface_id_fails)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;

    //act
    result = DigitalTwin_InterfaceClient_Create(NULL, dtTestInterfaceInstanceName1, NULL, NULL, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_NULL_interface_name_fails)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;

    //act
    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, NULL, NULL, NULL, &h);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_NULL_handle_fails)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;

    //act
    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, NULL, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Create_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = NULL;
    DIGITALTWIN_CLIENT_RESULT result;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_Create();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //act
        result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, testInterfaceRegisteredCallback, testDTInterfaceCallbackContext, &h);

        //assert
        ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        ASSERT_IS_NULL(h, "Failure in test %lu", (unsigned long)i);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

// test_allocateDT_interface creates the DIGITALTWIN_INTERFACE_CLIENT_HANDLE, but doesn't perform the binding & registration steps.
// Attempts to use this handle by interface should fail.  This function does NOT setup any callbacks at this point.
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocateDT_interface()
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h;

    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, testInterfaceRegisteredCallback, testDTInterfaceCallbackContext, &h);

    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);

    umock_c_reset_all_calls();
    return h;    
}

// test_allocateDT_interface_with_callbacks creates the DIGITALTWIN_INTERFACE_CLIENT_HANDLE, but doesn't perform the binding & registration steps.
// Attempts to use this handle by interface should fail.  This function also sets up property and command callback tables
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocateDT_interface_with_callbacks()
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h;

    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, testInterfaceRegisteredCallback, testDTInterfaceCallbackContext, &h);

    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);

    result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    umock_c_reset_all_calls();
    return h;    
}

// test_allocate_and_bind_DT_interface creates the DIGITALTWIN_INTERFACE_CLIENT_HANDLE and binds it to (unit test) client core handle, but 
// it does not go through with actually simulating registering the handle.  Attempts to use this handle by interface should fail.
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocate_and_bind_DT_interface()
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    umock_c_reset_all_calls();
    return h;
}


// test_allocate_and_bind_DT_interface_with_callbacks creates the DIGITALTWIN_INTERFACE_CLIENT_HANDLE and binds it to (unit test) client core handle, but 
// it does not go through with actually simulating registering the handle.  Attempts to use this handle by interface should fail.
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocate_and_bind_DT_interface_with_callbacks()
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    umock_c_reset_all_calls();
    return h;
}

// test_allocate_and_register_DT_interface performs the required processing for creating a DIGITALTWIN_INTERFACE_CLIENT_HANDLE and connecting
// it to a (unit test) clientCore handle and also simulating its registration.  Interface is able to use the handle at this point.
// It does NOT register any callbacks, however.
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocate_and_register_DT_interface()
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    DT_InterfaceClient_RegistrationCompleteCallback(h, DIGITALTWIN_CLIENT_OK);
    
    umock_c_reset_all_calls();
    return h;
}


// test_allocate_and_register_DT_interface_with_callbacks performs the required processing for creating a DIGITALTWIN_INTERFACE_CLIENT_HANDLE and connecting
// it to a (unit test) clientCore handle and also simulating its registration.  Interface is able to use the handle at this point.
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocate_and_register_DT_interface_with_callbacks()
{
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    DT_InterfaceClient_RegistrationCompleteCallback(h, DIGITALTWIN_CLIENT_OK);
    
    umock_c_reset_all_calls();
    return h;
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_SetCommandsCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DigitalTwin_InterfaceClient_SetCommandsCallback()
{
    ;  
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    set_expected_calls_for_DigitalTwin_InterfaceClient_SetCommandsCallback();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_NULL_interface_handle)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetCommandsCallback(NULL, testDTClientCommandCallback);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_NULL_callback_function_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_already_bound_fail)
{
    //arrange
    // The interface handle is already bound.  Past this state, we will not accept new callback tables.
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_invoked_already_fail)
{
    //arrange - successfully setup callback table
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    umock_c_reset_all_calls();

    //act
    result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_CALLBACK_ALREADY_REGISTERED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetCommandsCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    DIGITALTWIN_CLIENT_RESULT result;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_SetCommandsCallback();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //act
        result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, testDTClientCommandCallback);

        //assert
        ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
    }

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback()
{
    ;
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    set_expected_calls_for_DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_NULL_interface_handle)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(NULL, testDTClientPropertyUpdate);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_NULL_callback_function_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_already_bound_fail)
{
    //arrange
    // The interface handle is already bound.  Past this state, we will not accept new callback tables.
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_ALREADY_REGISTERED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_invoked_already_fail)
{
    //arrange - successfully setup callback table
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);

    umock_c_reset_all_calls();

    //act
    result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_CALLBACK_ALREADY_REGISTERED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface();
    DIGITALTWIN_CLIENT_RESULT result;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        //act
        result = DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback(h, testDTClientPropertyUpdate);

        //assert
        ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
    }

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_BindToClientHandle
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceClient_BindToClientHandle()
{   
    STRICT_EXPECTED_CALL(testBindingLockInit());
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_ok)
{   
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_BindToClientHandle();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);    
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_NULL_interface_fails)
{   
    //arrange

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(NULL, testDTClientCoreHandle, &testLockThreadBinding);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);    
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_NULL_client_core_fails)
{   
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, NULL, &testLockThreadBinding);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);    
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_NULL_lock_binding_fails)
{   
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG);    
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_already_bound_fails)
{   
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    umock_c_reset_all_calls();

    //act
    // Since the initial call to DT_InterfaceClient_BindToClientHandle() already bound us, this one should fail.
    result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_ERROR);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_BindToClientHandle_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_BindToClientHandle();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_CreateTelemetryMessage
///////////////////////////////////////////////////////////////////////////////

static void set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(bool includeInterfaceId)
{
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    if (includeInterfaceId)
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetContentTypeSystemProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_no_interfaceId_ok)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(false);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(NULL, DT_TEST_INTERFACE_NAME_1,  dtTestTelemetryName, dtTestMessageData, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(messageHandle);

    //cleanup
    my_IoTHubMessage_Destroy(messageHandle);
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_with_interfaceId_ok)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(true);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_NAME_1,  dtTestTelemetryName, dtTestMessageData, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(messageHandle);

    //cleanup
    my_IoTHubMessage_Destroy(messageHandle);
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_NULL_InterfaceInstanceName_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, NULL, dtTestTelemetryName, dtTestMessageData, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(messageHandle);
}


TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_NULL_telemetry_name_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_NAME_1, NULL, dtTestMessageData, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(messageHandle);
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_NULL_messageData_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_NAME_1, dtTestTelemetryName, NULL, &messageHandle);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(messageHandle);
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_NULL_messageHandle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_NAME_1, dtTestTelemetryName, dtTestMessageData, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceClient_CreateTelemetryMessage_fail)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(true);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_CreateTelemetryMessage(DT_TEST_INTERFACE_ID_1, DT_TEST_INTERFACE_NAME_1,  dtTestTelemetryName, dtTestMessageData, &messageHandle);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
            ASSERT_IS_NULL(messageHandle);
        }
    }

    //cleanup
    umock_c_negative_tests_deinit();
}



///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_SendTelemetryAsync
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DigitalTwin_InterfaceClient_SendTelemetryAsync()
{
    set_expected_calls_for_IsInterfaceRegisteredWithCloud();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(false);
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(DT_ClientCoreSendTelemetryAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    set_expected_calls_for_DigitalTwin_InterfaceClient_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
    free(clientCore_SendTelemetryAsync_userContext);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_NULL_interface_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(NULL, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_NULL_telemetryName_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, NULL, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);  
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_NULL_messageData_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, NULL, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_interface_not_bound_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();
    set_expected_calls_for_DigitalTwin_InterfaceClient_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED, result);

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
    free(clientCore_SendTelemetryAsync_userContext);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_interface_not_registered_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    set_expected_calls_for_DigitalTwin_InterfaceClient_SendTelemetryAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED, result);

    //cleanup
    test_free_bound_interface_handle(h);
    free(clientCore_SendTelemetryAsync_userContext);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_SendTelemetryAsync_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_SendTelemetryAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_ReportPropertyAsync
///////////////////////////////////////////////////////////////////////////////
static DIGITALTWIN_INTERFACE_CLIENT_HANDLE test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(DIGITALTWIN_COMMAND_EXECUTE_CALLBACK dtCommandExecuteCallback)
{
    DIGITALTWIN_CLIENT_RESULT result;
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h;

    result = DigitalTwin_InterfaceClient_Create(dtTestInterfaceId1, dtTestInterfaceInstanceName1, testInterfaceRegisteredCallback, testDTInterfaceCallbackContext, &h);

    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);

    if (dtCommandExecuteCallback != NULL)
    {
        result = DigitalTwin_InterfaceClient_SetCommandsCallback(h, dtCommandExecuteCallback);
        ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    }

    result = DT_InterfaceClient_BindToClientHandle(h, testDTClientCoreHandle, &testLockThreadBinding);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, result, DIGITALTWIN_CLIENT_OK);

    DT_InterfaceClient_RegistrationCompleteCallback(h, DIGITALTWIN_CLIENT_OK);

    umock_c_reset_all_calls();
    return h;
}

static void set_expected_calls_for_DigitalTwin_InterfaceClient_ReportPropertyAsync()
{
    set_expected_calls_for_IsInterfaceRegisteredWithCloud();
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(DT_ClientCoreReportPropertyStatusAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void test_initialize_propertyResponse(DIGITALTWIN_CLIENT_PROPERTY_RESPONSE* propertyResponse)
{
    propertyResponse->version = DIGITALTWIN_CLIENT_PROPERTY_RESPONSE_VERSION_1;
    propertyResponse->responseVersion = dtTestPropertyUpdatedResponseVersion;
    propertyResponse->statusCode = dtTestPropertyUpdatedStatus;
    propertyResponse->statusDescription = dtTestExpectedPropertyStatusDescription;
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_specified_property_response_OK)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    set_expected_calls_for_DigitalTwin_InterfaceClient_ReportPropertyAsync();

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
    free(clientCore_ReportProperty_user_context);
}


TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_NULL_property_response_OK)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    set_expected_calls_for_DigitalTwin_InterfaceClient_ReportPropertyAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, NULL, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
    free(clientCore_ReportProperty_user_context);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_NULL_interface_handle_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(NULL, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_NULL_property_name_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, NULL, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_invalid_version_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);
    propertyResponse.version = 2;

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
    free(clientCore_ReportProperty_user_context);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_not_bound_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED, result);

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_not_registered_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INTERFACE_NOT_REGISTERED, result);

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_ReportPropertyAsync_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    DIGITALTWIN_CLIENT_PROPERTY_RESPONSE propertyResponse;
    test_initialize_propertyResponse(&propertyResponse);
    

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_ReportPropertyAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, &propertyResponse, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_CheckNameValid
///////////////////////////////////////////////////////////////////////////////
TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_ok)
{
    size_t i;
    for (i = 0; i < DT_TEST_Valid_InterfaceIdsLen; i++)
    {
        const char* valueToCheck = DT_TEST_Valid_InterfaceIds[i];
        ASSERT_ARE_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(valueToCheck, true), "Value=%s failed but should have succeeded", valueToCheck);
    }

    for (i = 0; i < DT_TEST_Valid_InterfaceInstanceNamesLen; i++)
    {
        const char* valueToCheck = DT_TEST_Valid_InterfaceInstanceNames[i];
        ASSERT_ARE_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(valueToCheck, false), "Value=%s failed but should have succeeded", valueToCheck);
    }
}


TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_null_ValueToCheckName_fails)
{
    ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(NULL, true));
    ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(NULL, false));
}

TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_no_urn_in_interface_fails)
{
    size_t i;
    for (i = 0; i < DT_TEST_MissingUrn_InterfaceIdsLen; i++)
    {
        const char* valueToCheck = DT_TEST_MissingUrn_InterfaceIds[i];
        ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(valueToCheck, true), "Value=%s succeeded but should have failed", valueToCheck);
    }
}

TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_invalid_character_in_interface_fails)
{
    int i;
    char invalidName[128];
    char* invalidNamePointer = invalidName + 5;

    // Copy to local buffer since we can't overwrite const field
    strcpy(invalidName, DT_TEST_Valid_InterfaceIds[0]);

    // Iterate through each char, 1 to 0xff.  Put any invalid characters into the array in 5th location.
    for (i = 1; i < 256; i++)
    {
        char c = (char)i;
        if (ISDIGIT(c) || (c == '_') || (c == ':'))
        {
            // Current character is legal, so skip
            continue;
        }
        else if (i < 127 && isalpha(c))
        {
            // Current character is legal, so skip.  Note the i<127 check is because isalpha is only defined up to 127.
            continue;
        }
        // Set invalid character to partway through the otherwise legit interface name.
        *invalidNamePointer = c;
        
        ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(invalidName, true), "Value=%s succeeded but should have failed, i=%d", invalidName, i);
    }
}

TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_invalid_character_in_name_fails)
{
    int i;
    char invalidName[128];
    char* invalidNamePointer = invalidName + 5;

    // Copy to local buffer since we can't overwrite const field
    strcpy(invalidName, DT_TEST_Valid_InterfaceInstanceNames[0]);

    // Iterate through each char, 1 to 0xff.  Put any invalid characters into the array in 5th location.
    for (i = 1; i < 256; i++)
    {
        char c = (char)i;
        if (ISDIGIT(c) || (c == '_'))
        {
            // Current character is legal, so skip
            continue;
        }
        else if (i < 127 && isalpha(c))
        {
            // Current character is legal, so skip.  Note the i<127 check is because isalpha is only defined up to 127.
            continue;
        }
        // Set invalid character to partway through the otherwise legit interface name.
        *invalidNamePointer = c;
        
        ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(invalidName, false), "Value=%s succeeded but should have failed, i=%d", invalidName, i);
    }
}

TEST_FUNCTION(DT_InterfaceClient_CheckNameValid_name_too_long_fails)
{
    // ASSERT
    ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(DT_Test_InterfaceIdTooLong, true), "Value=%s failed but should have succeeded", DT_Test_InterfaceIdTooLong);
    ASSERT_ARE_NOT_EQUAL(int, 0, DT_InterfaceClient_CheckNameValid(DT_Test_InterfaceInstanceNameTooLong, false), "Value=%s failed but should have succeeded", DT_Test_InterfaceInstanceNameTooLong);
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_GetInterfaceInstanceName
///////////////////////////////////////////////////////////////////////////////
TEST_FUNCTION(DT_InterfaceClient_GetInterfaceInstanceName_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    //act
    const char* result = DT_InterfaceClient_GetInterfaceInstanceName(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, dtTestInterfaceInstanceName1, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_GetInterfaceInstanceName_NULL_handle_returns_NULL)
{
    //act
    const char* result = DT_InterfaceClient_GetInterfaceInstanceName(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_Destroy
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_free_DT_InterfaceClientCore()
{
    STRICT_EXPECTED_CALL(testBindingLockDeinit(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DigitalTwin_InterfaceClient_Destroy()
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG));

    set_expected_calls_for_free_DT_InterfaceClientCore();
}

static void set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(bool pendingDelete)
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    if (pendingDelete)
    {
        STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
        set_expected_calls_for_free_DT_InterfaceClientCore();
    }
    else
    {
        STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(testInterfaceRegisteredCallback(DIGITALTWIN_CLIENT_ERROR_INTERFACE_UNREGISTERING, testDTInterfaceCallbackContext));
        STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
    }
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Destroy_no_commands_properties_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(NULL);

    set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(false);
    DT_InterfaceClient_UnbindFromClientHandle(h);
    umock_c_reset_all_calls();

    set_expected_calls_for_DigitalTwin_InterfaceClient_Destroy();

    //act
    DigitalTwin_InterfaceClient_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Destroy_multiple_commands_properties_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(false);
    DT_InterfaceClient_UnbindFromClientHandle(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();

    set_expected_calls_for_DigitalTwin_InterfaceClient_Destroy();

    //act
    DigitalTwin_InterfaceClient_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_Destroy_NULL_handle_fails)
{
    //act
    DigitalTwin_InterfaceClient_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_UnbindFromClientHandle
///////////////////////////////////////////////////////////////////////////////
TEST_FUNCTION(DT_InterfaceClient_UnbindFromClientHandle_destroy_first_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    DigitalTwin_InterfaceClient_Destroy(h);
    umock_c_reset_all_calls();
    set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(true);

    //act
    DT_InterfaceClient_UnbindFromClientHandle(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceClient_UnbindFromClientHandle_destroy_after_unbind_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(false);

    //act
    DT_InterfaceClient_UnbindFromClientHandle(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

TEST_FUNCTION(DT_InterfaceClient_UnbindFromClientHandle_NULL_handle_fails)
{
    //act
    DT_InterfaceClient_UnbindFromClientHandle(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests that if interface handle is created, but NOT bound to clientCore, that we successfully ignore this.
TEST_FUNCTION(DT_InterfaceClient_UnbindFromClientHandle_not_yet_bound_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocateDT_interface_with_callbacks();

    //act
    DT_InterfaceClient_UnbindFromClientHandle(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

// Tests that if DT_InterfaceClient_UnbindFromClientHandle is called multiple times, we successfully
// ignore subsequent one.  Note that we cannot have called destroy yet, since an interface unbind/destroy
// (in whichever order) will result in the memory being deleted.  An unbind on deleted handle will be a crash.
TEST_FUNCTION(DT_InterfaceClient_UnbindFromClientHandle_multiple_unbind_calls_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_UnbindFromClientHandle(false);

    DT_InterfaceClient_UnbindFromClientHandle(h);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();

    //act
    // subsequent call to DT_InterfaceClient_UnbindFromClientHandle(h) should gracefully ignore request.
    DT_InterfaceClient_UnbindFromClientHandle(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    DigitalTwin_InterfaceClient_Destroy(h);
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_RegistrationCompleteCallback
///////////////////////////////////////////////////////////////////////////////

static void set_expected_calls_for_DT_InterfaceClient_RegistrationCompleteCallback(DIGITALTWIN_CLIENT_RESULT dtInterfaceStatus)
{
    STRICT_EXPECTED_CALL(testBindingLock(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(testBindingUnlock(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(testInterfaceRegisteredCallback(dtInterfaceStatus, testDTInterfaceCallbackContext));
}

static void testDTInterfaceClientCore_RegistrationCompleteCallback_failure_code(DIGITALTWIN_CLIENT_RESULT status)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_RegistrationCompleteCallback(status);

    //act
    DT_InterfaceClient_RegistrationCompleteCallback(h, status);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_RegistrationCompleteCallback(DIGITALTWIN_CLIENT_OK);

    //act
    DT_InterfaceClient_RegistrationCompleteCallback(h, DIGITALTWIN_CLIENT_OK);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_NULL_handle_fails)
{
    //act
    DT_InterfaceClient_RegistrationCompleteCallback(NULL, DIGITALTWIN_CLIENT_OK);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_handle_destroyed_error_fails)
{
    testDTInterfaceClientCore_RegistrationCompleteCallback_failure_code(DIGITALTWIN_CLIENT_ERROR_HANDLE_DESTROYED);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_out_of_memory_error_fails)
{
    testDTInterfaceClientCore_RegistrationCompleteCallback_failure_code(DIGITALTWIN_CLIENT_ERROR_OUT_OF_MEMORY);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_timeout_error_fails)
{
    testDTInterfaceClientCore_RegistrationCompleteCallback_failure_code(DIGITALTWIN_CLIENT_ERROR_TIMEOUT);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_error_fails)
{
    testDTInterfaceClientCore_RegistrationCompleteCallback_failure_code(DIGITALTWIN_CLIENT_ERROR);
}

TEST_FUNCTION(DT_InterfaceClient_RegistrationCompleteCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_bind_DT_interface_with_callbacks();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_RegistrationCompleteCallback(DIGITALTWIN_CLIENT_OK);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DT_InterfaceClient_RegistrationCompleteCallback(h, DIGITALTWIN_CLIENT_OK);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}



///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_GetInterfaceId
///////////////////////////////////////////////////////////////////////////////
TEST_FUNCTION(DT_InterfaceClient_GetInterfaceId_ok)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    //act
    const char* result = DT_InterfaceClient_GetInterfaceId(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DT_TEST_INTERFACE_ID_1, result); 
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}


TEST_FUNCTION(DT_InterfaceClient_GetInterfaceId_NULL_handle_returns_NULL)
{
    //act
    const char* result = DT_InterfaceClient_GetInterfaceId(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_ProcessTwinCallback
///////////////////////////////////////////////////////////////////////////////

// TestProcessTwinCallback_Info structure specifies which property callbacks, if any, 
// we expect to be invoked on a given test run.  Also helps with knowing which
// STRICT_EXPETED_CALL's should be invoked
typedef struct TestProcessTwinCallback_Info_Tag
{
    bool fullTwin; // Specifies whether this is a fullTwin or partial twin update; impacts STRICT_EXPETED_CALL's
    unsigned int numDesiredPropertiesFromJson; // Number of child elements of "desired" for this interface.  Does NOT matter how they are mapped to interface handle, just that its from JSON proper
    bool expectReportedJsonForCurrentInterface; // Whether this particular interface has any "reported { <intfName>" json for it
} TestProcessTwinCallback_Info;

static void set_expected_calls_for_GetDesiredAndReportedJsonObjects(const TestProcessTwinCallback_Info* twinCallbackInfo)
{
    if (twinCallbackInfo->fullTwin)
    {
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    }
}

static void set_expected_calls_for_GetPayloadFromProperty()
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_dotget_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

// Note that because at this stage call failures do not translate into error codes returned to caller,
// everything here is marked as a CallCannotFail().
static void set_expected_calls_for_ProcessPropertyUpdateIfNeededFromDesired(const TestProcessTwinCallback_Info* twinCallbackInfo)
{
    STRICT_EXPECTED_CALL(json_object_get_name(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).CallCannotFail();
    set_expected_calls_for_GetPayloadFromProperty();

    if (twinCallbackInfo->expectReportedJsonForCurrentInterface)
    {
        // If the twin had a reported section, then product code will GetPayloadFromProperty on it.
        set_expected_calls_for_GetPayloadFromProperty();
    }
        
    STRICT_EXPECTED_CALL(testDTClientPropertyUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
 }

static void set_expected_calls_for_ProcessPropertiesForTwin(const TestProcessTwinCallback_Info* twinCallbackInfo)
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();

    // Mark json_object_get_object as CallCannotFail because its legitimate for JSON to not have given field, so negative tests should skip this case.
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    if (twinCallbackInfo->fullTwin)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        // Mark json_object_get_object as CallCannotFail because its legitimate for JSON to not have given field, so negative tests should skip this case.
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    }

    if ((twinCallbackInfo->numDesiredPropertiesFromJson > 0) || (twinCallbackInfo->expectReportedJsonForCurrentInterface == true))
    {
        // Mark CallCannotFail() because Parson always returns numbers (even if 0), not proper errors, so there is no way to error test.
        STRICT_EXPECTED_CALL(json_object_dotget_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(json_object_get_count(IGNORED_PTR_ARG)).CallCannotFail();

        for (unsigned int i = 0; i < twinCallbackInfo->numDesiredPropertiesFromJson; i++)
        {
            set_expected_calls_for_ProcessPropertyUpdateIfNeededFromDesired(twinCallbackInfo);
        }
    }

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DT_InterfaceClient_ProcessTwinCallback(const TestProcessTwinCallback_Info* twinCallbackInfo)
{
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));

    set_expected_calls_for_GetDesiredAndReportedJsonObjects(twinCallbackInfo);
    set_expected_calls_for_ProcessPropertiesForTwin(twinCallbackInfo);

    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void test_DT_InterfaceClient_ProcessTwinCallback(const char* testJson, size_t testJsonLen, const TestProcessTwinCallback_Info* twinCallbackInfo)
{
    printf("Testing twin Json <%s>\n", testJson);
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_ProcessTwinCallback(twinCallbackInfo);

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, twinCallbackInfo->fullTwin, (const unsigned char*)testJson, testJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

// Tests case where full twin data contains "DTTestProperty1" as only property.  Expect testDTClientPropertyUpdate to be invoked
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property1_set_full_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true,  // Using a full twin
        1,     // There is 1 desired property specified in twin
        false  // No reported properties are present 
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME1;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE1;
    dtTestExpectedPropertyStatus.maxProperty = 1;
  
    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyFullTwin1, updatePropertiesPropertyFullTwin1Len, &callbackInfo);
}

// Tests case where full twin data contains "DTTestProperty2" as only property.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property2_set_full_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        1,    // There is 1 desired property specified in twin
        false // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME2;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE2;
    dtTestExpectedPropertyStatus.maxProperty = 1;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyFullTwin2, updatePropertiesPropertyFullTwin2Len, &callbackInfo);
}

// Tests case where full twin data contains "DTTestProperty3" as only property.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property3_set_full_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        1,    // There is 1 desired property specified in twin
        false // No reported properties are present
     };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME3;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE3;
    dtTestExpectedPropertyStatus.maxProperty = 1;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyFullTwin3, updatePropertiesPropertyFullTwin3Len, &callbackInfo);
}

// Tests case where partial twin data contains "DTTestProperty1" as only property.  Expect testDTClientPropertyUpdate1 to be invoked
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property1_set_update_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        false, // Using a non-full twin
        1,     // There is 1 desired property specified in twin
        false  // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME1;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE1;
    dtTestExpectedPropertyStatus.maxProperty = 1;
    
    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyUpdateTwin1, updatePropertiesPropertyUpdateTwin1Len, &callbackInfo);
}

// Tests case where full twin data contains "DTTestProperty2" as only property.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property2_set_update_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        false, // Using a non-full twin
        1,     // There is 1 desired property specified in twin
        false  // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME2;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE2;
    dtTestExpectedPropertyStatus.maxProperty = 1;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyUpdateTwin2, updatePropertiesPropertyUpdateTwin2Len, &callbackInfo);
}

// Tests case where full twin data contains "DTTestProperty3" as only property.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property3_update_twin_set_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        false, // Using a non-full twin
        1,     // There is 1 desired property specified in twin
        false  // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME3;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE3;
    dtTestExpectedPropertyStatus.maxProperty = 1;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyUpdateTwin3, updatePropertiesPropertyUpdateTwin3Len, &callbackInfo);
}

// Tests case where legit json arrives, but it is NOT for this interface
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property3_different_interface_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        false, // Using a non-full twin
        0,     // There is 0 desired property specified in twin (because it arrives on different interface)
        false  // No reported properties are present
    };

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesDifferentInterfaceFullTwin, updatePropertiesDifferentInterfaceFullTwinLen, &callbackInfo);
}

// Tests case where legit json arrives, both for this interface AND random unrelated.  Expect our callback to be happily called and non-relavant data to be ignored.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_property1_and_unrelated_interface_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        1,    // There is 1 desired property specified in twin
        false // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME1;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE1;
    dtTestExpectedPropertyStatus.maxProperty = 1;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyFullTwin1AndRandomInterfaces, updatePropertiesPropertyFullTwin1AndRandomInterfacesLen, &callbackInfo);
}

// Tests that when all 3 properties are updated, everything is processed correctly
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_three_properties_update_full_twin_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        3,    // There are 3 desired property specified in twin
        false // No reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME1;
    dtTestExpectedPropertyStatus.expectedPropertyName[1] = DT_TEST_PROPERTY_NAME2;
    dtTestExpectedPropertyStatus.expectedPropertyName[2] = DT_TEST_PROPERTY_NAME3;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE1;
    dtTestExpectedPropertyStatus.expectedDesiredData[1] = DT_TEST_PROPERTY_DESIRED_VALUE2;
    dtTestExpectedPropertyStatus.expectedDesiredData[2] = DT_TEST_PROPERTY_DESIRED_VALUE3;
    dtTestExpectedPropertyStatus.maxProperty = 3;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesPropertyFullTwinAll, updatePropertiesPropertyFullTwinAllLen, &callbackInfo);
}

// Tests case where JSON arrives but the interface never called DigitalTwin_InterfaceClient_SetPropertiesUpdatedCallback.
// This is not an error; the interface should just skip past JSON parsing.
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_no_callback_registered_by_application_OK)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface();
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, true, (const unsigned char*)updatePropertiesPropertyFullTwinAll, updatePropertiesPropertyFullTwinAllLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}


// Tests that when all 3 properties are updated (for both desired + reported), everything is processed correctly
TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_three_properties_update_full_twin_with_all_reported_ok)
{
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_REPORTED_EXPECTED;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        3,    // There are 3 desired properties specified in twin
        true  // Reported properties are present
    };

    dtTestExpectedPropertyStatus.expectedPropertyName[0] = DT_TEST_PROPERTY_NAME1;
    dtTestExpectedPropertyStatus.expectedPropertyName[1] = DT_TEST_PROPERTY_NAME2;
    dtTestExpectedPropertyStatus.expectedPropertyName[2] = DT_TEST_PROPERTY_NAME3;
    dtTestExpectedPropertyStatus.expectedDesiredData[0] = DT_TEST_PROPERTY_DESIRED_VALUE1;
    dtTestExpectedPropertyStatus.expectedDesiredData[1] = DT_TEST_PROPERTY_DESIRED_VALUE2;
    dtTestExpectedPropertyStatus.expectedDesiredData[2] = DT_TEST_PROPERTY_DESIRED_VALUE3;
    dtTestExpectedPropertyStatus.expectedReportedData[0] = DT_TEST_PROPERTY_REPORTED_VALUE1;
    dtTestExpectedPropertyStatus.expectedReportedData[1] = DT_TEST_PROPERTY_REPORTED_VALUE2;
    dtTestExpectedPropertyStatus.expectedReportedData[2] = DT_TEST_PROPERTY_REPORTED_VALUE3;
    
    dtTestExpectedPropertyStatus.maxProperty = 3;

    test_DT_InterfaceClient_ProcessTwinCallback(updatePropertiesDesiredAndReportedFullTwinAll, updatePropertiesDesiredAndReportedFullTwinAllLen, &callbackInfo);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_invalid_json)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, true, (const unsigned char*)updatePropertiesInvalidJson, updatePropertiesInvalidJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR, result);

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_FAILURE_TEST;

    TestProcessTwinCallback_Info callbackInfo = { 
        true, // Using a full twin
        3,    // There are 3 desired properties specified in twin
        true  // No reported properties are present
    };

    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    set_expected_calls_for_DT_InterfaceClient_ProcessTwinCallback(&callbackInfo);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, true, (const unsigned char*)updatePropertiesDesiredAndReportedFullTwinAll, updatePropertiesDesiredAndReportedFullTwinAllLen);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_NULL_handle_fails)
{
    //arrange
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(NULL, true, (const unsigned char*)updatePropertiesInvalidJson, updatePropertiesInvalidJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_NULL_payload_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, true, NULL, updatePropertiesInvalidJsonLen);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTwinCallback_0_payloadLen_fails)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();
    dtTestPropertyUpdateTest = DT_TEST_PROPERTY_UPDATE_TEST_NO_REPORTED_EXPECTED;

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTwinCallback(h, true, (const unsigned char*)updatePropertiesInvalidJson, 0);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);

    //cleanup
    test_free_bound_interface_handle(h);
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_InvokeCommandIfSupported (syncronous methods and general helpers)
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_json_parsing()
{
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_dotget_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_dotget_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_resource_deallocation(bool testingAsync)
{
    if (testingAsync)
    {
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
}


static void set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported(int expectedCommandMatchFunction)
{
    switch (expectedCommandMatchFunction)
    {
        case DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK:
            set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_json_parsing();
            STRICT_EXPECTED_CALL(testDTClientCommandCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_resource_deallocation(false);
        break;

        case DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE:
            set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_json_parsing();
            STRICT_EXPECTED_CALL(testDTClientCommandCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported_resource_deallocation(false);
        break;

        case DT_TEST_EXPECTED_INTERFACE_NO_MATCH:
            // If an interface isn't found, we exit very quickly
            ;
        break;

        default:
            ASSERT_FAIL("Unknown match function %d", expectedCommandMatchFunction);
            break;
    }
}

// Based on which command was expected to be invoked, figure out the expected data to have been returned.
static void testDT_VerifyCommandResponse(int expectedCommandMatchFunction, unsigned char* actualResponse, size_t actualResponseLen, int actualResponseCode)
{
    const char* expectedResponse = NULL;
    size_t expectedResponseLen = 0;
    int expectedResponseCode = 0;
        
    switch (expectedCommandMatchFunction)
    {
        case DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK:
            // When the command callback specifies data for response, this layer of SDK returns it directly.
            expectedResponse = dtTestCommandResponse1;
            expectedResponseCode = dtTestCommandResponseStatus1;
        break;

        case DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE:
            // When the command callback specifies a NULL for response, the SDK will override this with a "null" before returning
            expectedResponse = dtTestJsonNull;
            expectedResponseCode = dtTestCommandResponseStatus1;
        break;

        case DT_TEST_EXPECTED_INTERFACE_NO_MATCH:
            ;
        break;

        default:
            ASSERT_FAIL("Unknown match function %d", expectedCommandMatchFunction);
            break;
    }   

    if (expectedResponse != NULL) 
    {
        expectedResponseLen = strlen(expectedResponse);
    } 

    ASSERT_ARE_EQUAL(char_ptr, expectedResponse, actualResponse, "Response data from command %d didn't match", expectedCommandMatchFunction);
    ASSERT_ARE_EQUAL(int, (int)expectedResponseLen, actualResponseLen, "Response len from command %d didn't match", expectedCommandMatchFunction);
    ASSERT_ARE_EQUAL(int, expectedResponseCode, actualResponseCode, "Response code %d didn't match", expectedCommandMatchFunction);
}

static void test_DT_InterfaceClient_InvokeCommandIfSupported(DIGITALTWIN_COMMAND_EXECUTE_CALLBACK dtCommandExecuteCallback, DT_COMMAND_PROCESSOR_RESULT expectedResult, const char* methodName, int expectedCommandMatchFunction)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(dtCommandExecuteCallback);
    set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported(expectedCommandMatchFunction);

    if (expectedCommandMatchFunction == DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE)
    {
        intefaceClient_CommandCallbackSetsData = false;
    }

    //act
    unsigned char* response = NULL;
    size_t responseLen = 0;
    int responseCode = 0;
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, methodName, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, &response, &responseLen, &responseCode);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, expectedResult, result, "DigitalTwin command processor results don't match");
    testDT_VerifyCommandResponse(expectedCommandMatchFunction, response, responseLen, responseCode);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    my_gballoc_free(response);

    //cleanup
    test_free_bound_interface_handle(h);
}

// The interface instance name is a match and response returned from callback is valid string.
TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_InterfaceInstanceName_matches_ok)
{
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);
    test_DT_InterfaceClient_InvokeCommandIfSupported(testDTClientCommandCallback, DT_COMMAND_PROCESSOR_PROCESSED, testCommandName, DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK);
}

// The interface instance name is a match, but response returned from callback is NULL
TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_InterfaceInstanceName_no_response_set_matches_ok)
{
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);
    test_DT_InterfaceClient_InvokeCommandIfSupported(testDTClientCommandCallback, DT_COMMAND_PROCESSOR_PROCESSED, testCommandName, DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE);
}


// DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK_RETURNS_NULL_RESPONSE

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_no_commands_registered_fail)
{
    // This interface doesn't have any commands registered to it.
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);
    test_DT_InterfaceClient_InvokeCommandIfSupported(NULL, DT_COMMAND_PROCESSOR_NOT_APPLICABLE, testCommandName, DT_TEST_EXPECTED_INTERFACE_NO_MATCH);
}

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_not_for_this_interface_fail)
{
    // This interface doesn't support the requested interface (which is how product legitimately works, because caller
    // visits each interfaceCore not knowing whether its supported or not).
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NOT_REGISTERED_NAME, DT_TEST_COMMAND_NAME);
    test_DT_InterfaceClient_InvokeCommandIfSupported(testDTClientCommandCallback, DT_COMMAND_PROCESSOR_NOT_APPLICABLE, testCommandName, DT_TEST_EXPECTED_INTERFACE_NO_MATCH);
}

TEST_FUNCTION(test_DT_InterfaceClient_InvokeCommandIfSupported_malformed_command_name_fail)
{
    const char* testCommandName = "ThisCommandIsNotLegalSyntax";
    test_DT_InterfaceClient_InvokeCommandIfSupported(testDTClientCommandCallback, DT_COMMAND_PROCESSOR_NOT_APPLICABLE, testCommandName, DT_TEST_EXPECTED_INTERFACE_NO_MATCH);
}

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_NULL_method_name_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);

    //act
    unsigned char* response = NULL;
    size_t responseLen = 0;
    int responseCode = 0;
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, NULL, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, &response, &responseLen, &responseCode);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_ERROR, result);
    testDT_VerifyCommandResponse(DT_TEST_EXPECTED_INTERFACE_NO_MATCH, response, responseLen, responseCode);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_NULL_response_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);

    //act
    unsigned char* response = NULL;
    size_t responseLen = 0;
    int responseCode = 0;
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, testCommandName, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, NULL, &responseLen, &responseCode);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_ERROR, result);
    testDT_VerifyCommandResponse(DT_TEST_EXPECTED_INTERFACE_NO_MATCH, response, responseLen, responseCode);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_NULL_response_len_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);

    //act
    unsigned char* response = NULL;
    size_t responseLen = 0;
    int responseCode = 0;
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, testCommandName, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, &response, NULL, &responseCode);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_ERROR, result);
    testDT_VerifyCommandResponse(DT_TEST_EXPECTED_INTERFACE_NO_MATCH, response, responseLen, responseCode);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_NULL_response_code_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);

    //act
    unsigned char* response = NULL;
    size_t responseLen = 0;
    int responseCode = 0;
    DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, testCommandName, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, &response, &responseLen, NULL);

    //assert
    ASSERT_ARE_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_ERROR, result);
    testDT_VerifyCommandResponse(DT_TEST_EXPECTED_INTERFACE_NO_MATCH, response, responseLen, responseCode);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}


TEST_FUNCTION(DT_InterfaceClient_InvokeCommandIfSupported_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    const char* testCommandName = DT_TEST_BUILD_COMMAND_NAME(DT_TEST_INTERFACE_NAME_1, DT_TEST_COMMAND_NAME);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_InvokeCommandIfSupported(DT_TEST_EXPECTED_COMMAND_SYNC_CALLBACK);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            unsigned char* response = NULL;
            size_t responseLen = 0;
            int responseCode = 0;

            //act
            DT_COMMAND_PROCESSOR_RESULT result = DT_InterfaceClient_InvokeCommandIfSupported(h, testCommandName, (const unsigned char *)dtTestCommandPayload, dtTestCommandPayloadLen, &response, &responseLen, &responseCode);

            //assert
            ASSERT_ARE_NOT_EQUAL(DT_COMMAND_PROCESSOR_RESULT, DT_COMMAND_PROCESSOR_PROCESSED, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync()
{
    set_expected_calls_for_IsInterfaceRegisteredWithCloud();
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
    set_expected_calls_for_DT_InterfaceClient_CreateTelemetryMessage(false);
    STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetProperty(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DT_ClientCoreSendTelemetryAsync(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

// Helper to test various permutations of invalid DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE fields
static void testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(const DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE* dtClientAsyncCommandUpdate)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(h, dtClientAsyncCommandUpdate);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

void testDT_SetAsyncUpdate(DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE* dtClientAsyncCommandUpdate)
{
    memset(dtClientAsyncCommandUpdate, 0, sizeof(*dtClientAsyncCommandUpdate));
    dtClientAsyncCommandUpdate->version = DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE_VERSION_1;
    dtClientAsyncCommandUpdate->commandName = DT_TEST_ASYNC_UPDATING_COMMAND_NAME;
    dtClientAsyncCommandUpdate->requestId = dtTestAsyncCommandRequestId;
    dtClientAsyncCommandUpdate->propertyData  = dtTestAsyncCommandResponse3;
    dtClientAsyncCommandUpdate->statusCode = DIGITALTWIN_ASYNC_STATUS_CODE_PENDING;
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_ok)
{
    //arrange
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);

    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);
    set_expected_calls_for_DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(h, &dtClientAsyncCommandUpdate);
    
    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_NULL_asyncUpdate)
{
    testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(NULL);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_invalid_version_fail)
{
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);
    dtClientAsyncCommandUpdate.version = 2;
    testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(&dtClientAsyncCommandUpdate);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_NULL_command_fail)
{
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);
    dtClientAsyncCommandUpdate.commandName = NULL;
    testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(&dtClientAsyncCommandUpdate);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_NULL_requestId_fail)
{
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);
    dtClientAsyncCommandUpdate.requestId = NULL;
    testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(&dtClientAsyncCommandUpdate);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_NULL_propertyData_fail)
{
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);
    dtClientAsyncCommandUpdate.propertyData = NULL;
    testDT_UpdateAsyncCommandStatus_AsyncUpdateStruct_invalid(&dtClientAsyncCommandUpdate);
}

TEST_FUNCTION(DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync_fail)
{
    // arrange
    DIGITALTWIN_CLIENT_ASYNC_COMMAND_UPDATE dtClientAsyncCommandUpdate;
    testDT_SetAsyncUpdate(&dtClientAsyncCommandUpdate);
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks_specifying_commands(testDTClientCommandCallback);

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync();
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_UpdateAsyncCommandStatusAsync(h, &dtClientAsyncCommandUpdate);
            
            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
        }
    }

    //cleanup
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_ProcessTelemetryCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_CLIENT_RESULT expectedDTSendTelemetryStatus, void* expectedUserContextCallback)
{
    STRICT_EXPECTED_CALL(testDTClientTelemetryConfirmationCallback(expectedDTSendTelemetryStatus, expectedUserContextCallback));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void test_DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_CLIENT_RESULT expectedDTSendTelemetryStatus)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    // Need to initiate a send telemetry so that there's a context to callback onto.
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(clientCore_SendTelemetryAsync_userContext, "User context not set during mocked client core sendTelemetry callback");
    umock_c_reset_all_calls();
    
    set_expected_calls_for_DT_InterfaceClient_ProcessTelemetryCallback(expectedDTSendTelemetryStatus, testDTClientSendTelemetryCallbackContext);

    //act
    result = DT_InterfaceClient_ProcessTelemetryCallback(h, expectedDTSendTelemetryStatus, clientCore_SendTelemetryAsync_userContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_ok)
{
    test_DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_CLIENT_OK);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_caller_sets_error)
{
    test_DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_CLIENT_ERROR);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_NULL_interface_handle_fails)
{
    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTelemetryCallback(NULL, DIGITALTWIN_CLIENT_OK, testDTClientSendTelemetryCallbackContext);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_ERROR_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_NULL_user_context_succeeds)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    //act
    DIGITALTWIN_CLIENT_RESULT result = DT_InterfaceClient_ProcessTelemetryCallback(h, DIGITALTWIN_CLIENT_OK, NULL);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessTelemetryCallback_fail)
{
    // arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    // Need to initiate a send telemetry so that there's a context to callback onto.
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_SendTelemetryAsync(h, dtTestTelemetryName, dtTestMessageData, testDTClientTelemetryConfirmationCallback, testDTClientSendTelemetryCallbackContext);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(clientCore_SendTelemetryAsync_userContext, "User context not set during mocked client core sendTelemetry callback");
    umock_c_reset_all_calls();
    
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_ProcessTelemetryCallback(DIGITALTWIN_CLIENT_OK, testDTClientSendTelemetryCallbackContext);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            result = DT_InterfaceClient_ProcessTelemetryCallback(h, DIGITALTWIN_CLIENT_OK, clientCore_SendTelemetryAsync_userContext);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    free(clientCore_SendTelemetryAsync_userContext);
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}


///////////////////////////////////////////////////////////////////////////////
// DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback
///////////////////////////////////////////////////////////////////////////////
static void set_expected_calls_for_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_CLIENT_RESULT expectedDTReportedStatus)
{
    STRICT_EXPECTED_CALL(testDTClientReportedPropertyCallback(expectedDTReportedStatus, dtClientReportedStatusCallbackContext));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void test_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_CLIENT_RESULT expectedDTReportedStatus)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    // Need to initiate a send telemetry so that there's a context to callback onto.
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, NULL, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(clientCore_ReportProperty_user_context, "User context not set during mocked client core reportedProperty callback");
    umock_c_reset_all_calls();
    
    set_expected_calls_for_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(expectedDTReportedStatus);

    //act
    result = DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(h, expectedDTReportedStatus, clientCore_ReportProperty_user_context);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    test_free_bound_interface_handle(h);

}

TEST_FUNCTION(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback_ok)
{
    test_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_CLIENT_OK);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback_caller_sets_error)
{
    test_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_CLIENT_ERROR);
}

TEST_FUNCTION(DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback_fail)
{
    //arrange
    DIGITALTWIN_INTERFACE_CLIENT_HANDLE h = test_allocate_and_register_DT_interface_with_callbacks();

    // Need to initiate a send telemetry so that there's a context to callback onto.
    DIGITALTWIN_CLIENT_RESULT result = DigitalTwin_InterfaceClient_ReportPropertyAsync(h, DT_TEST_PROPERTY_NAME1, DT_TEST_PROPERTY_REPORTED_VALUE1, NULL, testDTClientReportedPropertyCallback, dtClientReportedStatusCallbackContext);
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(clientCore_ReportProperty_user_context, "User context not set during mocked client core reportedProperty callback");
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(DIGITALTWIN_CLIENT_OK);
    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t i = 0; i < count; i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            result = DT_InterfaceClient_ProcessReportedPropertiesUpdateCallback(h, DIGITALTWIN_CLIENT_OK, clientCore_ReportProperty_user_context);

            //assert
            ASSERT_ARE_NOT_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result, "Failure in test %lu", (unsigned long)i);
        }
    }

    //cleanup
    free(clientCore_ReportProperty_user_context);
    clientCore_ReportProperty_user_context = NULL;    
    test_free_bound_interface_handle(h);
    umock_c_negative_tests_deinit();
}

//
// DT_InterfaceClient_GetSdkInformation
//
static void set_expected_calls_for_DT_InterfaceClient_GetSdkInformation()
{
    STRICT_EXPECTED_CALL(DigitalTwin_Client_GetVersionString()).CallCannotFail();
}

TEST_FUNCTION(DT_InterfaceClient_GetSdkInformation_ok)
{
    //arrange
    DIGITALTWIN_CLIENT_RESULT result;
    STRING_HANDLE sdkInfo;

    set_expected_calls_for_DT_InterfaceClient_GetSdkInformation();

    //act
    result = DT_InterfaceClient_GetSdkInformation(&sdkInfo);

    //assert
    ASSERT_ARE_EQUAL(DIGITALTWIN_CLIENT_RESULT, DIGITALTWIN_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(sdkInfo);
    ASSERT_ARE_EQUAL(char_ptr, dtTestExpectedSdkInfo, STRING_c_str(sdkInfo));

    //cleanup
    STRING_delete(sdkInfo);
}

END_TEST_SUITE(dt_interface_client_ut)

