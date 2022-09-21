// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4204) /* Allows initialization of arrays with non-consts */
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
    MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
    MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
    MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
    MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
    MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
    MOCKABLE_FUNCTION(, JSON_Value_Type, json_value_get_type, const JSON_Value *, value);
    MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object *, object, const char*, name);
    MOCKABLE_FUNCTION(, size_t, json_object_get_count, const JSON_Object *, object);
    MOCKABLE_FUNCTION(, const char *, json_object_get_name, const JSON_Object *, object, size_t, index);
    MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value_at, const JSON_Object *, object, size_t, index);
    MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object *, object, const char *, name);
#ifdef __cplusplus
}
#endif
#undef ENABLE_MOCKS

#include "iothub_client_properties.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

// Status code / ack code / descriptions for when serializing properties
#define TEST_STATUS_CODE_1 200
#define TEST_STATUS_CODE_2 400
#define TEST_STATUS_CODE_3 500
#define TEST_ACK_CODE_1 1
#define TEST_ACK_CODE_2 19
#define TEST_ACK_CODE_3 77
#define TEST_DESCRIPTION_2 "2-description"
#define TEST_DESCRIPTION_3 "3-description"
#define TEST_TWIN_VER_1 17
#define TEST_TWIN_VER_2  1010

// Used to preprocessor concatenate values in later macros
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Property / value / component names used throughout the tests.
#define TEST_PROP_NAME1 "name1"
#define TEST_PROP_NAME2 "name2"
#define TEST_PROP_NAME3 "name3"
#define TEST_PROP_NAME4 "name4"
#define TEST_PROP_NAME5 "name5"
#define TEST_PROP_NAME6 "name6"
#define TEST_PROP_VALUE1 "1234"
#define TEST_PROP_VALUE2 "\"value2\""
#define TEST_PROP_VALUE3 "{\"embeddedJSON\":123}"
#define TEST_PROP_VALUE4 "4321"
#define TEST_PROP_VALUE5 "\"value5\""
#define TEST_PROP_VALUE6 "{\"embeddedJSON\":321}"
#define TEST_COMPONENT_NAME_1 "testComponent1"
#define TEST_COMPONENT_NAME_2 "testComponent2"
#define TEST_COMPONENT_NAME_3 "testComponent3"
#define TEST_COMPONENT_NAME_4 "testComponent4"
#define TEST_COMPONENT_NAME_5 "testComponent5"
#define TEST_COMPONENT_NAME_6 "testComponent6"

static const int TEST_DEFAULT_PROPERTIES_VERSION = 119;

//
// The tests make extensive use of macros to build up test JSON, both the expected results 
// of serialization APIs such as IoTHubClient_Properties_Serializer_CreateReported and the test
// data for IoTHubClient_Properties_Deserializer_Create.
//

// BUILD_JSON_NAME_VALUE creates a JSON style "name": value with required C escaping of all this.
#define BUILD_JSON_NAME_VALUE(n, v) "\""n"\":"v""

// Helpers for building up name/value pairs inside components.
#define TEST_COMPONENT_MARKER_WITH_BRACE(componentName) "{\""componentName"\":{\"__t\":\"c\""
#define TEST_COMPONENT_JSON_WITH_BRACE(componentName, expectedProperties) TEST_COMPONENT_MARKER_WITH_BRACE(componentName) "," expectedProperties "}}"
#define TEST_COMPONENT_MARKER(componentName) "\""componentName"\":{\"__t\":\"c\""
#define TEST_COMPONENT_JSON(componentName, expectedProperties) TEST_COMPONENT_MARKER(componentName) "," expectedProperties "}"

// $version field part of the desired part of JSON.  Will be preprocessor string concatenated with 
// other properties as requried for a given test.
#define TEST_JSON_TWIN_VER_1  "\"$version\":" STR(TEST_TWIN_VER_1)
#define TEST_JSON_TWIN_VER_2  "\"$version\":" STR(TEST_TWIN_VER_2)

// Helpers that build up various combinations of desired and reported JSON.  Note that the twinVersion is REQUIRED
// to be legal, so it's always included.
#define TEST_BUILD_DESIRED_ALL(nameValuePairs, twinVersion) "{ \"desired\": { " nameValuePairs "," twinVersion "} }"
#define TEST_BUILD_DESIRED_UPDATE(nameValuePairs, twinVersion) "{" nameValuePairs "," twinVersion "}"
#define TEST_BUILD_REPORTED(reportedNameValuePairs, twinVersion) "{ \"reported\": {" reportedNameValuePairs "},  \"desired\": { " twinVersion "} }"
#define TEST_BUILD_REPORTED_AND_DESIRED(reportedNameValuePairs, desiredNameValuePairs, twinVersion) "{ \"reported\": {" reportedNameValuePairs "},  \"desired\": { " desiredNameValuePairs "," twinVersion "} }"

