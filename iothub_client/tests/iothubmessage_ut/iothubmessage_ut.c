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

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/map.h"

#undef ENABLE_MOCKS

#include "iothub_message.h"
#include "real_strings.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern BUFFER_HANDLE real_BUFFER_new(void);
    extern void real_BUFFER_delete(BUFFER_HANDLE handle);
    extern unsigned char* real_BUFFER_u_char(BUFFER_HANDLE handle);
    extern size_t real_BUFFER_length(BUFFER_HANDLE handle);
    extern int real_BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern int real_BUFFER_append_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern BUFFER_HANDLE real_BUFFER_clone(BUFFER_HANDLE handle);
    extern BUFFER_HANDLE real_BUFFER_create(const unsigned char* source, size_t size);

#ifdef __cplusplus
}
#endif

#define NUMBER_OF_CHAR      8

static MAP_FILTER_CALLBACK g_mapFilterFunc;

static const unsigned char c[1] = { '3' };
static const char* TEST_MESSAGE_ID = "3820ADAE-E3CA-4065-843A-A6BDE950D8DC";
static const char* TEST_MESSAGE_ID2 = "052BA01A-ECBF-48CF-BC7B-64B315D898B7";
static const char* TEST_STRING_VALUE = "aaaa";
static const char* TEST_VALID_MAP_KEY = "Valid_key";
static const char* TEST_VALID_MAP_VALUE = "Valid_value";
static const char* TEST_INVALID_MAP_KEY = "Inval\nd_key";
static const char* TEST_INVALID_MAP_VALUE = "Inval\nd_value";
static const char* TEST_CONTENT_TYPE = "text/plain";
static const char* TEST_CONTENT_ENCODING = "utf8";

static const char* TEST_OUTPUT_NAME = "outputname";
static const char* TEST_OUTPUT_NAME2 = "outputname2";
static const char* TEST_INPUT_NAME = "inputname";
static const char* TEST_INPUT_NAME2 = "inputname2";
static const char* TEST_CONNECTION_DEVICE_ID = "connectiondeviceid";
static const char* TEST_CONNECTION_DEVICE_ID2 = "connectiondeviceid2";
static const char* TEST_CONNECTION_MODULE_ID = "connectionmoduleid";
static const char* TEST_CONNECTION_MODULE_ID2 = "connectionmoduleid2";
static const char* TEST_PROPERTY_KEY = "property_key";
static const char* TEST_PROPERTY_VALUE = "property_value";

static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA TEST_DIAGNOSTIC_DATA = { "12345678",  "1506054179"};
static IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA TEST_DIAGNOSTIC_DATA2 = { "87654321", "1506054179.100" };

TEST_DEFINE_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static MAP_HANDLE my_Map_Create(MAP_FILTER_CALLBACK mapFilterFunc)
{
    g_mapFilterFunc = mapFilterFunc;
    return (MAP_HANDLE)my_gballoc_malloc(1);
}

static MAP_HANDLE my_Map_Clone(MAP_HANDLE handle)
{
    (void)handle;
    return (MAP_HANDLE)my_gballoc_malloc(1);
}

static void my_Map_Destroy(MAP_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    *destination = (char*)my_gballoc_malloc(strlen(source)+1);
    strcpy(*destination, source);
    return 0;
}

typedef const char*(*PFN_MESSAGE_GET_STRING)(IOTHUB_MESSAGE_HANDLE handle);
typedef IOTHUB_MESSAGE_RESULT(*PFN_MESSAGE_SET_STRING)(IOTHUB_MESSAGE_HANDLE handle, const char *string);

static void get_string_NULL_handle_fails_impl(PFN_MESSAGE_GET_STRING pfnGetMessageString)
{
    //arrange

    //act
    const char* result = pfnGetMessageString(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

static void set_string_NULL_handle_fails_impl(PFN_MESSAGE_SET_STRING pfnSetMessageString, const char* test_value)
{
    //arrange

    //act
    IOTHUB_MESSAGE_RESULT result = pfnSetMessageString(NULL, test_value);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

static void set_string_NULL_string_fails_impl(PFN_MESSAGE_SET_STRING pfnSetMessageString)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = pfnSetMessageString(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);

}

/* Tests_SRS_IOTHUBMESSAGE_07_015: [IoTHubMessage_SetMessageId finishes successfully it shall return IOTHUB_MESSAGE_OK.] */
static void set_string_succeeds_impl(PFN_MESSAGE_SET_STRING pfnSetMessageString, const char *test_value)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, test_value));

    //act
    IOTHUB_MESSAGE_RESULT result = pfnSetMessageString(h, test_value);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

static void set_string_string_already_allocated_succeeds_impl(PFN_MESSAGE_SET_STRING pfnSetMessageString, const char *test_value)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IOTHUB_MESSAGE_RESULT result = pfnSetMessageString(h, test_value);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, test_value));

    //act
    result = pfnSetMessageString(h, test_value);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}


