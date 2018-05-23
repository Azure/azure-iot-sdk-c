// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <stdlib.h>
#else
#include <stdlib.h>
#endif

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/xio.h"

#ifdef USE_PROV_MODULE
#include "azure_prov_client/internal/iothub_auth_client.h"
#endif

#include "azure_c_shared_utility/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "internal/iothub_client_authorization.h"

static const char* DEVICE_ID = "device_id";
static const char* DEVICE_KEY = "device_key";
static const char* SCOPE_NAME = "Scope_name";
static const char* TEST_SAS_TOKEN = "sas_token";
static const char* TEST_STRING_VALUE = "Test_string_value";
static const char* TEST_KEYNAME_VALUE = "Test_keyname_value";
static size_t TEST_EXPIRY_TIME = 1;

#define TEST_TIME_VALUE                     (time_t)123456

TEST_DEFINE_ENUM_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALUES);

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len+1);
    strcpy(*destination, source);
    return 0;
}

static STRING_HANDLE my_SASToken_CreateString(const char* key, const char* scope, const char* keyName, size_t expiry)
{
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}


static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothub_client_authorization_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_AUTHORIZATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(time_t, long long);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(XDA_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_SECURITY_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

    REGISTER_GLOBAL_MOCK_HOOK(SASToken_CreateString, my_SASToken_CreateString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_CreateString, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(get_time, TEST_TIME_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(get_time, ((time_t)(-1)));

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_RETURN(SASToken_Validate, true);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static void setup_IoTHubClient_Auth_Create_mocks(bool device_key)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (device_key)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DEVICE_KEY));
    }
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DEVICE_ID));
}