// Test reported properties to serialize during calls to IoTHubClient_Properties_Serializer_CreateReported (valid structures).
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP1 = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, TEST_PROP_NAME1, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP2 = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, TEST_PROP_NAME2, TEST_PROP_VALUE2 };
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP3 = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, TEST_PROP_NAME3, TEST_PROP_VALUE3 };
// Test reported properties to serialize (invalid structures, IoTHubClient_Properties_Serializer_CreateReported should fail when passed these).
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP_WRONG_VERSION = { 2, TEST_PROP_NAME1, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP_NULL_NAME = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, NULL, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_PROPERTY_REPORTED TEST_REPORTED_PROP_NULL_VALUE = { IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1, TEST_PROP_NAME1, NULL };

// JSON expected to be serialized during IoTHubClient_Properties_Serializer_CreateReported tests.
#define TEST_REPORTED_PROP1_JSON_NO_BRACE BUILD_JSON_NAME_VALUE(TEST_PROP_NAME1, TEST_PROP_VALUE1)
#define TEST_REPORTED_PROP_JSON1 "{" TEST_REPORTED_PROP1_JSON_NO_BRACE "}"
#define TEST_REPORTED_PROP1_2_JSON_NO_BRACE BUILD_JSON_NAME_VALUE(TEST_PROP_NAME1, TEST_PROP_VALUE1) "," BUILD_JSON_NAME_VALUE(TEST_PROP_NAME2, TEST_PROP_VALUE2)
#define TEST_REPORTED_PROP1_2_JSON "{" TEST_REPORTED_PROP1_2_JSON_NO_BRACE "}"
#define TEST_REPORTED_PROP1_2_3_JSON_NO_BRACE BUILD_JSON_NAME_VALUE(TEST_PROP_NAME1, TEST_PROP_VALUE1) "," BUILD_JSON_NAME_VALUE(TEST_PROP_NAME2, TEST_PROP_VALUE2) "," BUILD_JSON_NAME_VALUE(TEST_PROP_NAME3, TEST_PROP_VALUE3)
#define TEST_REPORTED_PROP1_2_3_JSON  "{" TEST_REPORTED_PROP1_2_3_JSON_NO_BRACE "}"
#define TEST_REPORTED_PROP1_JSON_COMPONENT1 TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_REPORTED_PROP1_JSON_NO_BRACE)
#define TEST_REPORTED_PROP1_2_JSON_COMPONENT1 TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_REPORTED_PROP1_2_JSON_NO_BRACE)
#define TEST_REPORTED_PROP1_2_3_JSON_COMPONENT1 TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_REPORTED_PROP1_2_3_JSON_NO_BRACE)

// Test reported properties to serialize during calls to IoTHubClient_Properties_Serializer_CreateWritableResponse (valid structures).
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_PROP1 = { IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME1, TEST_PROP_VALUE1, TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_PROP2 = { IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME2, TEST_PROP_VALUE2, TEST_STATUS_CODE_2, TEST_ACK_CODE_2, TEST_DESCRIPTION_2 };
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_PROP3 = { IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME3, TEST_PROP_VALUE3, TEST_STATUS_CODE_3, TEST_ACK_CODE_3, TEST_DESCRIPTION_3 };
// Test reported properties to serialize (invalid structures, IoTHubClient_Properties_Serializer_CreateWritableResponse should fail when passed these).
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_WRONG_VERSION = { 2, TEST_PROP_NAME1, TEST_PROP_VALUE1, TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL};
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_PROP_NULL_NAME =  { IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1, NULL, TEST_PROP_VALUE1, TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE TEST_WRITABLE_PROP_NULL_VALUE = { IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME1, NULL, TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };

// Helpers to build expected JSON when calling IoTHubClient_Properties_Serializer_CreateWritableResponse.
#define BUILD_EXPECTED_WRITABLE_JSON(name, val, code, version) "\""name"\":{\"value\":"val",\"ac\":" #code ",\"av\":" #version "}"
#define BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(name, val, code, version, description) "\""name"\":{\"value\":"val",\"ac\":" #code ",\"av\":" #version ",\"ad\":\"" description "\"}"

// JSON expected to be serialized during IoTHubClient_Properties_Serializer_CreateWritableResponse tests.
#define TEST_WRITABLE_PROP1_JSON_NO_BRACE BUILD_EXPECTED_WRITABLE_JSON(TEST_PROP_NAME1, TEST_PROP_VALUE1, 200, 1)
#define TEST_WRITABLE_PROP1_JSON "{" TEST_WRITABLE_PROP1_JSON_NO_BRACE "}"
#define TEST_WRITABLE_PROP2_JSON_NO_BRACE BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(TEST_PROP_NAME2, TEST_PROP_VALUE2, 400, 19, TEST_DESCRIPTION_2)
#define TEST_WRITABLE_PROP2_JSON "{" TEST_WRITABLE_PROP2_JSON_NO_BRACE "}"
#define TEST_WRITABLE_PROP3_JSON_NO_BRACE BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(TEST_PROP_NAME3, TEST_PROP_VALUE3, 500, 77, TEST_DESCRIPTION_3)
#define TEST_WRITABLE_PROP1_2_JSON_NO_BRACE TEST_WRITABLE_PROP1_JSON_NO_BRACE "," TEST_WRITABLE_PROP2_JSON_NO_BRACE
#define TEST_WRITABLE_PROP1_2_JSON "{" TEST_WRITABLE_PROP1_2_JSON_NO_BRACE "}"
#define TEST_WRITABLE_PROP1_2_3_JSON_NO_BRACE TEST_WRITABLE_PROP1_JSON_NO_BRACE "," TEST_WRITABLE_PROP2_JSON_NO_BRACE "," TEST_WRITABLE_PROP3_JSON_NO_BRACE
#define TEST_WRITABLE_PROP1_2_3_JSON "{" TEST_WRITABLE_PROP1_2_3_JSON_NO_BRACE "}"
#define TEST_WRITABLE_PROP1_COMPONENT1_JSON TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_WRITABLE_PROP1_JSON_NO_BRACE)
#define TEST_WRITABLE_PROP2_COMPONENT1_JSON TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_WRITABLE_PROP2_JSON_NO_BRACE)
#define TEST_WRITABLE_PROP1_2_COMPONENT1_JSON TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_WRITABLE_PROP1_2_JSON_NO_BRACE)
#define TEST_WRITABLE_PROP1_2_3_COMPONENT1_JSON TEST_COMPONENT_JSON_WITH_BRACE(TEST_COMPONENT_NAME_1, TEST_WRITABLE_PROP1_2_3_JSON_NO_BRACE)

// Expected results deserialization tests.  The componentName is alway NULL in the test data below.  Tests that 
// expect a component to be set will make a copy of the needed structure(s) and then set 
// IOTHUB_CLIENT_PROPERTY_PARSED::componentName in the copied version.
static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY1 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME1,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE1 },
    (sizeof(TEST_PROP_VALUE1) / sizeof(TEST_PROP_VALUE1[0]) - 1)
};

static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY2 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME2,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE2 },
    (sizeof(TEST_PROP_VALUE2) / sizeof(TEST_PROP_VALUE2[0]) - 1)
};

