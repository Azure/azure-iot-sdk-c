
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "parson.h"

/*don't want any of the below mocks, so preemptively #include the header will prohibit the mocks from being created*/
#include "azure_c_shared_utility/crt_abstractions.h"
#include "commanddecoder.h"

#define ENABLE_MOCKS
#include "iothub_client.h"
#include "iothub_client_ll.h"
#undef ENABLE_MOCKS

#include "serializer_devicetwin.h"

static TEST_MUTEX_HANDLE g_testByTest;

TEST_DEFINE_ENUM_TYPE(SERIALIZER_RESULT, SERIALIZER_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(CODEFIRST_RESULT, CODEFIRST_RESULT_VALUES);

/*returns 0 is the two jsons are not equal, any other value means they are equal*/
/*typically "left" is the output of SERIALIZE... and right is hand-coded JSON*/
static bool areTwoJsonsEqual(const unsigned char* left, size_t leftSize, const char* right)
{
    bool result;

    char* cloneOfLeft = (char*)malloc(leftSize + 1); /*because of out SERIALIZE... there is a byte array that is NOT '\0' terminated*/
    ASSERT_IS_NOT_NULL(cloneOfLeft);

    (void)memcpy(cloneOfLeft, left, leftSize);
    cloneOfLeft[leftSize] = '\0';

    JSON_Value* actualJson = json_parse_string((char*)cloneOfLeft);
    ASSERT_IS_NOT_NULL(actualJson);
    JSON_Value* expectedJson = json_parse_string(right);
    ASSERT_IS_NOT_NULL(expectedJson);

    result = (json_value_equals(expectedJson, actualJson) != 0);

    json_value_free(expectedJson);
    json_value_free(actualJson);
    free(cloneOfLeft);

    return result;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_NAMESPACE(basic15)

DECLARE_DEVICETWIN_MODEL(basicModel_WithData15,
    WITH_DESIRED_PROPERTY(double, with_desired_property_double15, on_desired_property_double15),
    WITH_DESIRED_PROPERTY(int, with_desired_property_int15, on_desired_property_int15),
    WITH_DESIRED_PROPERTY(float, with_desired_property_float15, on_desired_property_float15),
    WITH_DESIRED_PROPERTY(long, with_desired_property_long15, on_desired_property_long15),
    WITH_DESIRED_PROPERTY(int8_t, with_desired_property_sint8_t15, on_desired_property_sint8_t15),
    WITH_DESIRED_PROPERTY(uint8_t, with_desired_property_uint8_t15, on_desired_property_uint8_t15),
    WITH_DESIRED_PROPERTY(int16_t, with_desired_property_int16_t15, on_desired_property_int16_t15),
    WITH_DESIRED_PROPERTY(int32_t, with_desired_property_int32_t15, on_desired_property_int32_t15),
    WITH_DESIRED_PROPERTY(int64_t, with_desired_property_int64_t15, on_desired_property_int64_t15),
    WITH_DESIRED_PROPERTY(bool, with_desired_property_bool15, on_desired_property_bool15),
    WITH_DESIRED_PROPERTY(ascii_char_ptr, with_desired_property_ascii_char_ptr15, on_desired_property_ascii_char_ptr15),
    WITH_DESIRED_PROPERTY(ascii_char_ptr_no_quotes, with_desired_property_ascii_char_ptr_no_quotes15, on_desired_property_ascii_char_ptr_no_quotes15),
    WITH_DESIRED_PROPERTY(EDM_DATE_TIME_OFFSET, with_desired_property_EdmDateTimeOffset15, on_desired_property_EdmDateTimeOffset15),
    WITH_DESIRED_PROPERTY(EDM_GUID, with_desired_property_EdmGuid15, on_desired_property_EdmGuid15),
    WITH_DESIRED_PROPERTY(EDM_BINARY, with_desired_property_EdmBinary15, on_desired_property_EdmBinary15),
    WITH_METHOD(methodA, int, a, int, b),
    WITH_REPORTED_PROPERTY(int, with_reported_property_int15)
)
END_NAMESPACE(basic15)

METHODRETURN_HANDLE my_methodA(basicModel_WithData15 *model, int a, int b)
{
    (void)model;
    METHODRETURN_HANDLE result = MethodReturn_Create(a + b, "\"ANSWER\"");
    return result;
}

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_desired_property_double15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_int15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_float15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_long15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_sint8_t15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_uint8_t15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_int16_t15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_int32_t15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_int64_t15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_bool15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_ascii_char_ptr15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_ascii_char_ptr_no_quotes15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_EdmDateTimeOffset15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_EdmGuid15, void*, v);
    MOCKABLE_FUNCTION(, void, on_desired_property_EdmBinary15, void*, v);
    MOCKABLE_FUNCTION(, METHODRETURN_HANDLE, methodA, basicModel_WithData15 *, model, int, a, int, b);
    MOCKABLE_FUNCTION(, void, sendReportedStateCallback, int, status_code, void*, userContextCallback);
#undef ENABLE_MOCKS

#define TEST_IOTHUB_CLIENT_HANDLE ((IOTHUB_CLIENT_HANDLE)0x42)
#define TEST_IOTHUB_CLIENT_LL_HANDLE ((IOTHUB_CLIENT_LL_HANDLE)0x432)

static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK g_SerializerIngest_fp = NULL;
static void* g_SerializerIngest_userContextCallback = NULL;

static IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC g_deviceMethodCallback_fp = NULL;
static void* g_deviceMethodCallback_userContextCallback = NULL;


static IOTHUB_CLIENT_RESULT my_IoTHubClient_SetDeviceTwinCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)(iotHubClientHandle);
    g_SerializerIngest_fp = deviceTwinCallback;
    g_SerializerIngest_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    (void)(iotHubClientHandle);
    g_SerializerIngest_fp = deviceTwinCallback;
    g_SerializerIngest_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_SetDeviceMethodCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    (void)(iotHubClientHandle);
    g_deviceMethodCallback_fp = deviceMethodCallback;
    g_deviceMethodCallback_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    (void)(iotHubClientHandle);
    g_deviceMethodCallback_fp = deviceMethodCallback;
    g_deviceMethodCallback_userContextCallback = userContextCallback;
    return IOTHUB_CLIENT_OK;
}

