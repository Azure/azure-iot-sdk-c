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
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "hsm_client_data.h"
MOCKABLE_FUNCTION(, int, initialize_hsm_system);
MOCKABLE_FUNCTION(, void, deinitialize_hsm_system);

#define DPS_SECURITY_FACTORY_H
#include "azure_prov_client/iothub_security_factory.h"
#undef DPS_SECURITY_FACTORY_H

#undef ENABLE_MOCKS

#include "azure_prov_client/prov_security_factory.h"

static const char* TEST_SYMM_KEY = "Test_symm_key";
static const char* TEST_REG_NAME = "Test_registration_name";

TEST_DEFINE_ENUM_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);

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

BEGIN_TEST_SUITE(prov_security_factory_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE);
    REGISTER_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE);

    //REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_RETURN(initialize_hsm_system, 0);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_init, 0);
    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_type, IOTHUB_SECURITY_TYPE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_set_symmetric_key_info, 0);
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

#if defined(HSM_TYPE_SAS_TOKEN)  || defined(HSM_AUTH_TYPE_CUSTOM)
TEST_FUNCTION(prov_dev_security_init_sas_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type());
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_SAS));
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_sas_types_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_SAS);
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_sas_types_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_sas_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}


TEST_FUNCTION(prov_dev_security_deinit_tpm_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(deinitialize_hsm_system());
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(TEST_SYMM_KEY);
    STRICT_EXPECTED_CALL(iothub_security_deinit());

    //act
    prov_dev_security_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}


TEST_FUNCTION(prov_dev_security_get_type_tpm_succees)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);
    umock_c_reset_all_calls();

    //arrange

    //act
    SECURE_DEVICE_TYPE result = prov_dev_security_get_type();

    //assert
    ASSERT_ARE_EQUAL(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_TPM, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}
#endif

#if defined(HSM_TYPE_X509) || defined(HSM_AUTH_TYPE_CUSTOM)
TEST_FUNCTION(prov_dev_security_init_x509_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type());
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_X509));
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_x509_types_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_x509_types_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_SAS);

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_X509);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_deinit_x509_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(deinitialize_hsm_system());
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(iothub_security_get_symm_registration_name()).SetReturn(NULL);

    //act
    prov_dev_security_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_get_type_x509_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
    umock_c_reset_all_calls();

    //arrange

    //act
    SECURE_DEVICE_TYPE result = prov_dev_security_get_type();

    //assert
    ASSERT_ARE_EQUAL(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_X509, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}
#endif

TEST_FUNCTION(prov_dev_set_symmetric_key_info_success)
{
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(iothub_security_set_symmetric_key_info(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_call_2_success)
{
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(TEST_SYMM_KEY);
    STRICT_EXPECTED_CALL(iothub_security_get_symm_registration_name()).SetReturn(TEST_REG_NAME);

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_reg_NULL_fail)
{
    umock_c_reset_all_calls();

    //arrange

    //act
    int result = prov_dev_set_symmetric_key_info(NULL, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_symm_NULL_fail)
{
    umock_c_reset_all_calls();

    //arrange

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_malloc_fail)
{
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_key_malloc_fail)
{
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(__LINE__);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

TEST_FUNCTION(prov_dev_set_symmetric_key_info_called_twice_success)
{
    (void)prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(TEST_SYMM_KEY);
    STRICT_EXPECTED_CALL(iothub_security_get_symm_registration_name()).SetReturn(TEST_REG_NAME);

    //act
    int result = prov_dev_set_symmetric_key_info(TEST_REG_NAME, TEST_SYMM_KEY);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_dev_security_deinit();
}

#ifdef HSM_TYPE_HTTP_EDGE
TEST_FUNCTION(prov_dev_security_init_http_edge_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type());
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_HTTP_EDGE));
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_HTTP_EDGE);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_http_edge_types_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_HTTP_EDGE);
    STRICT_EXPECTED_CALL(initialize_hsm_system());

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_HTTP_EDGE);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_init_http_edge_types_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_SAS);

    //act
    int result = prov_dev_security_init(SECURE_DEVICE_TYPE_HTTP_EDGE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_deinit_http_edge_success)
{
    STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_HTTP_EDGE);
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_HTTP_EDGE);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(deinitialize_hsm_system());
    STRICT_EXPECTED_CALL(iothub_security_get_symmetric_key()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(iothub_security_get_symm_registration_name()).SetReturn(NULL);

    //act
    prov_dev_security_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_get_type_http_edge_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_HTTP_EDGE);
    umock_c_reset_all_calls();

    //arrange

    //act
    SECURE_DEVICE_TYPE result = prov_dev_security_get_type();

    //assert
    ASSERT_ARE_EQUAL(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_HTTP_EDGE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}
#endif




END_TEST_SUITE(prov_security_factory_ut)