static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY3 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME3,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE3 },
    (sizeof(TEST_PROP_VALUE3) / sizeof(TEST_PROP_VALUE3[0]) - 1)
};

static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY4 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT,
    NULL,
    TEST_PROP_NAME4,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE4 },
    (sizeof(TEST_PROP_VALUE4) / sizeof(TEST_PROP_VALUE4[0]) - 1)
};

static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY5 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT,
    NULL,
    TEST_PROP_NAME5,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE5 },
    (sizeof(TEST_PROP_VALUE5) / sizeof(TEST_PROP_VALUE5[0]) - 1)
};

static const IOTHUB_CLIENT_PROPERTY_PARSED TEST_EXPECTED_PROPERTY6 = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT,
    NULL,
    TEST_PROP_NAME6,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE6 },
    (sizeof(TEST_PROP_VALUE6) / sizeof(TEST_PROP_VALUE6[0]) - 1)
};

// For error cases, make sure IoTHubClient_Properties_Deserializer_GetNext does not change any members
// of the passed in structure
static IOTHUB_CLIENT_PROPERTY_PARSED unfilledProperty = {
    IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1,
    0,
    NULL,
    NULL,
    0,
    { .str = NULL },
    0
};

//
// JSON to be used as input during tests to IoTHubClient_Properties_Deserializer_Create/IoTHubClient_Properties_Deserializer_GetNext.
// 
// Builds up the most common name / value pairs so they're more convenient for later use.  
// For instance, TEST_JSON_NAME_VALUE1, after preprocessing, turns into "name1":1234
#define TEST_JSON_NAME_VALUE1 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME1, TEST_PROP_VALUE1)
#define TEST_JSON_NAME_VALUE2 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME2, TEST_PROP_VALUE2)
#define TEST_JSON_NAME_VALUE3 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME3, TEST_PROP_VALUE3)
#define TEST_JSON_NAME_VALUE4 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME4, TEST_PROP_VALUE4)
#define TEST_JSON_NAME_VALUE5 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME5, TEST_PROP_VALUE5)
#define TEST_JSON_NAME_VALUE6 BUILD_JSON_NAME_VALUE(TEST_PROP_NAME6, TEST_PROP_VALUE6)

// Concatenate more than one name/value pair together.  Used for convenience to keep the lines
// where these are used from becoming overly long.  For instance, TEST_JSON_NAME_VALUE1_2 becomes
// "name1":1234,"name2":"value2"
#define TEST_JSON_NAME_VALUE1_2 TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2
#define TEST_JSON_NAME_VALUE1_2_3 TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_NAME_VALUE3
#define TEST_JSON_NAME_VALUE4_5 TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5
#define TEST_JSON_NAME_VALUE4_5_6 TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5 "," TEST_JSON_NAME_VALUE6

