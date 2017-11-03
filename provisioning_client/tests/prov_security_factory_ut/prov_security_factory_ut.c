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
#include "umock_c.h"
#include "umocktypes_charptr.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "hsm_client_tpm_abstract.h"
#include "hsm_client_x509_abstract.h"

#define DPS_SECURITY_FACTORY_H
#include "azure_prov_client/iothub_security_factory.h"
#undef DPS_SECURITY_FACTORY_H

#undef ENABLE_MOCKS

#include "azure_prov_client/prov_security_factory.h"

TEST_DEFINE_ENUM_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(prov_security_factory_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE);
    REGISTER_TYPE(SECURE_DEVICE_TYPE, SECURE_DEVICE_TYPE);

    //REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_RETURN(hsm_init_x509_system, 0);
    REGISTER_GLOBAL_MOCK_RETURN(hsm_client_tpm_init, 0);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(iothub_security_type, IOTHUB_SECURITY_TYPE_UNKNOWN);
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

TEST_FUNCTION(prov_dev_security_init_sas_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type());
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_SAS));
    STRICT_EXPECTED_CALL(hsm_client_tpm_init());

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
    STRICT_EXPECTED_CALL(hsm_client_tpm_init());

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

TEST_FUNCTION(prov_dev_security_init_x509_success)
{
    //arrange
    STRICT_EXPECTED_CALL(iothub_security_type());
    STRICT_EXPECTED_CALL(iothub_security_init(IOTHUB_SECURITY_TYPE_X509));
    STRICT_EXPECTED_CALL(hsm_init_x509_system());

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
    STRICT_EXPECTED_CALL(hsm_init_x509_system());

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

TEST_FUNCTION(prov_dev_security_deinit_tpm_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_TPM);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(hsm_client_tpm_deinit());

    //act
    prov_dev_security_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_dev_security_deinit_x509_success)
{
    (void)prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(hsm_deinit_x509_system());

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

END_TEST_SUITE(prov_security_factory_ut)