static char* g_IoTHubClient_SendReportedState_reportedState = NULL;
static size_t g_IoTHubClient_SendReportedState_size = 0;
static IOTHUB_CLIENT_RESULT my_IoTHubClient_SendReportedState(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    (void)(iotHubClientHandle);

    g_IoTHubClient_SendReportedState_reportedState = (char*)malloc(size+1);
    ASSERT_IS_NOT_NULL(g_IoTHubClient_SendReportedState_reportedState);
    (void)memcpy(g_IoTHubClient_SendReportedState_reportedState, reportedState, size);
    g_IoTHubClient_SendReportedState_reportedState[size] = '\0';

    reportedStateCallback(201, userContextCallback);
    return IOTHUB_CLIENT_OK;
}

static char* g_IoTHubClient_LL_SendReportedState_reportedState = NULL;
static size_t g_IoTHubClient_LL_SendReportedState_size = 0;
static IOTHUB_CLIENT_RESULT my_IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    (void)iotHubClientHandle;
    (void)reportedState;
    (void)size;
    g_IoTHubClient_LL_SendReportedState_reportedState = (char*)malloc(size+1);
    ASSERT_IS_NOT_NULL(g_IoTHubClient_LL_SendReportedState_reportedState);
    (void)memcpy(g_IoTHubClient_LL_SendReportedState_reportedState, reportedState, size);
    g_IoTHubClient_LL_SendReportedState_reportedState[size] = '\0';

    reportedStateCallback(201, userContextCallback);
    return IOTHUB_CLIENT_OK;
}

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