// Another series of helpers, again to keep lines of the actual JSON from becoming unwieldy.
// TEST_JSON_COMPONENT1_NAME_VALUE1 for example will end up creatin a JSON block with contents
// "testComponent1":{"__t":"c","name1":1234}
#define TEST_JSON_COMPONENT1_NAME_VALUE1 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, TEST_JSON_NAME_VALUE1)
#define TEST_JSON_COMPONENT2_NAME_VALUE2 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_2, TEST_JSON_NAME_VALUE2)
#define TEST_JSON_COMPONENT3_NAME_VALUE3 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_3, TEST_JSON_NAME_VALUE3)
#define TEST_JSON_COMPONENT4_NAME_VALUE4 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_4, TEST_JSON_NAME_VALUE4)
#define TEST_JSON_COMPONENT5_NAME_VALUE5 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_5, TEST_JSON_NAME_VALUE5)
#define TEST_JSON_COMPONENT6_NAME_VALUE6 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_6, TEST_JSON_NAME_VALUE6)
#define TEST_JSON_COMPONENT1_NAME_VALUE1_2 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, TEST_JSON_NAME_VALUE1_2)
#define TEST_JSON_COMPONENT1_NAME_VALUE1_2_3  TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, TEST_JSON_NAME_VALUE1_2_3)
#define TEST_JSON_COMPONENT1_NAME_VALUE4 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_4, TEST_JSON_NAME_VALUE4)
#define TEST_JSON_COMPONENT1_NAME_VALUE4_5 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_4, TEST_JSON_NAME_VALUE4_5)
#define TEST_JSON_COMPONENT1_NAME_VALUE4_5_6 TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_4, TEST_JSON_NAME_VALUE4_5_6)

// Build up the actual JSON for IoTHubClient_Properties_Deserializer_Create/IoTHubClient_Properties_Deserializer_GetNext tests.
static unsigned const char TEST_JSON_ONE_PROPERTY_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_NAME_VALUE1, TEST_JSON_TWIN_VER_1);
static const size_t TEST_JSON_ONE_PROPERTY_ALL_LEN = sizeof(TEST_JSON_ONE_PROPERTY_ALL) - 1;
static unsigned const char TEST_JSON_ONE_PROPERTY_WRITABLE[] = TEST_BUILD_DESIRED_UPDATE(TEST_JSON_NAME_VALUE1, TEST_JSON_TWIN_VER_2);
static unsigned const char TEST_JSON_TWO_PROPERTIES_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_NAME_VALUE1_2, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_PROPERTIES_WRITABLE[] = TEST_BUILD_DESIRED_UPDATE(TEST_JSON_NAME_VALUE1_2, TEST_JSON_TWIN_VER_2);
static unsigned const char TEST_JSON_THREE_PROPERTIES_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_NAME_VALUE1_2_3, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_PROPERTIES_WRITABLE[] = TEST_BUILD_DESIRED_UPDATE(TEST_JSON_NAME_VALUE1_2_3, TEST_JSON_TWIN_VER_2);
static unsigned const char TEST_JSON_ONE_REPORTED_PROPERTY_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_NAME_VALUE4, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_REPORTED_PROPERTIES_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_NAME_VALUE4_5, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_REPORTED_PROPERTIES_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_NAME_VALUE4_5_6, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL[] = TEST_BUILD_REPORTED_AND_DESIRED(TEST_JSON_NAME_VALUE4, TEST_JSON_NAME_VALUE1, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL[] =TEST_BUILD_REPORTED_AND_DESIRED(TEST_JSON_NAME_VALUE4_5, TEST_JSON_NAME_VALUE1_2, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL[] = TEST_BUILD_REPORTED_AND_DESIRED(TEST_JSON_NAME_VALUE4_5_6, TEST_JSON_NAME_VALUE1_2_3, TEST_JSON_TWIN_VER_1);

static unsigned const char TEST_JSON_ONE_PROPERTY_COMPONENT_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_COMPONENT1_NAME_VALUE1, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_ONE_PROPERTY_COMPONENT_WRITABLE[] = TEST_BUILD_DESIRED_UPDATE(TEST_JSON_COMPONENT1_NAME_VALUE1, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_PROPERTIES_COMPONENT_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_COMPONENT1_NAME_VALUE1_2, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_PROPERTIES_COMPONENT_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_COMPONENT1_NAME_VALUE1_2_3, TEST_JSON_TWIN_VER_1);

static unsigned const char TEST_JSON_ONE_REPORTED_PROPERTY_COMPONENT_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_COMPONENT1_NAME_VALUE4, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_REPORTED_PROPERTIES_COMPONENT_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_COMPONENT1_NAME_VALUE4_5, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_REPORTED_PROPERTIES_COMPONENT_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_COMPONENT1_NAME_VALUE4_5_6, TEST_JSON_TWIN_VER_1);

static unsigned const char TEST_JSON_TWO_UDPATE_PROPERTIES_TWO_COMPONENTS_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_COMPONENT1_NAME_VALUE1 "," TEST_JSON_COMPONENT2_NAME_VALUE2, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_TWO_UDPATE_PROPERTIES_THREE_COMPONENTS_ALL[] = TEST_BUILD_DESIRED_ALL(TEST_JSON_COMPONENT1_NAME_VALUE1 "," TEST_JSON_COMPONENT2_NAME_VALUE2 "," TEST_JSON_COMPONENT3_NAME_VALUE3, TEST_JSON_TWIN_VER_1);

static unsigned const char TEST_JSON_TWO_REPORTED_PROPERTIES_TWO_COMPONENTS_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_COMPONENT4_NAME_VALUE4 "," TEST_JSON_COMPONENT5_NAME_VALUE5, TEST_JSON_TWIN_VER_1);
static unsigned const char TEST_JSON_THREE_REPORTED_PROPERTIES_THREE_COMPONENTS_ALL[] = TEST_BUILD_REPORTED(TEST_JSON_COMPONENT4_NAME_VALUE4 "," TEST_JSON_COMPONENT5_NAME_VALUE5 "," TEST_JSON_COMPONENT6_NAME_VALUE6, TEST_JSON_TWIN_VER_1);

