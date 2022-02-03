// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#else
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#endif

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
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
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#undef ENABLE_MOCKS

#include "hsm_client_x509.h"
#include "hsm_client_data.h"

static const char* TEST_CERTIFICATE_VALUE = 
"-----BEGIN CERTIFICATE-----\n"
"MIIBVjCB/aADAgECAhRMqqc/rOqEW+Afbkw4XyMLu1PUaTAKBggqhkjOPQQDAjAT\n"
"MREwDwYDVQQDDAhkZXYxLWVjYzAeFw0yMTExMTAwMDUxNTZaFw0yMjExMTAwMDUx\n"
"XYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZX\n"
"EwYDVR0lBAwwCgYIKwYBBQUHAwIwCgYIKoZIzj0EAwIDSAAwRQIgNp9ptsv8tJDk\n"
"keusij1tWWKt4dyz4U0sA8t+XwTldAsCIQDPoqnY0oVw8xDVVN3y/9IYmZvZdrNQ\n"
"dwjNGPacV5zzgA==\n"
"-----END CERTIFICATE-----\n";

static const char* TEST_KEY_VALUE = 
"-----BEGIN EC PARAMETERS-----\n"
"BggqhkjOPQMBBw==\n"
"-----END EC PARAMETERS-----\n"
"-----BEGIN EC PRIVATE KEY-----\n"
"MHcCAQEEIOJWKjjKkxL+m+FMJ6XdnpWI7OrHu4d0ZHKCDAOPGNTxoAoGCCqGSM49\n"
"XYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZXYZX\n"
"xuXxTAVynXrA6UKoIT3a9mB2VbNQlwGPyQ==\n"
"-----END EC PRIVATE KEY-----\n";

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(hsm_client_x509_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
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

// Unit: hsm_client_x509_create

TEST_FUNCTION(hsm_client_x509_create_succeed)
{
    //arrange
    hsm_client_x509_init();

    //act
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    HSM_CLIENT_HANDLE sec_handle2 = hsm_client_x509_create();

    //assert
    ASSERT_IS_NOT_NULL(sec_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    // We support a single X509 HSM module (singleton).
    ASSERT_ARE_EQUAL(void_ptr, sec_handle, sec_handle2);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

// Unit: hsm_client_x509_destroy

TEST_FUNCTION(hsm_client_x509_destroy_succeed)
{
    //arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();

    //act
    hsm_client_x509_destroy(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_destroy_handle_NULL_succeed)
{
    // arrange
    hsm_client_x509_init();
    umock_c_reset_all_calls();

    //act
    hsm_client_x509_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    hsm_client_x509_deinit();
}

// Unit: hsm_client_x509_set_certificate

TEST_FUNCTION(hsm_client_x509_set_certificate_succeed)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    int ret = hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_certificate_null_handle_fail)
{
    // arrange
    hsm_client_x509_init();
    umock_c_reset_all_calls();
    
    //act
    int ret = hsm_client_x509_set_certificate(NULL, TEST_CERTIFICATE_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_certificate_null_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();
    
    //act
    int ret = hsm_client_x509_set_certificate(sec_handle, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_certificate_twice_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    int ret = hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);
    ret = hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_certificate_oom_fail)
{
    // arrange
    umock_c_negative_tests_init();
    umock_c_negative_tests_snapshot();

    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_negative_tests_reset();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_fail_call(0);

    //act
    int ret = hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();

    umock_c_negative_tests_deinit();
}

// Unit: hsm_client_x509_set_key

TEST_FUNCTION(hsm_client_x509_set_key_succeed)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    int ret = hsm_client_x509_set_key(sec_handle, TEST_KEY_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_key_null_handle_fail)
{
    // arrange
    hsm_client_x509_init();
    umock_c_reset_all_calls();
    
    //act
    int ret = hsm_client_x509_set_key(NULL, TEST_KEY_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_key_null_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();
    
    //act
    int ret = hsm_client_x509_set_key(sec_handle, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_key_twice_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    int ret = hsm_client_x509_set_key(sec_handle, TEST_KEY_VALUE);
    ret = hsm_client_x509_set_key(sec_handle, TEST_KEY_VALUE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_set_key_oom_fail)
{
    // arrange
    umock_c_negative_tests_init();
    umock_c_negative_tests_snapshot();

    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_negative_tests_reset();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_fail_call(0);

    //act
    int ret = hsm_client_x509_set_key(sec_handle, TEST_KEY_VALUE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();

    umock_c_negative_tests_deinit();
}

// Unit: hsm_client_x509_get_certificate

TEST_FUNCTION(hsm_client_x509_get_certificate_succeed)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    char* ret = hsm_client_x509_get_certificate(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_CERTIFICATE_VALUE, ret);

    //cleanup
    free(ret);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_certificate_null_fail)
{
    // arrange
    hsm_client_x509_init();
    umock_c_reset_all_calls();
    
    //act
    char* ret = hsm_client_x509_get_certificate(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_certificate_not_set_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    
    umock_c_reset_all_calls();
    
    //act
    char* ret = hsm_client_x509_get_certificate(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_certificate_oom_fail)
{
    // arrange
    umock_c_negative_tests_init();
    umock_c_negative_tests_snapshot();

    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    hsm_client_x509_set_certificate(sec_handle, TEST_CERTIFICATE_VALUE);
    umock_c_negative_tests_reset();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_fail_call(0);

    //act
    char* ret = hsm_client_x509_get_certificate(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();

    umock_c_negative_tests_deinit();
}

// Unit: hsm_client_x509_get_key

TEST_FUNCTION(hsm_client_x509_get_key_succeed)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    hsm_client_x509_set_key(sec_handle, TEST_CERTIFICATE_VALUE);

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    
    //act
    char* ret = hsm_client_x509_get_key(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_CERTIFICATE_VALUE, ret);

    //cleanup
    free(ret);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_key_null_fail)
{
    // arrange
    hsm_client_x509_init();
    umock_c_reset_all_calls();
    
    //act
    char* ret = hsm_client_x509_get_key(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_key_not_set_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    
    umock_c_reset_all_calls();
    
    //act
    char* ret = hsm_client_x509_get_key(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();
}

TEST_FUNCTION(hsm_client_x509_get_key_oom_fail)
{
    // arrange
    umock_c_negative_tests_init();
    umock_c_negative_tests_snapshot();

    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    hsm_client_x509_set_key(sec_handle, TEST_CERTIFICATE_VALUE);
    umock_c_negative_tests_reset();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_fail_call(0);

    //act
    char* ret = hsm_client_x509_get_key(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, NULL, ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();

    umock_c_negative_tests_deinit();
}

// Unit: hsm_client_x509_get_common_name

TEST_FUNCTION(hsm_client_x509_get_common_name_always_fail)
{
    // arrange
    hsm_client_x509_init();
    HSM_CLIENT_HANDLE sec_handle = hsm_client_x509_create();
    umock_c_reset_all_calls();

    //act
    char* ret = hsm_client_x509_get_common_name(sec_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    // If X509 certificate authentication is used, the application MUST set PROV_REGISTRATION_ID.
    // If PROV_REGISTRATION_ID is set, get common name is never called by the SDK.
    ASSERT_IS_NULL(ret);

    //cleanup
    hsm_client_x509_destroy(sec_handle);
    hsm_client_x509_deinit();

    umock_c_negative_tests_deinit();
}

// Unit: hsm_client_x509_interface

TEST_FUNCTION(hsm_client_x509_interface_succeed)
{
    //arrange
    //act
    const HSM_CLIENT_X509_INTERFACE* x509_interface = hsm_client_x509_interface();

    //assert
    ASSERT_IS_NOT_NULL(x509_interface);
    ASSERT_IS_NOT_NULL(x509_interface->hsm_client_x509_create);
    ASSERT_IS_NOT_NULL(x509_interface->hsm_client_x509_destroy);
    ASSERT_IS_NOT_NULL(x509_interface->hsm_client_get_cert);
    ASSERT_IS_NOT_NULL(x509_interface->hsm_client_get_key);
    ASSERT_IS_NOT_NULL(x509_interface->hsm_client_get_common_name);

    //cleanup
}

END_TEST_SUITE(hsm_client_x509_ut)