static void get_string_not_set_fails_impl(PFN_MESSAGE_GET_STRING pfnGetMessageString)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    const char* result = pfnGetMessageString(h);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_011: [IoTHubMessage_GetMessageId shall return the messageId as a const char*.] */
static void get_string_succeeds_impl(PFN_MESSAGE_SET_STRING pfnSetMessageString, PFN_MESSAGE_GET_STRING pfnGetMessageString, const char *test_value)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    IOTHUB_MESSAGE_RESULT msgResult = pfnSetMessageString(h, test_value);
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, msgResult);
    umock_c_reset_all_calls();

    //act
    const char* result = pfnGetMessageString(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, test_value, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(iothubmessage_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    //result = umocktypes_bool_register_types();
    //ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(MAP_FILTER_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, real_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, real_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, real_BUFFER_delete);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_build, real_BUFFER_build);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, real_BUFFER_u_char);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, real_BUFFER_length);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, real_BUFFER_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;

    REGISTER_GLOBAL_MOCK_HOOK(Map_Create, my_Map_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Map_Clone, my_Map_Clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_Clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(Map_Destroy, my_Map_Destroy);
    REGISTER_GLOBAL_MOCK_RETURN(Map_AddOrUpdate, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_AddOrUpdate, MAP_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(Map_ContainsKey, MAP_OK);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

static void reset_test_data()
{
    g_mapFilterFunc = NULL;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();
    reset_test_data();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_IOTHUBMESSAGE_02_022: [IoTHubMessage_CreateFromByteArray shall call BUFFER_create passing byteArray and size as parameters.]*/
/*Tests_SRS_IOTHUBMESSAGE_02_023: [IoTHubMessage_CreateFromByteArray shall call Map_Create to create the message properties.]*/
/*Tests_SRS_IOTHUBMESSAGE_02_025: [Otherwise, IoTHubMessage_CreateFromByteArray shall return a non-NULL handle.] */
/*Tests_SRS_IOTHUBMESSAGE_02_026: [The type of the new message shall be IOTHUBMESSAGE_BYTEARRAY.] */
/*Tests_SRS_IOTHUBMESSAGE_02_009: [Otherwise IoTHubMessage_GetContentType shall return the type of the message.] */
TEST_FUNCTION(IoTHubMessage_CreateFromByteArray_happy_path)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(c, 1));
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_BYTEARRAY, IoTHubMessage_GetContentType(h) );
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_06_002: [If size is NOT zero then byteArray MUST NOT be NULL*/
TEST_FUNCTION(IoTHubMessage_CreateFromByteArray_fails_when_size_non_zero_buffer_NULL)
{
    //arrange

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(NULL, 1);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_06_001: [If size is zero then byteArray may be NULL.]*/
TEST_FUNCTION(IoTHubMessage_CreateFromByteArray_succeeds_when_size_0_and_buffer_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(NULL, 0);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_06_001: [If size is zero then byteArray may be NULL.]*/
TEST_FUNCTION(IoTHubMessage_CreateFromByteArray_succeeds_when_size_0_and_buffer_non_NULL)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, 0)).IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 0);

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_02_024: [If there are any errors then IoTHubMessage_CreateFromByteArray shall return NULL*/
TEST_FUNCTION(IoTHubMessage_CreateFromByteArray_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_create(c, 1));
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubMessage_CreateFromByteArray failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

        //assert
        ASSERT_IS_NULL(h, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBMESSAGE_02_027: [IoTHubMessage_CreateFromString shall call STRING_construct passing source as parameter.] */
/*Tests_SRS_IOTHUBMESSAGE_02_028: [IoTHubMessage_CreateFromString shall call Map_Create to create the message properties.] */
/*Tests_SRS_IOTHUBMESSAGE_02_031: [Otherwise, IoTHubMessage_CreateFromString shall return a non-NULL handle.] */
/*Tests_SRS_IOTHUBMESSAGE_02_032: [The type of the new message shall be IOTHUBMESSAGE_STRING.] */
/*Tests_SRS_IOTHUBMESSAGE_02_009: [Otherwise IoTHubMessage_GetContentType shall return the type of the message.] */
TEST_FUNCTION(IoTHubMessage_CreateFromString_happy_path)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct("a"));
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString("a");

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_STRING, IoTHubMessage_GetContentType(h));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_CreateFromString_source_NULL_fail)
{
    //arrange

    //act
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(NULL);

    //assert
    ASSERT_IS_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_02_029: [If there are any encountered in the execution of IoTHubMessage_CreateFromString then IoTHubMessage_CreateFromString shall return NULL.] */
TEST_FUNCTION(IoTHubMessage_CreateFromString_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    // arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_construct("a"));
    STRICT_EXPECTED_CALL(Map_Create(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubMessage_CreateFromByteArray failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString("a");

        //assert
        ASSERT_IS_NULL(h, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubMessage_Map_Filter_validate_Ascii_char_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    int result = g_mapFilterFunc(TEST_VALID_MAP_KEY, TEST_VALID_MAP_VALUE);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_Map_Filter_invalidate_Ascii_key_fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    int result = g_mapFilterFunc(TEST_INVALID_MAP_KEY, TEST_VALID_MAP_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_Map_Filter_invalidate_Ascii_value_fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    int result = g_mapFilterFunc(TEST_VALID_MAP_KEY, TEST_INVALID_MAP_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_01_004: [If iotHubMessageHandle is NULL, IoTHubMessage_Destroy shall do nothing.] */
TEST_FUNCTION(IoTHubMessage_Destroy_With_NULL_handle_does_nothing)
{
    // arrange

    ///act
    IoTHubMessage_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_01_003: [IoTHubMessage_Destroy shall free all resources associated with iotHubMessageHandle.]  */
TEST_FUNCTION(IoTHubMessage_Destroy_destroys_a_BYTEARRAY_IoTHubMEssage)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(h));

    //act
    IoTHubMessage_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_01_003: [IoTHubMessage_Destroy shall free all resources associated with iotHubMessageHandle.]  */
TEST_FUNCTION(IoTHubMessage_Destroy_destroys_a_STRING_IoTHubMessage)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(h));

    //act
    IoTHubMessage_Destroy(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_01_011: [The pointer shall be obtained by using BUFFER_u_char and it shall be copied in the buffer argument.]*/
/*Tests_SRS_IOTHUBMESSAGE_01_012: [The size of the associated data shall be obtained by using BUFFER_length and it shall be copied to the size argument.]*/
/*Tests_SRS_IOTHUBMESSAGE_02_033: [IoTHubMessage_GetByteArray shall return IOTHUBMESSAGE_OK when all oeprations complete succesfully.] */
TEST_FUNCTION(IoTHubMessage_GetByteArray_happy_path)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    const unsigned char* byteArray;
    size_t size;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_RESULT r = IoTHubMessage_GetByteArray(h, &byteArray, &size);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, r);
    ASSERT_ARE_EQUAL(uint8_t, c[0], byteArray[0]);
    ASSERT_ARE_EQUAL(size_t, 1, size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_01_014: [If any of the arguments passed to IoTHubMessage_GetByteArray  is NULL IoTHubMessage_GetByteArray shall return IOTHUBMESSAGE_INVALID_ARG.] */
TEST_FUNCTION(IoTHubMessage_GetByteArray_with_NULL_handle_fails)
{
    //arrange
    const unsigned char* byteArray;
    size_t size;

    //act
    IOTHUB_MESSAGE_RESULT r = IoTHubMessage_GetByteArray(NULL, &byteArray, &size);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_01_014: [If any of the arguments passed to IoTHubMessage_GetByteArray  is NULL IoTHubMessage_GetByteArray shall return IOTHUBMESSAGE_INVALID_ARG.] */
TEST_FUNCTION(IoTHubMessage_GetByteArray_with_NULL_byteArray_fails)
{
    ///arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    size_t size;
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT r = IoTHubMessage_GetByteArray(h, NULL, &size);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_02_021: [If iotHubMessageHandle is not a iothubmessage containing BYTEARRAY data, then IoTHubMessage_GetByteArray  shall return IOTHUBMESSAGE_INVALID_ARG.]*/
TEST_FUNCTION(IoTHubMessage_GetByteArray_with_STRING_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    const unsigned char* byteArray;
    size_t size;
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT r = IoTHubMessage_GetByteArray(h, &byteArray, &size);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_03_001: [IoTHubMessage_Clone shall create a new IoT hub message with data content identical to that of the iotHubMessageHandle parameter.]*/
/*Tests_SRS_IOTHUBMESSAGE_02_006: [IoTHubMessage_Clone shall clone the content by a call to BUFFER_clone or STRING_clone] */
/*Tests_SRS_IOTHUBMESSAGE_02_005: [IoTHubMessage_Clone shall clone the properties map by using Map_Clone.] */
/*Tests_SRS_IOTHUBMESSAGE_03_002: [IoTHubMessage_Clone shall return upon success a non-NULL handle to the newly created IoT hub message.]*/
TEST_FUNCTION(IoTHubMessage_Clone_with_BYTE_ARRAY_happy_path)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Clone(IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_HANDLE r = IoTHubMessage_Clone(h);

    //assert
    ASSERT_IS_NOT_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(r);
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_Clone_handle_NULL_fail)
{
    //arrange


    //act
    IOTHUB_MESSAGE_HANDLE r = IoTHubMessage_Clone(NULL);

    //assert
    ASSERT_IS_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
TEST_FUNCTION(IoTHubMessage_Clone_with_BYTE_ARRAY_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Clone(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubMessage_Clone failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        IOTHUB_MESSAGE_HANDLE r = IoTHubMessage_Clone(h);

        //assert
        ASSERT_IS_NULL(r, tmp_msg);
    }

    //cleanup
    IoTHubMessage_Destroy(h);
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBMESSAGE_03_001: [IoTHubMessage_Clone shall create a new IoT hub message with data content identical to that of the iotHubMessageHandle parameter.]*/
/*Tests_SRS_IOTHUBMESSAGE_02_006: [IoTHubMessage_Clone shall clone the content by a call to BUFFER_clone or STRING_clone] */
/*Tests_SRS_IOTHUBMESSAGE_02_005: [IoTHubMessage_Clone shall clone the properties map by using Map_Clone.] */
/*Tests_SRS_IOTHUBMESSAGE_03_002: [IoTHubMessage_Clone shall return upon success a non-NULL handle to the newly created IoT hub message.]*/
TEST_FUNCTION(IoTHubMessage_Clone_with_STRING_happy_path)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Clone(IGNORED_PTR_ARG));

    ///act
    IOTHUB_MESSAGE_HANDLE r = IoTHubMessage_Clone(h);

    ///assert
    ASSERT_IS_NOT_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(r);
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
TEST_FUNCTION(IoTHubMessage_Clone_with_STRING_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Clone(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubMessage_Clone_w_string failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        IOTHUB_MESSAGE_HANDLE r = IoTHubMessage_Clone(h);

        //assert
        ASSERT_IS_NULL(r, tmp_msg);
    }

    //cleanup
    IoTHubMessage_Destroy(h);
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_IOTHUBMESSAGE_02_002: [Otherwise, for any non-NULL iotHubMessageHandle it shall return a non-NULL MAP_HANDLE.] */
TEST_FUNCTION(IoTHubMessage_Properties_happy_path)
{
    ///arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    MAP_HANDLE r = IoTHubMessage_Properties(h);

    //assert
    ASSERT_IS_NOT_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_02_001: [If iotHubMessageHandle is NULL then IoTHubMessage_Properties shall return NULL.] */
TEST_FUNCTION(IoTHubMessage_Properties_with_NULL_handle_retuns_NULL)
{
    //arrange

    //act
    MAP_HANDLE r = IoTHubMessage_Properties(NULL);

    //assert
    ASSERT_IS_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_02_008: [If any parameter is NULL then IoTHubMessage_GetContentType shall return IOTHUBMESSAGE_UNKNOWN.] */
TEST_FUNCTION(IoTHubMessage_GetContentType_with_NULL_handle_fails)
{
    //arrange

    //act
    IOTHUBMESSAGE_CONTENT_TYPE r = IoTHubMessage_GetContentType(NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_UNKNOWN, r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_02_016: [If any parameter is NULL then IoTHubMessage_GetString  shall return NULL.] */
TEST_FUNCTION(IoTHubMessage_GetString_with_NULL_handle_returns_NULL)
{
    //arrange

    //act
    const char* r = IoTHubMessage_GetString(NULL);

    //assert
    ASSERT_IS_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_SRS_IOTHUBMESSAGE_02_018: [IoTHubMessage_GetStringData shall return the currently stored null terminated string.] */
TEST_FUNCTION(IoTHubMessage_GetString_happy_path)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    //act
    const char* r = IoTHubMessage_GetString(h);

    //assert
    ASSERT_IS_NOT_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/*Tests_SRS_IOTHUBMESSAGE_02_017: [IoTHubMessage_GetString shall return NULL if the iotHubMessageHandle does not refer to a IOTHUBMESSAGE of type STRING.] */
TEST_FUNCTION(IoTHubMessage_GetString_with_BYTEARRAY_fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    const char* r = IoTHubMessage_GetString(h);

    //assert
    ASSERT_IS_NULL(r);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_012: [if any of the parameters are NULL then IoTHubMessage_SetMessageId shall return a IOTHUB_MESSAGE_INVALID_ARG value.] */
TEST_FUNCTION(IoTHubMessage_SetMessageId_NULL_handle_Fails)
{
    //arrange

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetMessageId(NULL, TEST_MESSAGE_ID);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBMESSAGE_07_012: [if any of the parameters are NULL then IoTHubMessage_SetMessageId shall return a IOTHUB_MESSAGE_INVALID_ARG value.] */
TEST_FUNCTION(IoTHubMessage_SetMessageId_NULL_MessageId_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetMessageId(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_015: [IoTHubMessage_SetMessageId finishes successfully it shall return IOTHUB_MESSAGE_OK.] */
TEST_FUNCTION(IoTHubMessage_SetMessageId_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_MESSAGE_ID));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetMessageId(h, TEST_MESSAGE_ID);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* SRS_IOTHUBMESSAGE_07_013: [If the IOTHUB_MESSAGE_HANDLE messageId is not NULL, then the IOTHUB_MESSAGE_HANDLE messageId will be deallocated.] */
TEST_FUNCTION(IoTHubMessage_SetMessageId_MessageId_Not_NULL_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetMessageId(h, TEST_MESSAGE_ID);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_MESSAGE_ID2));

    //act
    result = IoTHubMessage_SetMessageId(h, TEST_MESSAGE_ID2);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_010: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetMessageId shall return a NULL value.] */
TEST_FUNCTION(IoTHubMessage_GetMessageId_NULL_handle_Fails)
{
    //arrange

    //act
    const char* result = IoTHubMessage_GetMessageId(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBMESSAGE_07_011: [IoTHubMessage_GetMessageId shall return the messageId as a const char*.] */
TEST_FUNCTION(IoTHubMessage_GetMessageId_MessageId_Not_Set_Fails)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetMessageId(h);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_011: [IoTHubMessage_GetMessageId shall return the messageId as a const char*.] */
TEST_FUNCTION(IoTHubMessage_GetMessageId_SUCCEED)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetMessageId(h, TEST_MESSAGE_ID);
    //ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, msgResult);
    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetMessageId(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_MESSAGE_ID, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_016: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetCorrelationId shall return a NULL value.] */
TEST_FUNCTION(IoTHubMessage_GetCorrelationId_NULL_handle_Fails)
{
    //arrange

    //act
    const char* result = IoTHubMessage_GetCorrelationId(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBMESSAGE_07_016: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetCorrelationId shall return a NULL value.] */
TEST_FUNCTION(IoTHubMessage_GetCorrelationId_MessageId_Not_Set_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetCorrelationId(h);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_017: [IoTHubMessage_GetCorrelationId shall return the correlationId as a const char*.] */
TEST_FUNCTION(IoTHubMessage_GetCorrelationId_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetCorrelationId(h, TEST_MESSAGE_ID);
    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetCorrelationId(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_MESSAGE_ID, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_018: [if any of the parameters are NULL then IoTHubMessage_SetCorrelationId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]*/
TEST_FUNCTION(IoTHubMessage_SetCorrelationId_NULL_CorrelationId_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetCorrelationId(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_021: [IoTHubMessage_SetCorrelationId finishes successfully it shall return IOTHUB_MESSAGE_OK.] */
TEST_FUNCTION(IoTHubMessage_SetCorrelationId_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_MESSAGE_ID));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetCorrelationId(h, TEST_MESSAGE_ID);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_07_019: [If the IOTHUB_MESSAGE_HANDLE correlationId is not NULL, then the IOTHUB_MESSAGE_HANDLE correlationId will be deallocated.] */
TEST_FUNCTION(IoTHubMessage_SetCorrelationId_CorrelationId_Not_NULL_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    (void)IoTHubMessage_SetCorrelationId(h, TEST_MESSAGE_ID);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_MESSAGE_ID2));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetCorrelationId(h, TEST_MESSAGE_ID2);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_004: [If IoTHubMessage_SetContentTypeSystemProperty finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetContentTypeSystemProperty_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_TYPE));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_002: [If the IOTHUB_MESSAGE_HANDLE `contentType` is not NULL it shall be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetContentTypeSystemProperty_Not_NULL_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_TYPE));

    //act
    result = IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_001: [If any of the parameters are NULL then IoTHubMessage_SetContentTypeSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetContentTypeSystemProperty_NULL_handle_Fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentTypeSystemProperty(NULL, TEST_CONTENT_TYPE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SRS_IOTHUBMESSAGE_09_001: [If any of the parameters are NULL then IoTHubMessage_SetContentTypeSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetContentTypeSystemProperty_NULL_contentType_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentTypeSystemProperty(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_003: [If the allocation or the copying of `contentType` fails, then IoTHubMessage_SetContentTypeSystemProperty shall return IOTHUB_MESSAGE_ERROR.]
TEST_FUNCTION(IoTHubMessage_SetContentTypeSystemProperty_Fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_TYPE));
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failed in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        //act
        IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_ERROR, result, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_009: [If IoTHubMessage_SetContentEncodingSystemProperty finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetContentEncodingSystemProperty_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_ENCODING));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_007: [If the IOTHUB_MESSAGE_HANDLE `contentEncoding` is not NULL it shall be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetContentEncodingSystemProperty_Not_NULL_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_ENCODING));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    result = IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_006: [If any of the parameters are NULL then IoTHubMessage_SetContentEncodingSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetContentEncodingSystemProperty_NULL_handle_Fails)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentEncodingSystemProperty(NULL, TEST_CONTENT_ENCODING);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SRS_IOTHUBMESSAGE_09_006: [If any of the parameters are NULL then IoTHubMessage_SetContentEncodingSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetContentEncodingSystemProperty_NULL_contentType_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentEncodingSystemProperty(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_008: [If the allocation or the copying of `contentEncoding` fails, then IoTHubMessage_SetContentEncodingSystemProperty shall return IOTHUB_MESSAGE_ERROR.]
TEST_FUNCTION(IoTHubMessage_SetContentEncodingSystemProperty_Fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_CONTENT_ENCODING));
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failed in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        //act
        IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_ERROR, result, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_006: [IoTHubMessage_GetContentTypeSystemProperty shall return the `contentType` as a const char* ]
TEST_FUNCTION(IoTHubMessage_GetContentTypeSystemProperty_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentTypeSystemProperty(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONTENT_TYPE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_006: [IoTHubMessage_GetContentTypeSystemProperty shall return the `contentType` as a const char* ]
TEST_FUNCTION(IoTHubMessage_GetContentTypeSystemProperty_NULL_contentType_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentTypeSystemProperty(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_005: [If any of the parameters are NULL then BLAH shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_GetContentTypeSystemProperty_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetContentTypeSystemProperty(h, TEST_CONTENT_TYPE);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentTypeSystemProperty(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_011: [IoTHubMessage_GetContentEncodingSystemProperty shall return the `contentEncoding` as a const char* ]
TEST_FUNCTION(IoTHubMessage_GetContentEncodingSystemProperty_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentEncodingSystemProperty(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONTENT_ENCODING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_011: [IoTHubMessage_GetContentEncodingSystemProperty shall return the `contentEncoding` as a const char* ]
TEST_FUNCTION(IoTHubMessage_GetContentEncodingSystemProperty_NULL_contentType_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentEncodingSystemProperty(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_09_010: [If any of the parameters are NULL then IoTHubMessage_GetContentEncodingSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_GetContentEncodingSystemProperty_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetContentEncodingSystemProperty(h, TEST_CONTENT_ENCODING);

    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetContentEncodingSystemProperty(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_001: [If any of the parameters are NULL then IoTHubMessage_GetDiagnosticPropertyData shall return a NULL value.]
TEST_FUNCTION(IoTHubMessage_GetDiagnosticPropertyData_NULL_handle_Fails)
{
    //arrange

    //act
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* result = IoTHubMessage_GetDiagnosticPropertyData(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_IOTHUBMESSAGE_10_002: [IoTHubMessage_GetDiagnosticPropertyData shall return the diagnosticData as a const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA*.] */
TEST_FUNCTION(IoTHubMessage_GetDiagnosticPropertyData_DiagnosticData_Not_Set_Fails)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    umock_c_reset_all_calls();

    //act
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* result = IoTHubMessage_GetDiagnosticPropertyData(h);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    IoTHubMessage_Destroy(h);
}

/* Tests_SRS_IOTHUBMESSAGE_10_002: [IoTHubMessage_GetDiagnosticPropertyData shall return the diagnosticData as a const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA*.] */
TEST_FUNCTION(IoTHubMessage_GetDiagnosticPropertyData_SUCCEED)
{
    // arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromString(TEST_STRING_VALUE);
    (void)IoTHubMessage_SetDiagnosticPropertyData(h, &TEST_DIAGNOSTIC_DATA);

    umock_c_reset_all_calls();

    //act
    const IOTHUB_MESSAGE_DIAGNOSTIC_PROPERTY_DATA* result = IoTHubMessage_GetDiagnosticPropertyData(h);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, TEST_DIAGNOSTIC_DATA.diagnosticId, result->diagnosticId);
    ASSERT_ARE_EQUAL(char_ptr, TEST_DIAGNOSTIC_DATA.diagnosticCreationTimeUtc, result->diagnosticCreationTimeUtc);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_003: [If any of the parameters are NULL then IoTHubMessage_SetDiagnosticPropertyData shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetDiagnosticPropertyData_NULL_handle_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetDiagnosticPropertyData(NULL, &TEST_DIAGNOSTIC_DATA);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_003: [If any of the parameters are NULL then IoTHubMessage_SetDiagnosticPropertyData shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetDiagnosticPropertyData_NULL_DiagnosticData_Fails)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetDiagnosticPropertyData(h, NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_004: [If the IOTHUB_MESSAGE_HANDLE `diagnosticData` is not NULL it shall be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetDiagnosticPropertyData_DiagnosticData_Not_NULL_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    (void)IoTHubMessage_SetDiagnosticPropertyData(h, &TEST_DIAGNOSTIC_DATA);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetDiagnosticPropertyData(h, &TEST_DIAGNOSTIC_DATA2);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_005: [If the allocation or the copying of `diagnosticData` fails, then IoTHubMessage_SetDiagnosticPropertyData shall return IOTHUB_MESSAGE_ERROR.]
TEST_FUNCTION(IoTHubMessage_SetDiagnosticPropertyData_Fails)
{
    // arrange
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "Failed in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        //act
        IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetDiagnosticPropertyData(h, &TEST_DIAGNOSTIC_DATA);

        //assert
        ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_ERROR, result, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_10_006: [If IoTHubMessage_SetDiagnosticPropertyData finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetDiagnosticPropertyData_SUCCEED)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetDiagnosticPropertyData(h, &TEST_DIAGNOSTIC_DATA);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_SetProperty_handle_NULL_Fail)
{
    //arrange

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetProperty(NULL, TEST_PROPERTY_KEY, TEST_PROPERTY_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubMessage_SetProperty_key_NULL_Fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetProperty(h, NULL, TEST_PROPERTY_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_SetProperty_value_NULL_Fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetProperty(h, TEST_PROPERTY_KEY, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_SetProperty_Fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Map_AddOrUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(MAP_ERROR);

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetProperty(h, TEST_PROPERTY_KEY, TEST_PROPERTY_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_SetProperty_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(Map_AddOrUpdate(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetProperty(h, TEST_PROPERTY_KEY, TEST_PROPERTY_VALUE);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_GetProperty_handle_NULL_Fail)
{
    //arrange

    //act
    const char* result = IoTHubMessage_GetProperty(NULL, TEST_PROPERTY_KEY);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubMessage_GetProperty_key_NULL_Fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    const char* result = IoTHubMessage_GetProperty(h, NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_GetProperty_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    bool key_exist = true;
    STRICT_EXPECTED_CALL(Map_ContainsKey(IGNORED_PTR_ARG, TEST_PROPERTY_KEY, IGNORED_PTR_ARG)).CopyOutArgumentBuffer_keyExists(&key_exist, sizeof(bool));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, TEST_PROPERTY_KEY)).SetReturn(TEST_PROPERTY_VALUE);

    //act
    const char* result = IoTHubMessage_GetProperty(h, TEST_PROPERTY_KEY);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, TEST_PROPERTY_VALUE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_GetProperty_Fail)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    bool key_exist = false;
    STRICT_EXPECTED_CALL(Map_ContainsKey(IGNORED_PTR_ARG, TEST_PROPERTY_KEY, IGNORED_PTR_ARG)).CopyOutArgumentBuffer_keyExists(&key_exist, sizeof(bool));

    //act
    const char* result = IoTHubMessage_GetProperty(h, TEST_PROPERTY_KEY);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

// Tests_SRS_IOTHUBMESSAGE_31_036: [If any of the parameters are NULL then IoTHubMessage_SetOutputName shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetOutputName_NULL_handle_Fails)
{
    set_string_NULL_handle_fails_impl(IoTHubMessage_SetOutputName, TEST_OUTPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_036: [If any of the parameters are NULL then IoTHubMessage_SetOutputName shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetOutputName_NULL_MessageId_Fails)
{
    set_string_NULL_string_fails_impl(IoTHubMessage_SetOutputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_039: [IoTHubMessage_SetOutputName finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetOutputName_SUCCEED)
{
    set_string_succeeds_impl(IoTHubMessage_SetOutputName, TEST_OUTPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_037: [If the IOTHUB_MESSAGE_HANDLE OutputName is not NULL, then the IOTHUB_MESSAGE_HANDLE OutputName will be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetOutputName_OutputName_Not_NULL_SUCCEED)
{
    set_string_string_already_allocated_succeeds_impl(IoTHubMessage_SetOutputName, TEST_OUTPUT_NAME2);
}

// Tests_SRS_IOTHUBMESSAGE_31_034: [If the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetOutputName shall return a NULL value.]
TEST_FUNCTION(IoTHubMessage_GetOutputName_NULL_handle_Fails)
{
    get_string_NULL_handle_fails_impl(IoTHubMessage_GetOutputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_035: [IoTHubMessage_GetOutputName shall return the OutputName as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetOutputName_OutputName_Not_Set_Fails)
{
    get_string_not_set_fails_impl(IoTHubMessage_GetOutputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_035: [IoTHubMessage_GetOutputName shall return the OutputName as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetOutputName_SUCCEED)
{
    get_string_succeeds_impl(IoTHubMessage_SetOutputName, IoTHubMessage_GetOutputName, TEST_OUTPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_042: [if any of the parameters are NULL then IoTHubMessage_SetInputName shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetInputName_NULL_handle_Fails)
{
    set_string_NULL_handle_fails_impl(IoTHubMessage_SetInputName, TEST_INPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_042: [if any of the parameters are NULL then IoTHubMessage_SetInputName shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetInputName_NULL_MessageId_Fails)
{
    set_string_NULL_string_fails_impl(IoTHubMessage_SetInputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_045: [IoTHubMessage_SetInputName finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetInputName_SUCCEED)
{
    set_string_succeeds_impl(IoTHubMessage_SetInputName, TEST_INPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_043: [If the IOTHUB_MESSAGE_HANDLE InputName is not NULL, then the IOTHUB_MESSAGE_HANDLE InputName will be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetInputName_InputName_Not_NULL_SUCCEED)
{
    set_string_string_already_allocated_succeeds_impl(IoTHubMessage_SetInputName, TEST_INPUT_NAME2);
}

// Tests_SRS_IOTHUBMESSAGE_31_042: [if any of the parameters are NULL then IoTHubMessage_SetInputName shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_GetInputName_NULL_handle_Fails)
{
    get_string_NULL_handle_fails_impl(IoTHubMessage_GetInputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_045: [IoTHubMessage_SetInputName finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_GetInputName_InputName_Not_Set_Fails)
{
    get_string_not_set_fails_impl(IoTHubMessage_GetInputName);
}

// Tests_SRS_IOTHUBMESSAGE_31_045: [IoTHubMessage_SetInputName finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_GetInputName_SUCCEED)
{
    get_string_succeeds_impl(IoTHubMessage_SetInputName, IoTHubMessage_GetInputName, TEST_INPUT_NAME);
}

// Tests_SRS_IOTHUBMESSAGE_31_048: [if any of the parameters are NULL then IoTHubMessage_SetConnectionModuleId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetConnectionModuleId_NULL_handle_Fails)
{
    set_string_NULL_handle_fails_impl(IoTHubMessage_SetConnectionModuleId, TEST_CONNECTION_MODULE_ID);
}

// Tests_SRS_IOTHUBMESSAGE_31_048: [if any of the parameters are NULL then IoTHubMessage_SetConnectionModuleId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetConnectionModuleId_NULL_MessageId_Fails)
{
    set_string_NULL_string_fails_impl(IoTHubMessage_SetConnectionModuleId);
}

// Tests_SRS_IOTHUBMESSAGE_31_051: [IoTHubMessage_SetConnectionModuleId finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetConnectionModuleId_SUCCEED)
{
    set_string_succeeds_impl(IoTHubMessage_SetConnectionModuleId, TEST_CONNECTION_MODULE_ID);
}

// Tests_SRS_IOTHUBMESSAGE_31_049: [If the IOTHUB_MESSAGE_HANDLE ConnectionModuleId is not NULL, then the IOTHUB_MESSAGE_HANDLE ConnectionModuleId will be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetConnectionModuleId_ConnectionModuleId_Not_NULL_SUCCEED)
{
    set_string_string_already_allocated_succeeds_impl(IoTHubMessage_SetConnectionModuleId, TEST_CONNECTION_MODULE_ID2);
}

// Tests_SRS_IOTHUBMESSAGE_31_046: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetConnectionModuleId shall return a NULL value.]
TEST_FUNCTION(IoTHubMessage_GetConnectionModuleId_NULL_handle_Fails)
{
    get_string_NULL_handle_fails_impl(IoTHubMessage_GetConnectionModuleId);
}

// Tests_SRS_IOTHUBMESSAGE_31_047: [IoTHubMessage_GetConnectionModuleId shall return the ConnectionModuleId as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetConnectionModuleId_ConnectionModuleId_Not_Set_Fails)
{
    get_string_not_set_fails_impl(IoTHubMessage_GetConnectionModuleId);
}

// Tests_SRS_IOTHUBMESSAGE_31_047: [IoTHubMessage_GetConnectionModuleId shall return the ConnectionModuleId as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetConnectionModuleId_SUCCEED)
{
    get_string_succeeds_impl(IoTHubMessage_SetConnectionModuleId, IoTHubMessage_GetConnectionModuleId, TEST_CONNECTION_MODULE_ID);
}

// Tests_SRS_IOTHUBMESSAGE_31_054: [if any of the parameters are NULL then IoTHubMessage_SetConnectionDeviceId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetConnectionDeviceId_NULL_handle_Fails)
{
    set_string_NULL_handle_fails_impl(IoTHubMessage_SetConnectionDeviceId, TEST_CONNECTION_DEVICE_ID);
}

// Tests_SRS_IOTHUBMESSAGE_31_054: [if any of the parameters are NULL then IoTHubMessage_SetConnectionDeviceId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]
TEST_FUNCTION(IoTHubMessage_SetConnectionDeviceId_NULL_MessageId_Fails)
{
    set_string_NULL_string_fails_impl(IoTHubMessage_SetConnectionDeviceId);
}

// Tests_SRS_IOTHUBMESSAGE_31_057: [IoTHubMessage_SetConnectionDeviceId finishes successfully it shall return IOTHUB_MESSAGE_OK.]
TEST_FUNCTION(IoTHubMessage_SetConnectionDeviceId_SUCCEED)
{
    set_string_succeeds_impl(IoTHubMessage_SetConnectionDeviceId, TEST_CONNECTION_DEVICE_ID);
}

// Tests_SRS_IOTHUBMESSAGE_31_055: [If the IOTHUB_MESSAGE_HANDLE ConnectionDeviceId is not NULL, then the IOTHUB_MESSAGE_HANDLE ConnectionDeviceId will be deallocated.]
TEST_FUNCTION(IoTHubMessage_SetConnectionDeviceId_ConnectionDeviceId_Not_NULL_SUCCEED)
{
    set_string_string_already_allocated_succeeds_impl(IoTHubMessage_SetConnectionDeviceId, "output_string2");
}

// Tests_SRS_IOTHUBMESSAGE_31_052: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetConnectionDeviceId shall return a NULL value.]
TEST_FUNCTION(IoTHubMessage_GetConnectionDeviceId_NULL_handle_Fails)
{
    get_string_NULL_handle_fails_impl(IoTHubMessage_GetConnectionDeviceId);
}

// Tests_SRS_IOTHUBMESSAGE_31_053: [IoTHubMessage_GetConnectionDeviceId shall return the ConnectionDeviceId as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetConnectionDeviceId_ConnectionDeviceId_Not_Set_Fails)
{
    get_string_not_set_fails_impl(IoTHubMessage_GetConnectionDeviceId);
}

// Tests_SRS_IOTHUBMESSAGE_31_053: [IoTHubMessage_GetConnectionDeviceId shall return the ConnectionDeviceId as a const char*.]
TEST_FUNCTION(IoTHubMessage_GetConnectionDeviceId_SUCCEED)
{
    get_string_succeeds_impl(IoTHubMessage_SetConnectionDeviceId, IoTHubMessage_GetConnectionDeviceId, TEST_CONNECTION_DEVICE_ID);
}

TEST_FUNCTION(IoTHubMessage_SetAsSecurityMessage_handle_NULL_fail)
{
    //arrange

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetAsSecurityMessage(NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubMessage_SetAsSecurityMessage_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    IOTHUB_MESSAGE_RESULT result = IoTHubMessage_SetAsSecurityMessage(h);

    //assert
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_Clone_SecurityMessage_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IoTHubMessage_SetAsSecurityMessage(h);
    umock_c_reset_all_calls();

    //act
    IOTHUB_MESSAGE_HANDLE clone_msg = IoTHubMessage_Clone(h);
    bool result = IoTHubMessage_IsSecurityMessage(h);

    //assert
    ASSERT_IS_TRUE(result);

    //cleanup
    IoTHubMessage_Destroy(h);
    IoTHubMessage_Destroy(clone_msg);
}

TEST_FUNCTION(IoTHubMessage_IsSecurityMessage_handle_NULL_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    bool result = IoTHubMessage_IsSecurityMessage(NULL);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubMessage_IsSecurityMessage_true_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    IoTHubMessage_SetAsSecurityMessage(h);
    umock_c_reset_all_calls();

    //act
    bool result = IoTHubMessage_IsSecurityMessage(h);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

TEST_FUNCTION(IoTHubMessage_IsSecurityMessage_false_Succeed)
{
    //arrange
    IOTHUB_MESSAGE_HANDLE h = IoTHubMessage_CreateFromByteArray(c, 1);
    umock_c_reset_all_calls();

    //act
    bool result = IoTHubMessage_IsSecurityMessage(h);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubMessage_Destroy(h);
}

END_TEST_SUITE(iothubmessage_ut)