#define TEST_JSON_ALL_REPORTED TEST_JSON_COMPONENT4_NAME_VALUE4 "," TEST_JSON_COMPONENT5_NAME_VALUE5 "," TEST_JSON_COMPONENT6_NAME_VALUE6
#define TEST_JSON_ALL_WRITABLE TEST_JSON_COMPONENT1_NAME_VALUE1 "," TEST_JSON_COMPONENT2_NAME_VALUE2 "," TEST_JSON_COMPONENT3_NAME_VALUE3

// This tests 3 reported properties, 3 writable, each in a separate component.
static unsigned const char TEST_JSON_THREE_WRITABLE_REPORTED_IN_SEPARATE_COMPONENTS[] = TEST_BUILD_REPORTED_AND_DESIRED(TEST_JSON_ALL_REPORTED, TEST_JSON_ALL_WRITABLE, TEST_JSON_TWIN_VER_1);

// Invalid JSON.  IoTHubClient_Properties_Deserializer_Create will fail trying to deserialize this.
static unsigned const char TEST_INVALID_JSON[] = "}{-not-valid";
// Legal JSON but no $version.  IoTHubClient_Properties_Deserializer_Create will fail trying to deserialize this.
static unsigned const char TEST_JSON_NO_VERSION[] = "44";
// Legal JSON including $version, but for an "all properties" json its missing the desired.  IoTHubClient_Properties_Deserializer_Create 
// will succeed but IoTHubClient_Properties_Deserializer_GetNext won't have anything to enumerate.
static unsigned const char TEST_JSON_NO_DESIRED[] = "{ " TEST_JSON_TWIN_VER_1 " }";

BEGIN_TEST_SUITE(iothub_client_properties_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    umock_c_init(on_umock_c_error);
    
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_realloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);


    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, real_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_free_serialized_string, real_json_free_serialized_string);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_object, real_json_value_get_object );
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value, real_json_object_get_value );
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, real_json_value_free);
    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_type, real_json_value_get_type);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_type, JSONError);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_object, real_json_object_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_count, real_json_object_get_count);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_name, real_json_object_get_name);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value_at, real_json_object_get_value_at);
    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_string, real_json_object_get_string);

    REGISTER_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

//
// IoTHubClient_Properties_Serializer_CreateReported tests
//
static void set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported(void)
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_NULL_properties)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(NULL, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_NULL_serializedProperties)
{
    // arrange
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP1, 1, NULL, NULL, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_NULL_serializedPropertiesLength)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP1, 1, NULL, &serializedProperties, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_wrong_struct_version)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP_WRONG_VERSION, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_NULL_propname)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    IOTHUB_CLIENT_PROPERTY_REPORTED testReportedNullNameThirdIndex[3] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2, TEST_REPORTED_PROP_NULL_NAME};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedNullNameThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_NULL_propvalue)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedNullValueThirdIndex[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2, TEST_REPORTED_PROP_NULL_VALUE};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedNullValueThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_one_property_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP1, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP_JSON1, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP_JSON1), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_two_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedTwoProperties[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2};

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedTwoProperties, 2, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP1_2_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP1_2_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_three_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedTwoProperties[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2, TEST_REPORTED_PROP3};

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedTwoProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP1_2_3_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP1_2_3_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_one_property_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP1_JSON_COMPONENT1, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP1_JSON_COMPONENT1), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_two_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedTwoProperties[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2};

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedTwoProperties, 2, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP1_2_JSON_COMPONENT1, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP1_2_JSON_COMPONENT1), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_three_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedThreeProperties[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2, TEST_REPORTED_PROP3};

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_REPORTED_PROP1_2_3_JSON_COMPONENT1, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_REPORTED_PROP1_2_3_JSON_COMPONENT1), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateReported_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_REPORTED testReportedThreeProperties[] = { TEST_REPORTED_PROP1, TEST_REPORTED_PROP2, TEST_REPORTED_PROP3};

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateReported();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);
            
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(testReportedThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(serializedProperties);
            ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
        }
    }
}