BEGIN_TEST_SUITE(serializer_dt_int)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        (void)umocktypes_bool_register_types();
        (void)umocktypes_charptr_register_types();
        (void)umocktypes_stdint_register_types();

        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_LL_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, void*);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, void*);
        REGISTER_UMOCK_ALIAS_TYPE(basicModel_WithData15, void*);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);


        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_SetDeviceTwinCallback, my_IoTHubClient_SetDeviceTwinCallback);
        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetDeviceTwinCallback, my_IoTHubClient_LL_SetDeviceTwinCallback);

        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_SetDeviceMethodCallback, my_IoTHubClient_SetDeviceMethodCallback);
        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SetDeviceMethodCallback, my_IoTHubClient_LL_SetDeviceMethodCallback);

        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_LL_SendReportedState, my_IoTHubClient_LL_SendReportedState);
        REGISTER_GLOBAL_MOCK_HOOK(IoTHubClient_SendReportedState, my_IoTHubClient_SendReportedState);

        REGISTER_GLOBAL_MOCK_HOOK(methodA, my_methodA);
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();
        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        umock_c_reset_all_calls();

        g_SerializerIngest_fp = NULL;
        g_SerializerIngest_userContextCallback = NULL;

        g_IoTHubClient_SendReportedState_reportedState = NULL;
        g_IoTHubClient_LL_SendReportedState_reportedState = NULL;

        g_IoTHubClient_SendReportedState_size = 0;
        g_IoTHubClient_LL_SendReportedState_size = 0;

    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        if (g_IoTHubClient_SendReportedState_reportedState != NULL)
        {
            free(g_IoTHubClient_SendReportedState_reportedState);
        }

        if (g_IoTHubClient_LL_SendReportedState_reportedState != NULL)
        {
            free(g_IoTHubClient_LL_SendReportedState_reportedState);
        }
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /*this test wants to see that IoTHubDeviceTwin_CreatebasicModel_WithData15 doesn't fail*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_METHODS_SUCCEEDS)
    {
        ///arrange
        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        ///act
        basicModel_WithData15* model = IoTHubDeviceTwin_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_HANDLE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(model);

        ///clean
        IoTHubDeviceTwin_DestroybasicModel_WithData15(model);

    }

    /*this test wants to see that IoTHubDeviceTwin_CreatebasicModel_WithData15 doesn't fail*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_METHODS_LL_SUCCEEDS)
    {
        ///arrange
        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        ///act
        basicModel_WithData15* model = IoTHubDeviceTwin_LL_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_LL_HANDLE);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(model);

        ///clean
        IoTHubDeviceTwin_DestroybasicModel_WithData15(model);

    }

    /*this test wants to see that IoTHubDeviceTwin_CreatebasicModel_WithData15 can receive desired properties*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_RECEIVES_DESIRED_PROPERTY_SUCCEEDS)
    {
        ///arrange

        const char* inputJsonAsString =
        "{\"desired\":{                                                                                        \
            \"$version\": 1,                                                                                   \
            \"with_desired_property_double15\" : 1.0,                                                          \
            \"with_desired_property_int15\" : 2,                                                               \
            \"with_desired_property_float15\" : 3.000000,                                                      \
            \"with_desired_property_long15\" : 4,                                                              \
            \"with_desired_property_sint8_t15\" : 5,                                                           \
            \"with_desired_property_uint8_t15\" : 6,                                                           \
            \"with_desired_property_int16_t15\" : 7,                                                           \
            \"with_desired_property_int32_t15\" : 8,                                                           \
            \"with_desired_property_int64_t15\" : 9,                                                           \
            \"with_desired_property_bool15\" : true,                                                           \
            \"with_desired_property_ascii_char_ptr15\" : \"e/leven\",                                          \
            \"with_desired_property_ascii_char_ptr_no_quotes15\" : \"twelve\",                                 \
            \"with_desired_property_EdmDateTimeOffset15\" : \"2014-06-17T08:51:23.000000000005-08:01\",        \
            \"with_desired_property_EdmGuid15\" : \"00112233-4455-6677-8899-AABBCCDDEEFF\",                    \
            \"with_desired_property_EdmBinary15\": \"MzQ1\"                                                    \
        }}";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_HANDLE);

        STRICT_EXPECTED_CALL(on_desired_property_double15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_float15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_long15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_sint8_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_uint8_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int16_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int32_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int64_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_bool15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_ascii_char_ptr15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_ascii_char_ptr_no_quotes15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmDateTimeOffset15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmGuid15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmBinary15(modelWithData));

        ///act
        g_SerializerIngest_fp(DEVICE_TWIN_UPDATE_COMPLETE, (const unsigned char*)inputJsonAsString, strlen(inputJsonAsString), modelWithData);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(modelWithData);

        ASSERT_ARE_EQUAL(double,    1.0,            modelWithData->with_desired_property_double15);
        ASSERT_ARE_EQUAL(int,       2,              modelWithData->with_desired_property_int15);
        ASSERT_ARE_EQUAL(float,     3.0,            modelWithData->with_desired_property_float15);
        ASSERT_ARE_EQUAL(long,      4,              modelWithData->with_desired_property_long15);
        ASSERT_ARE_EQUAL(int8_t,    5,              modelWithData->with_desired_property_sint8_t15);
        ASSERT_ARE_EQUAL(uint8_t,   6,              modelWithData->with_desired_property_uint8_t15);
        ASSERT_ARE_EQUAL(int16_t,   7,              modelWithData->with_desired_property_int16_t15);
        ASSERT_ARE_EQUAL(int32_t,   8,              modelWithData->with_desired_property_int32_t15);
        ASSERT_ARE_EQUAL(int64_t,   9,              modelWithData->with_desired_property_int64_t15);
        ASSERT_IS_TRUE(modelWithData->with_desired_property_bool15);
        ASSERT_ARE_EQUAL(char_ptr,  "e/leven",       modelWithData->with_desired_property_ascii_char_ptr15);
        ASSERT_ARE_EQUAL(char_ptr,  "\"twelve\"",   modelWithData->with_desired_property_ascii_char_ptr_no_quotes15);
        ASSERT_ARE_EQUAL(int,       114,            modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_year);
        ASSERT_ARE_EQUAL(int,       6-1,            modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_mon);
        ASSERT_ARE_EQUAL(int,       17,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_mday);
        ASSERT_ARE_EQUAL(int,       8,              modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_hour);
        ASSERT_ARE_EQUAL(int,       51,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_min);
        ASSERT_ARE_EQUAL(int,       23,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_sec);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.hasFractionalSecond);
        ASSERT_ARE_EQUAL(uint64_t,  5,              modelWithData->with_desired_property_EdmDateTimeOffset15.fractionalSecond);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.hasTimeZone);
        ASSERT_ARE_EQUAL(int8_t,    -8,             modelWithData->with_desired_property_EdmDateTimeOffset15.timeZoneHour);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.timeZoneMinute);
        ASSERT_ARE_EQUAL(uint8_t,   0x00,           modelWithData->with_desired_property_EdmGuid15.GUID[0]);
        ASSERT_ARE_EQUAL(uint8_t,   0x11,           modelWithData->with_desired_property_EdmGuid15.GUID[1]);
        ASSERT_ARE_EQUAL(uint8_t,   0x22,           modelWithData->with_desired_property_EdmGuid15.GUID[2]);
        ASSERT_ARE_EQUAL(uint8_t,   0x33,           modelWithData->with_desired_property_EdmGuid15.GUID[3]);
        ASSERT_ARE_EQUAL(uint8_t,   0x44,           modelWithData->with_desired_property_EdmGuid15.GUID[4]);
        ASSERT_ARE_EQUAL(uint8_t,   0x55,           modelWithData->with_desired_property_EdmGuid15.GUID[5]);
        ASSERT_ARE_EQUAL(uint8_t,   0x66,           modelWithData->with_desired_property_EdmGuid15.GUID[6]);
        ASSERT_ARE_EQUAL(uint8_t,   0x77,           modelWithData->with_desired_property_EdmGuid15.GUID[7]);
        ASSERT_ARE_EQUAL(uint8_t,   0x88,           modelWithData->with_desired_property_EdmGuid15.GUID[8]);
        ASSERT_ARE_EQUAL(uint8_t,   0x99,           modelWithData->with_desired_property_EdmGuid15.GUID[9]);
        ASSERT_ARE_EQUAL(uint8_t,   0xAA,           modelWithData->with_desired_property_EdmGuid15.GUID[10]);
        ASSERT_ARE_EQUAL(uint8_t,   0xBB,           modelWithData->with_desired_property_EdmGuid15.GUID[11]);
        ASSERT_ARE_EQUAL(uint8_t,   0xCC,           modelWithData->with_desired_property_EdmGuid15.GUID[12]);
        ASSERT_ARE_EQUAL(uint8_t,   0xDD,           modelWithData->with_desired_property_EdmGuid15.GUID[13]);
        ASSERT_ARE_EQUAL(uint8_t,   0xEE,           modelWithData->with_desired_property_EdmGuid15.GUID[14]);
        ASSERT_ARE_EQUAL(uint8_t,   0xFF,           modelWithData->with_desired_property_EdmGuid15.GUID[15]);

        ASSERT_ARE_EQUAL(size_t,     3,             modelWithData->with_desired_property_EdmBinary15.size);
        ASSERT_ARE_EQUAL(uint8_t,   '3',            modelWithData->with_desired_property_EdmBinary15.data[0]);
        ASSERT_ARE_EQUAL(uint8_t,   '4',            modelWithData->with_desired_property_EdmBinary15.data[1]);
        ASSERT_ARE_EQUAL(uint8_t,   '5',            modelWithData->with_desired_property_EdmBinary15.data[2]);

        ///clean
        IoTHubDeviceTwin_DestroybasicModel_WithData15(modelWithData);

    }

     /*this test wants to see that IoTHubDeviceTwin_CreatebasicModel_WithData15 can receive desired properties*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_RECEIVES_DESIRED_PROPERTY_LL_SUCCEEDS)
    {
        ///arrange

        const char* inputJsonAsString =
        "{\"desired\":{                                                                                        \
            \"$version\": 1,                                                                                   \
            \"with_desired_property_double15\" : 1.0,                                                          \
            \"with_desired_property_int15\" : 2,                                                               \
            \"with_desired_property_float15\" : 3.000000,                                                      \
            \"with_desired_property_long15\" : 4,                                                              \
            \"with_desired_property_sint8_t15\" : 5,                                                           \
            \"with_desired_property_uint8_t15\" : 6,                                                           \
            \"with_desired_property_int16_t15\" : 7,                                                           \
            \"with_desired_property_int32_t15\" : 8,                                                           \
            \"with_desired_property_int64_t15\" : 9,                                                           \
            \"with_desired_property_bool15\" : true,                                                           \
            \"with_desired_property_ascii_char_ptr15\" : \"e/leven\",                                          \
            \"with_desired_property_ascii_char_ptr_no_quotes15\" : \"twelve\",                                 \
            \"with_desired_property_EdmDateTimeOffset15\" : \"2014-06-17T08:51:23.000000000005-08:01\",        \
            \"with_desired_property_EdmGuid15\" : \"00112233-4455-6677-8899-AABBCCDDEEFF\",                    \
            \"with_desired_property_EdmBinary15\": \"MzQ1\"                                                    \
        }}";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_LL_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_LL_HANDLE);

        STRICT_EXPECTED_CALL(on_desired_property_double15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_float15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_long15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_sint8_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_uint8_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int16_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int32_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_int64_t15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_bool15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_ascii_char_ptr15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_ascii_char_ptr_no_quotes15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmDateTimeOffset15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmGuid15(modelWithData));
        STRICT_EXPECTED_CALL(on_desired_property_EdmBinary15(modelWithData));

        ///act
        g_SerializerIngest_fp(DEVICE_TWIN_UPDATE_COMPLETE, (const unsigned char*)inputJsonAsString, strlen(inputJsonAsString), modelWithData);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(modelWithData);

        ASSERT_ARE_EQUAL(double,    1.0,            modelWithData->with_desired_property_double15);
        ASSERT_ARE_EQUAL(int,       2,              modelWithData->with_desired_property_int15);
        ASSERT_ARE_EQUAL(float,     3.0,            modelWithData->with_desired_property_float15);
        ASSERT_ARE_EQUAL(long,      4,              modelWithData->with_desired_property_long15);
        ASSERT_ARE_EQUAL(int8_t,    5,              modelWithData->with_desired_property_sint8_t15);
        ASSERT_ARE_EQUAL(uint8_t,   6,              modelWithData->with_desired_property_uint8_t15);
        ASSERT_ARE_EQUAL(int16_t,   7,              modelWithData->with_desired_property_int16_t15);
        ASSERT_ARE_EQUAL(int32_t,   8,              modelWithData->with_desired_property_int32_t15);
        ASSERT_ARE_EQUAL(int64_t,   9,              modelWithData->with_desired_property_int64_t15);
        ASSERT_IS_TRUE(modelWithData->with_desired_property_bool15);
        ASSERT_ARE_EQUAL(char_ptr,  "e/leven",       modelWithData->with_desired_property_ascii_char_ptr15);
        ASSERT_ARE_EQUAL(char_ptr,  "\"twelve\"",   modelWithData->with_desired_property_ascii_char_ptr_no_quotes15);
        ASSERT_ARE_EQUAL(int,       114,            modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_year);
        ASSERT_ARE_EQUAL(int,       6-1,            modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_mon);
        ASSERT_ARE_EQUAL(int,       17,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_mday);
        ASSERT_ARE_EQUAL(int,       8,              modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_hour);
        ASSERT_ARE_EQUAL(int,       51,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_min);
        ASSERT_ARE_EQUAL(int,       23,             modelWithData->with_desired_property_EdmDateTimeOffset15.dateTime.tm_sec);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.hasFractionalSecond);
        ASSERT_ARE_EQUAL(uint64_t,  5,              modelWithData->with_desired_property_EdmDateTimeOffset15.fractionalSecond);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.hasTimeZone);
        ASSERT_ARE_EQUAL(int8_t,    -8,             modelWithData->with_desired_property_EdmDateTimeOffset15.timeZoneHour);
        ASSERT_ARE_EQUAL(uint8_t,   1,              modelWithData->with_desired_property_EdmDateTimeOffset15.timeZoneMinute);
        ASSERT_ARE_EQUAL(uint8_t,   0x00,           modelWithData->with_desired_property_EdmGuid15.GUID[0]);
        ASSERT_ARE_EQUAL(uint8_t,   0x11,           modelWithData->with_desired_property_EdmGuid15.GUID[1]);
        ASSERT_ARE_EQUAL(uint8_t,   0x22,           modelWithData->with_desired_property_EdmGuid15.GUID[2]);
        ASSERT_ARE_EQUAL(uint8_t,   0x33,           modelWithData->with_desired_property_EdmGuid15.GUID[3]);
        ASSERT_ARE_EQUAL(uint8_t,   0x44,           modelWithData->with_desired_property_EdmGuid15.GUID[4]);
        ASSERT_ARE_EQUAL(uint8_t,   0x55,           modelWithData->with_desired_property_EdmGuid15.GUID[5]);
        ASSERT_ARE_EQUAL(uint8_t,   0x66,           modelWithData->with_desired_property_EdmGuid15.GUID[6]);
        ASSERT_ARE_EQUAL(uint8_t,   0x77,           modelWithData->with_desired_property_EdmGuid15.GUID[7]);
        ASSERT_ARE_EQUAL(uint8_t,   0x88,           modelWithData->with_desired_property_EdmGuid15.GUID[8]);
        ASSERT_ARE_EQUAL(uint8_t,   0x99,           modelWithData->with_desired_property_EdmGuid15.GUID[9]);
        ASSERT_ARE_EQUAL(uint8_t,   0xAA,           modelWithData->with_desired_property_EdmGuid15.GUID[10]);
        ASSERT_ARE_EQUAL(uint8_t,   0xBB,           modelWithData->with_desired_property_EdmGuid15.GUID[11]);
        ASSERT_ARE_EQUAL(uint8_t,   0xCC,           modelWithData->with_desired_property_EdmGuid15.GUID[12]);
        ASSERT_ARE_EQUAL(uint8_t,   0xDD,           modelWithData->with_desired_property_EdmGuid15.GUID[13]);
        ASSERT_ARE_EQUAL(uint8_t,   0xEE,           modelWithData->with_desired_property_EdmGuid15.GUID[14]);
        ASSERT_ARE_EQUAL(uint8_t,   0xFF,           modelWithData->with_desired_property_EdmGuid15.GUID[15]);

        ASSERT_ARE_EQUAL(size_t,     3,             modelWithData->with_desired_property_EdmBinary15.size);
        ASSERT_ARE_EQUAL(uint8_t,   '3',            modelWithData->with_desired_property_EdmBinary15.data[0]);
        ASSERT_ARE_EQUAL(uint8_t,   '4',            modelWithData->with_desired_property_EdmBinary15.data[1]);
        ASSERT_ARE_EQUAL(uint8_t,   '5',            modelWithData->with_desired_property_EdmBinary15.data[2]);

        ///clean
        IoTHubDeviceTwin_LL_DestroybasicModel_WithData15(modelWithData);

    }

    /*this test verifies that a partial update can be ingested*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_RECEIVES_PARTIAL_DESIRED_PROPERTY_SUCCEEDS)
    {
        ///arrange

        const char* inputJsonAsString =
            "{                                                                                        \
                \"with_desired_property_double15\" : 11.0                                             \
            }";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_HANDLE);

        STRICT_EXPECTED_CALL(on_desired_property_double15(modelWithData));

        ///act
        g_SerializerIngest_fp(DEVICE_TWIN_UPDATE_PARTIAL, (const unsigned char*)inputJsonAsString, strlen(inputJsonAsString), modelWithData);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(modelWithData);

        ASSERT_ARE_EQUAL(double, 11.0, modelWithData->with_desired_property_double15);

        ///clean
        IoTHubDeviceTwin_DestroybasicModel_WithData15(modelWithData);

    }

    /*this test verifies that a partial update can be ingested*/
    TEST_FUNCTION(SERIALIZER_DT_AUTOMATICALLY_SUBSCRIBED_TO_DEVICETWIN_AND_RECEIVES_PARTIAL_DESIRED_PROPERTY_LL_SUCCEEDS)
    {
        ///arrange

        const char* inputJsonAsString =
            "{                                                                                        \
                \"with_desired_property_double15\" : 11.0                                             \
            }";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceTwinCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceTwinCallback()
            .IgnoreArgument_userContextCallback();

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SetDeviceMethodCallback(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_deviceMethodCallback()
            .IgnoreArgument_userContextCallback();

        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_LL_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_LL_HANDLE);

        STRICT_EXPECTED_CALL(on_desired_property_double15(modelWithData));

        ///act
        g_SerializerIngest_fp(DEVICE_TWIN_UPDATE_PARTIAL, (const unsigned char*)inputJsonAsString, strlen(inputJsonAsString), modelWithData);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(modelWithData);

        ASSERT_ARE_EQUAL(double, 11.0, modelWithData->with_desired_property_double15);

        ///clean
        IoTHubDeviceTwin_LL_DestroybasicModel_WithData15(modelWithData);

    }

    /*this test verifies that a method is executed when a device is created from serializer_devicetwin*/
    TEST_FUNCTION(SERIALIZER_DT_RECEIVES_AND_EXECUTES_A_METHOD)
    {
        ///arrange

        const char* inputJsonAsString =
            "{                                                          \
                \"a\" : 1,                                              \
                \"b\" : 2                                               \
            }";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);
        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_LL_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_LL_HANDLE);
        umock_c_reset_all_calls();

        unsigned char* result;
        size_t result_size;

        STRICT_EXPECTED_CALL(methodA(modelWithData, 1, 2));

        ///act
        int r = g_deviceMethodCallback_fp("methodA", (const unsigned char*)inputJsonAsString, strlen(inputJsonAsString), &result, &result_size, g_deviceMethodCallback_userContextCallback);

        ///assert
        ASSERT_ARE_EQUAL(int, 3, r);
        ASSERT_ARE_EQUAL(size_t, 8, result_size);
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(int, 0, memcmp("\"ANSWER\"", result, result_size));

        ///clean
        IoTHubDeviceTwin_LL_DestroybasicModel_WithData15(modelWithData);
        free(result);
    }

    /*this test verifies that SendReportedState works for a device*/
    TEST_FUNCTION(SERIALIZER_DT_SENDREPORTEDSTATE_SUCCEEDS_CONVENIENCE)
    {
        ///arrange

        const char* expectedOutputJsonAsString =
            "{                                                          \
            \"with_reported_property_int15\" : 15                       \
        }";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);
        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_HANDLE);
        umock_c_reset_all_calls();

        modelWithData->with_reported_property_int15 = 15;

        STRICT_EXPECTED_CALL(IoTHubClient_SendReportedState(TEST_IOTHUB_CLIENT_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, sendReportedStateCallback, (void*)0x44))
            .IgnoreArgument_reportedState()
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(sendReportedStateCallback(201, (void*)0x44));

        ///act
        IOTHUB_CLIENT_RESULT r = IoTHubDeviceTwin_SendReportedStatebasicModel_WithData15(modelWithData, sendReportedStateCallback, (void*)0x44);

        ///assert
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        /*compare 2 jsons*/
        JSON_Value* expectedJson = json_parse_string(expectedOutputJsonAsString);
        ASSERT_IS_NOT_NULL(expectedJson);
        JSON_Value* actualJson = json_parse_string(g_IoTHubClient_SendReportedState_reportedState);
        ASSERT_IS_NOT_NULL(actualJson);

        ASSERT_ARE_EQUAL(int, 0, !json_value_equals(expectedJson, actualJson));

        ///clean
        IoTHubDeviceTwin_DestroybasicModel_WithData15(modelWithData);
        json_value_free(expectedJson);
        json_value_free(actualJson);
    }

    /*this test verifies that SendReportedState works for a device*/
    TEST_FUNCTION(SERIALIZER_DT_SENDREPORTEDSTATE_SUCCEEDS_LL)
    {
        ///arrange

        const char* expectedOutputJsonAsString =
            "{                                                          \
            \"with_reported_property_int15\" : 15                       \
        }";

        (void)SERIALIZER_REGISTER_NAMESPACE(basic15);
        basicModel_WithData15* modelWithData = IoTHubDeviceTwin_LL_CreatebasicModel_WithData15(TEST_IOTHUB_CLIENT_LL_HANDLE);
        umock_c_reset_all_calls();

        modelWithData->with_reported_property_int15 = 15;

        STRICT_EXPECTED_CALL(IoTHubClient_LL_SendReportedState(TEST_IOTHUB_CLIENT_LL_HANDLE, IGNORED_PTR_ARG, IGNORED_NUM_ARG, sendReportedStateCallback, (void*)0x44))
            .IgnoreArgument_reportedState()
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(sendReportedStateCallback(201, (void*)0x44));

        ///act
        IOTHUB_CLIENT_RESULT r = IoTHubDeviceTwin_LL_SendReportedStatebasicModel_WithData15(modelWithData, sendReportedStateCallback, (void*)0x44);

        ///assert
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, r);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        /*compare 2 jsons*/
        JSON_Value* expectedJson = json_parse_string(expectedOutputJsonAsString);
        ASSERT_IS_NOT_NULL(expectedJson);
        JSON_Value* actualJson = json_parse_string(g_IoTHubClient_LL_SendReportedState_reportedState);
        ASSERT_IS_NOT_NULL(actualJson);

        ASSERT_ARE_EQUAL(int, 0, !json_value_equals(expectedJson, actualJson));

        ///clean
        IoTHubDeviceTwin_LL_DestroybasicModel_WithData15(modelWithData);
        json_value_free(expectedJson);
        json_value_free(actualJson);
    }

END_TEST_SUITE(serializer_dt_int)
