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
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umocktypes_stdint.h"
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#undef ENABLE_MOCKS

#include "hsm_client_key.h"
#include "hsm_client_data.h"

static const char* TEST_STRING_VALUE = "Test_String_Value";
static const unsigned char TEST_IMPORT_KEY[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10 };

static const char* TEST_RSA_KEY = "1234567890";
static unsigned char TEST_BUFFER[128];

#define TEST_BUFFER_SIZE    128
#define TEST_KEY_SIZE       10

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(hsm_client_key_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        int result;

        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);
        result = umocktypes_stdint_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURE_DEVICE_TYPE, int);

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

    TEST_FUNCTION(hsm_client_key_create_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        HSM_CLIENT_HANDLE sec_handle = hsm_client_key_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_key_destroy(sec_handle);
    }

    TEST_FUNCTION(hsm_client_key_create_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        HSM_CLIENT_HANDLE sec_handle = hsm_client_key_create();

        //assert
        ASSERT_IS_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_004: [ hsm_client_tpm_destroy shall free the SEC_DEVICE_INFO instance. ] */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_006: [ hsm_client_tpm_destroy shall free all resources allocated in this module. ]*/
    TEST_FUNCTION(hsm_client_key_destroy_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_key_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        hsm_client_key_destroy(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_005: [ if handle is NULL, hsm_client_tpm_destroy shall do nothing. ] */
    TEST_FUNCTION(hsm_client_key_destroy_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        hsm_client_key_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(hsm_client_get_symmetric_key_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_key_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        char* key_value = hsm_client_get_symmetric_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(key_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_key_destroy(sec_handle);
        free(key_value);
    }

    TEST_FUNCTION(hsm_client_get_symmetric_key_handle_NULL_fail)
    {
        //arrange

        //act
        char* key_value = hsm_client_get_symmetric_key(NULL);

        //assert
        ASSERT_IS_NULL(key_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(hsm_client_get_registration_name_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_key_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        char* reg_name = hsm_client_get_registration_name(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(reg_name);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_key_destroy(sec_handle);
        free(reg_name);
    }

    TEST_FUNCTION(hsm_client_get_registration_name_handle_NULL_fail)
    {
        //arrange

        //act
        char* key_value = hsm_client_get_registration_name(NULL);

        //assert
        ASSERT_IS_NULL(key_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(hsm_client_key_interface_succeed)
    {
        //arrange
        //act
        const HSM_CLIENT_KEY_INTERFACE* key_interface = hsm_client_key_interface();

        //assert
        ASSERT_IS_NOT_NULL(key_interface);
        ASSERT_IS_NOT_NULL(key_interface->hsm_client_key_create);
        ASSERT_IS_NOT_NULL(key_interface->hsm_client_key_destroy);
        ASSERT_IS_NOT_NULL(key_interface->hsm_client_get_symm_key);
        ASSERT_IS_NOT_NULL(key_interface->hsm_client_get_registration_name);

        //cleanup
    }

    END_TEST_SUITE(hsm_client_key_ut)