//
// IoTHubClient_Properties_Serializer_CreateWritableResponse tests
// 
static void set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse(void)
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_NULL_properties)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(NULL, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_NULL_serializedProperties)
{
    // arrange
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP1, 1, NULL, NULL, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_NULL_serializedPropertiesLength)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP1, 1, NULL, &serializedProperties, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_wrong_struct_version)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_WRONG_VERSION, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_NULL_propname)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableNullValueThirdIndex[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP_NULL_NAME };
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableNullValueThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_CreateWritableResponse_NULL_propvalue)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableNullValueThirdIndex[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP_NULL_VALUE };
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableNullValueThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP1, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_description_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP2, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP2_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP2_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_two_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableTwoProperties[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP2 };

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableTwoProperties, 2, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_2_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_2_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_three_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableThreeProperties[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP2, TEST_WRITABLE_PROP3 };

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableThreeProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_2_3_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_2_3_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_COMPONENT1_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_COMPONENT1_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_description_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(&TEST_WRITABLE_PROP2, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP2_COMPONENT1_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP2_COMPONENT1_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_two_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableTwoProperties[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP2 };

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableTwoProperties, 2, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_2_COMPONENT1_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_2_COMPONENT1_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_three_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableThreeProperties[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP2, TEST_WRITABLE_PROP3 };

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_WRITABLE_PROP1_2_3_COMPONENT1_JSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(TEST_WRITABLE_PROP1_2_3_COMPONENT1_JSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE testWritableThreeProperties[] = { TEST_WRITABLE_PROP1, TEST_WRITABLE_PROP2, TEST_WRITABLE_PROP3 };

    set_expected_calls_for_IoTHubClient_Properties_Serializer_CreateWritableResponse();
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);
            
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateWritableResponse(testWritableThreeProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(serializedProperties);
            ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
        }
    }
}

//
// IoTHubClient_Properties_Serializer_Destroy tests
// 
static void set_expected_calls_for_IoTHubClient_Properties_Serializer_Destroy(void)
{
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_Destroy_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Serializer_CreateReported(&TEST_REPORTED_PROP1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(serializedPropertiesLength);

    umock_c_reset_all_calls();
    set_expected_calls_for_IoTHubClient_Properties_Serializer_Destroy();

    // act
    IoTHubClient_Properties_Serializer_Destroy(serializedProperties);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Serializer_Destroy_null)
{
    // act
    IoTHubClient_Properties_Serializer_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// IoTHubClient_Properties_Deserializer_Create tests
//
static void set_expected_calls_for_IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType)
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    if (payloadType == IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    }
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* payload)
{
    umock_c_reset_all_calls();

    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;
    size_t payloadLength = strlen((const char*)payload);
    set_expected_calls_for_IoTHubClient_Properties_Deserializer_Create(payloadType);

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(payloadType, payload, payloadLength, &h);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();

    return h;
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_invalid_payload_type)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create((IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE)1234, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN,  &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_NULL_payload)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, NULL, TEST_JSON_ONE_PROPERTY_ALL_LEN, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_zero_payloadLength)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, 0, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_NULL_handle)
{
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_all_success)
{
    // act
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_all_with_components_success)
{
    // act
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_writable_success)
{
    // act
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_no_properties_success)
{
    // act
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_NO_DESIRED);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_writable_with_components_success)
{
    // act
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

static void test_IoTHubClient_Properties_Deserializer_Create_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* invalidJson)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;
    size_t invalidJsonLen = strlen((const char*)invalidJson);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(payloadType, invalidJson, invalidJsonLen, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_writable_invalid_JSON_fail)
{
    test_IoTHubClient_Properties_Deserializer_Create_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_INVALID_JSON);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_writable_missing_version_fail)
{
    test_IoTHubClient_Properties_Deserializer_Create_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_NO_VERSION);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Create_fail)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = NULL;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL);
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_Create(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, &h);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(h);
        }
    }
}