static void setup_IoTHubClient_Auth_Get_ConnString_mocks()
{
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(get_difftime(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(SASToken_CreateString(IGNORED_PTR_ARG, SCOPE_NAME, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = __LINE__;
            break;
        }
    }
    return result;
}

/* Codes_SRS_IoTHub_Authorization_07_001: [if device_key or device_id is NULL IoTHubClient_Auth_Create, shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_id_NULL_succeed)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, NULL, NULL);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_002: [IoTHubClient_Auth_Create shall allocate a IOTHUB_AUTHORIZATION_HANDLE that is needed for subsequent calls. ] */
/* Codes_SRS_IoTHub_Authorization_07_003: [ IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY. ] */
/* Codes_SRS_IoTHub_Authorization_07_004: [ If successful IoTHubClient_Auth_Create shall return a IOTHUB_AUTHORIZATION_HANDLE value. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_succeed)
{
    //arrange
    umock_c_reset_all_calls();

    setup_IoTHubClient_Auth_Create_mocks(true);

    //act
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Tests_SRS_IoTHub_Authorization_07_024: [ if device_sas_token and device_key are NULL IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_UNKNOWN. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_unknown_status_succeed)
{
    //arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DEVICE_ID));

    //act
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(NULL, DEVICE_ID, NULL);
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(handle);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_002: [IoTHubClient_Auth_Create shall allocate a IOTHUB_AUTHORIZATION_HANDLE that is needed for subsequent calls. ] */
/* Codes_SRS_IoTHub_Authorization_07_003: [ IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY. ] */
/* Codes_SRS_IoTHub_Authorization_07_004: [ If successful IoTHubClient_Auth_Create shall return a IOTHUB_AUTHORIZATION_HANDLE value. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_device_key_NULL_succeed)
{
    //arrange
    umock_c_reset_all_calls();

    setup_IoTHubClient_Auth_Create_mocks(false);

    //act
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(NULL, DEVICE_ID, NULL);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_002: [IoTHubClient_Auth_Create shall allocate a IOTHUB_AUTHORIZATION_HANDLE that is needed for subsequent calls. ] */
/* Codes_SRS_IoTHub_Authorization_07_004: [ If successful IoTHubClient_Auth_Create shall return a IOTHUB_AUTHORIZATION_HANDLE value. ] */
/* Codes_SRS_IoTHub_Authorization_07_020: [ else IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_with_sas_succeed)
{
    //arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DEVICE_ID));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, TEST_SAS_TOKEN));

    //act
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(NULL, DEVICE_ID, TEST_SAS_TOKEN);
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(handle);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_019: [ On error IoTHubClient_Auth_Create shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Create_fail)
{
    //arrange
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_IoTHubClient_Auth_Create_mocks(true);

    umock_c_negative_tests_snapshot();

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_Auth_Create failure in test %zu/%zu", index, count);

        IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);

        //assert
        ASSERT_IS_NULL(handle);
    }
    //cleanup
    umock_c_negative_tests_deinit();
}

/* Codes_SRS_IoTHub_Authorization_07_005: [ if handle is NULL IoTHubClient_Auth_Destroy shall do nothing. ] */
TEST_FUNCTION(IoTHubClient_Auth_Destroy_handle_NULL_succeed)
{
    //arrange

    //act
    IoTHubClient_Auth_Destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_006: [ IoTHubClient_Auth_Destroy shall free all resources associated with the IOTHUB_AUTHORIZATION_HANDLE handle. ] */
TEST_FUNCTION(IoTHubClient_Auth_Destroy_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

#ifdef USE_PROV_MODULE
    STRICT_EXPECTED_CALL(iothub_device_auth_destroy(IGNORED_PTR_ARG));
#endif
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    IoTHubClient_Auth_Destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_Auth_Set_x509_Type_do_nothing)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Set_x509_Type(NULL, true);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubClient_Auth_Set_x509_Type_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(NULL, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Set_x509_Type(handle, true);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

TEST_FUNCTION(IoTHubClient_Auth_Set_x509_Type_no_x509_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    (void)IoTHubClient_Auth_Set_x509_Type(handle, true);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Set_x509_Type(handle, false);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}


/* Codes_SRS_IoTHub_Authorization_07_007: [ if handle is NULL IoTHub_Auth_Get_Credential_Type shall return IOTHUB_CREDENTIAL_TYPE_UNKNOWN. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_Credential_Type_handle_NULL)
{
    //arrange

    //act
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_UNKNOWN);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_008: [ IoTHub_Auth_Get_Credential_Type shall return the credential type that is set upon creation. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_Credential_Type_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(handle);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CREDENTIAL_TYPE, cred_type, IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_009: [ if handle or scope are NULL, IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_ConnString_handle_NULL)
{
    //arrange

    //act
    char* conn_string = IoTHubClient_Auth_Get_SasToken(NULL, SCOPE_NAME, TEST_EXPIRY_TIME, TEST_KEYNAME_VALUE);

    //assert
    ASSERT_IS_NULL(conn_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_009: [ if handle or scope are NULL, IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_ConnString_scope_NULL_fail)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    char* conn_string = IoTHubClient_Auth_Get_SasToken(handle, NULL, TEST_EXPIRY_TIME, TEST_KEYNAME_VALUE);

    //assert
    ASSERT_IS_NULL(conn_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_010: [ IoTHubClient_Auth_Get_ConnString shall construct the expiration time using the expire_time. ] */
/* Codes_SRS_IoTHub_Authorization_07_011: [ IoTHubClient_Auth_Get_ConnString shall call SASToken_CreateString to construct the sas token. ] */
/* Codes_SRS_IoTHub_Authorization_07_012: [ On success IoTHubClient_Auth_Get_ConnString shall allocate and return the sas token in a char*. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_ConnString_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    setup_IoTHubClient_Auth_Get_ConnString_mocks();

    //act
    char* conn_string = IoTHubClient_Auth_Get_SasToken(handle, SCOPE_NAME, TEST_EXPIRY_TIME, NULL);

    //assert
    ASSERT_IS_NOT_NULL(conn_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(conn_string);
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_020: [ If any error is encountered IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_ConnString_fail)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    setup_IoTHubClient_Auth_Get_ConnString_mocks();

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3, 5 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail)/sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubClient_Auth_Get_ConnString failure in test %zu/%zu", index, count);

        //act
        char* conn_string = IoTHubClient_Auth_Get_SasToken(handle, SCOPE_NAME, TEST_EXPIRY_TIME, TEST_KEYNAME_VALUE);

        //assert
        ASSERT_IS_NULL(conn_string);
    }
    //cleanup
    IoTHubClient_Auth_Destroy(handle);
    umock_c_negative_tests_deinit();
}

/* Codes_SRS_IoTHub_Authorization_07_013: [ if handle is NULL, IoTHubClient_Auth_Get_DeviceId shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_DeviceId_handle_NULL)
{
    //arrange

    //act
    const char* device_id = IoTHubClient_Auth_Get_DeviceId(NULL);

    //assert
    ASSERT_IS_NULL(device_id);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_014: [ IoTHubClient_Auth_Get_DeviceId shall return the device_id specified upon creation. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_DeviceId_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    const char* device_id = IoTHubClient_Auth_Get_DeviceId(handle);

    //assert
    ASSERT_IS_NOT_NULL(device_id);
    ASSERT_ARE_EQUAL(char_ptr, DEVICE_ID, device_id);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_022: [ if handle is NULL, IoTHubClient_Auth_Get_DeviceKey shall return NULL. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_DeviceKey_handle_NULL)
{
    //arrange

    //act
    const char* device_key = IoTHubClient_Auth_Get_DeviceKey(NULL);

    //assert
    ASSERT_IS_NULL(device_key);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_023: [ IoTHubClient_Auth_Get_DeviceKey shall return the device_key specified upon creation. ] */
TEST_FUNCTION(IoTHubClient_Auth_Get_Devicekey_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    const char* device_key = IoTHubClient_Auth_Get_DeviceKey(handle);

    //assert
    ASSERT_IS_NOT_NULL(device_key);
    ASSERT_ARE_EQUAL(char_ptr, DEVICE_KEY, device_key);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_015: [ if handle is NULL, IoTHubClient_Auth_Is_SasToken_Valid shall return false. ] */
TEST_FUNCTION(IoTHubClient_Auth_Is_SasToken_Valid_handle_NULL_fail)
{
    //arrange

    //act
    SAS_TOKEN_STATUS is_valid = IoTHubClient_Auth_Is_SasToken_Valid(NULL);

    //assert
    ASSERT_ARE_EQUAL(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_FAILED, is_valid);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Codes_SRS_IoTHub_Authorization_07_018: [ otherwise IoTHubClient_Auth_Is_SasToken_Valid shall return the value returned by SASToken_Validate. ] */
TEST_FUNCTION(IoTHubClient_Auth_Is_SasToken_Valid_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(DEVICE_KEY, DEVICE_ID, NULL);
    umock_c_reset_all_calls();

    //act
    SAS_TOKEN_STATUS is_valid = IoTHubClient_Auth_Is_SasToken_Valid(handle);

    //assert
    ASSERT_ARE_EQUAL(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALID, is_valid);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

/* Codes_SRS_IoTHub_Authorization_07_018: [ otherwise IoTHubClient_Auth_Is_SasToken_Valid shall return the value returned by SASToken_Validate. ] */
TEST_FUNCTION(IoTHubClient_Auth_Is_SasToken_Valid_sas_token_succeed)
{
    //arrange
    IOTHUB_AUTHORIZATION_HANDLE handle = IoTHubClient_Auth_Create(NULL, DEVICE_ID, TEST_SAS_TOKEN);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_construct(TEST_SAS_TOKEN));
    STRICT_EXPECTED_CALL(SASToken_Validate(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    SAS_TOKEN_STATUS is_valid = IoTHubClient_Auth_Is_SasToken_Valid(handle);

    //assert
    ASSERT_ARE_EQUAL(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALID, is_valid);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubClient_Auth_Destroy(handle);
}

END_TEST_SUITE(iothub_client_authorization_ut)