//
// IoTHubClient_Properties_Deserializer_GetVersion tests
//
TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetVersion_NULL_handle)
{
    // arrange
    int propertiesVersion = TEST_DEFAULT_PROPERTIES_VERSION;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetVersion(NULL, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    // Make sure test didn't inadvertently change value of properties version on failure
    ASSERT_ARE_EQUAL(int, TEST_DEFAULT_PROPERTIES_VERSION, propertiesVersion);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetVersion_NULL_version)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE);
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetVersion(h, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);    
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetVersion_writable_update_success)
{
    // arrange
    int propertiesVersion;
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE);
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetVersion(h, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, TEST_TWIN_VER_2, propertiesVersion);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetVersion_full_twin_success)
{
    // arrange
    int propertiesVersion;
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetVersion(h, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, TEST_TWIN_VER_1, propertiesVersion);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

//
// IoTHubClient_Properties_Deserializer_GetNext tests
//
static void ResetTestProperty(IOTHUB_CLIENT_PROPERTY_PARSED* property)
{
    memset(property, 0, sizeof(*property));
    property->structVersion = IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1;
}

static void CompareProperties(const IOTHUB_CLIENT_PROPERTY_PARSED* expectedProperty, IOTHUB_CLIENT_PROPERTY_PARSED* actualProperty)
{
    ASSERT_ARE_EQUAL(int, expectedProperty->structVersion, actualProperty->structVersion);
    ASSERT_ARE_EQUAL(int, expectedProperty->propertyType, actualProperty->propertyType);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->componentName, actualProperty->componentName);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->name, actualProperty->name);
    ASSERT_ARE_EQUAL(int, expectedProperty->valueType, actualProperty->valueType);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->value.str, actualProperty->value.str);
    ASSERT_ARE_EQUAL(int, expectedProperty->valueLength, actualProperty->valueLength);
}

static void TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* payload, const IOTHUB_CLIENT_PROPERTY_PARSED* expectedProperties, size_t numExpectedProperties)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(payloadType, payload);
    size_t numPropertiesVisited = 0;

    // act|assert

    while (true)
    {
        IOTHUB_CLIENT_PROPERTY_PARSED property;
        bool propertySpecified;
        ResetTestProperty(&property);

        IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, &property, &propertySpecified);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

        if (numPropertiesVisited == numExpectedProperties)
        {
            // This is our last time enumerating number of properties we expect.  We should still
            // get success from caller but propertySpecified better be set FALSE so caller knows to stop.
            ASSERT_IS_FALSE(propertySpecified);
            break;
        }
        ASSERT_IS_TRUE(propertySpecified);

        CompareProperties(expectedProperties + numPropertiesVisited, &property);

        IoTHubClient_Properties_DeserializerProperty_Destroy(&property);
        numPropertiesVisited++;
    }

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_NULL_handle)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_PARSED property;
    ResetTestProperty(&property);
    bool propertySpecified = false;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(NULL, &property, &propertySpecified);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_FALSE(propertySpecified);
    CompareProperties(&unfilledProperty, &property);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_NULL_property)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    bool propertySpecified = false;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, NULL, &propertySpecified);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_FALSE(propertySpecified);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_NULL_propertySpecified)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    IOTHUB_CLIENT_PROPERTY_PARSED property;
    ResetTestProperty(&property);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, &property, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    CompareProperties(&unfilledProperty, &property);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_wrong_struct_version)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    IOTHUB_CLIENT_PROPERTY_PARSED wrongVersion;
    IOTHUB_CLIENT_PROPERTY_PARSED property;
    bool propertySpecified = false;

    memset(&wrongVersion, 0, sizeof(wrongVersion));
    wrongVersion.structVersion = 22;
    memcpy(&property, &wrongVersion, sizeof(property));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, &property, &propertySpecified);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    // Make sure IoTHubClient_Properties_Deserializer_GetNext didn't modify property in error case
    ASSERT_IS_TRUE(0 == memcmp(&wrongVersion, &property, sizeof(property)));
    ASSERT_IS_FALSE(propertySpecified);

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);
}

// Test only to just skip STRICT_EXPECT work
TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_all_one_property_success)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_writable_one_property_success)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_no_properties_success)
{
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_NO_DESIRED, NULL, 0);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_all_two_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_PROPERTIES_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_writable_two_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_TWO_PROPERTIES_WRITABLE, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_all_three_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_PROPERTIES_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_writable_three_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_THREE_PROPERTIES_WRITABLE, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_reported_one_property)
{
     IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_REPORTED_PROPERTY_ALL, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_reported_two_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_PROPERTIES_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_reported_three_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5, TEST_EXPECTED_PROPERTY6 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_PROPERTIES_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_one_reported_update_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY4  };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_two_reported_update_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL, expectedPropList, 4);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_reported_update_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3, TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5, TEST_EXPECTED_PROPERTY6 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL, expectedPropList, 6);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_one_writable_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_COMPONENT_ALL, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_one_writable_update_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_COMPONENT_WRITABLE, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_two_writable_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2  };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_1;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_PROPERTIES_COMPONENT_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_writable_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3  };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[2].componentName = TEST_COMPONENT_NAME_1;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_PROPERTIES_COMPONENT_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_one_reported_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_4;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_REPORTED_PROPERTY_COMPONENT_ALL, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_two_reported_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5  };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_4;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_PROPERTIES_COMPONENT_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_reported_all_component)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5, TEST_EXPECTED_PROPERTY6  };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[2].componentName = TEST_COMPONENT_NAME_4;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_PROPERTIES_COMPONENT_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_two_components_writable_all)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_2;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_UDPATE_PROPERTIES_TWO_COMPONENTS_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_components_writable_all)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_2;
    expectedPropList[2].componentName = TEST_COMPONENT_NAME_3;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_UDPATE_PROPERTIES_THREE_COMPONENTS_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_two_components_reported)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_5;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_PROPERTIES_TWO_COMPONENTS_ALL, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_components_reported)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5, TEST_EXPECTED_PROPERTY6 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_5;
    expectedPropList[2].componentName = TEST_COMPONENT_NAME_6;
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_PROPERTIES_THREE_COMPONENTS_ALL, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_writable_and_reported_properties)
{
    IOTHUB_CLIENT_PROPERTY_PARSED expectedPropList[] = { TEST_EXPECTED_PROPERTY1, TEST_EXPECTED_PROPERTY2, TEST_EXPECTED_PROPERTY3, TEST_EXPECTED_PROPERTY4, TEST_EXPECTED_PROPERTY5, TEST_EXPECTED_PROPERTY6 };
    expectedPropList[0].componentName = TEST_COMPONENT_NAME_1;
    expectedPropList[1].componentName = TEST_COMPONENT_NAME_2;
    expectedPropList[2].componentName = TEST_COMPONENT_NAME_3;
    expectedPropList[3].componentName = TEST_COMPONENT_NAME_4;
    expectedPropList[4].componentName = TEST_COMPONENT_NAME_5;
    expectedPropList[5].componentName = TEST_COMPONENT_NAME_6;

    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_WRITABLE_REPORTED_IN_SEPARATE_COMPONENTS, expectedPropList, 6);
}

static void set_expected_calls_for_IoTHubClient_Properties_Deserializer_GetNext_fail_tests(void)
{
    STRICT_EXPECTED_CALL(json_object_get_count(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_name(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_value_at(IGNORED_PTR_ARG, IGNORED_NUM_ARG)).CallCannotFail(); 
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_name(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();    
    STRICT_EXPECTED_CALL(json_object_get_value_at(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_count(IGNORED_PTR_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_name(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_name(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_object_get_value_at(IGNORED_PTR_ARG,IGNORED_NUM_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_GetNext_three_writable_and_reported_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IoTHubClient_Properties_Deserializer_GetNext_fail_tests();
    // We take the initial snapshot to get the count of tests, but then immediately de-init.  We don't follow
    // the standard convention of other _fail() tests here.  Because we do an iterator approach, it makes
    // changes to the state of the underlying reader.  So we create a new handle on each pass.
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    
    for (size_t index = 0; index < count; index++)
    {
        if (! umock_c_negative_tests_can_call_fail(index))
        {
            continue;
        }

        IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_COMPONENT_ALL);

        // Reset up the negative test framework for this specific run.  Needed inside this for() loop
        // because of all the state changes GetNextProperty causes.
        umock_c_negative_tests_deinit();
        negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);
        set_expected_calls_for_IoTHubClient_Properties_Deserializer_GetNext_fail_tests();
        umock_c_negative_tests_snapshot();

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);
   
        IOTHUB_CLIENT_PROPERTY_PARSED property;
        bool propertySpecified = false;
        ResetTestProperty(&property);

        IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, &property, &propertySpecified);

        ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Unexpected success on test run  %lu", (unsigned long)index);
        ASSERT_IS_FALSE(propertySpecified, "Unexpected property=TRUE on test run %lu", (unsigned long)index);

        IoTHubClient_Properties_Deserializer_Destroy(h);
    }
}

//
// IoTHubClient_Properties_DeserializerProperty_Destroy tests
//
static void set_expected_calls_for_IoTHubClient_Properties_DeserializerProperty_Destroy(void)
{
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_DeserializerProperty_Destroy_ok)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    IOTHUB_CLIENT_PROPERTY_PARSED property;
    bool propertySpecified;
    ResetTestProperty(&property);

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Properties_Deserializer_GetNext(h, &property, &propertySpecified);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_TRUE(propertySpecified);
    umock_c_reset_all_calls();

    set_expected_calls_for_IoTHubClient_Properties_DeserializerProperty_Destroy();

    // act
    IoTHubClient_Properties_DeserializerProperty_Destroy(&property);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Properties_Deserializer_Destroy(h);

}


TEST_FUNCTION(IoTHubClient_Properties_DeserializerProperty_Destroy_null)
{
    // act
    IoTHubClient_Properties_DeserializerProperty_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// IoTHubClient_Properties_Deserializer_Destroy tests
//
static void set_expected_calls_for_IoTHubClient_Properties_Deserializer_Destroy(void)
{
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Destroy_success)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    set_expected_calls_for_IoTHubClient_Properties_Deserializer_Destroy();

    // act
    IoTHubClient_Properties_Deserializer_Destroy(h);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Destroy_multiple_components_success)
{
    // arrange
    IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE h = TestAllocatePropertiesReader(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL);
    set_expected_calls_for_IoTHubClient_Properties_Deserializer_Destroy();

    // act
    IoTHubClient_Properties_Deserializer_Destroy(h);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(IoTHubClient_Properties_Deserializer_Destroy_null)
{
    // act
    IoTHubClient_Properties_Deserializer_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(iothub_client_properties_ut)
