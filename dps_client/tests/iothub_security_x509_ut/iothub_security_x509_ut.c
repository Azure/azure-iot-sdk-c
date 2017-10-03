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
#include "umocktypes_stdint.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#include "RIoT.h"
#include "RiotCrypt.h"
#include "RiotDerEnc.h"
#include "RiotX509Bldr.h"
#include "DiceSha256.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_hub_modules/iothub_security_factory.h"

#include "dps_hsm_riot.h"

#undef ENABLE_MOCKS

#include "iothub_security_x509.h"

static const char* TEST_STRING_VALUE = "Test Cert Value";
static const char* TEST_ALIAS_VALUE = "Test Alias Value";
static const char* TEST_CN_VALUE = "RIoT_Device";

static DPS_SECURE_DEVICE_HANDLE my_dps_hsm_riot_create(void)
{
    return (DPS_SECURE_DEVICE_HANDLE)my_gballoc_malloc(1);
}

static void my_dps_hsm_riot_destroy(DPS_SECURE_DEVICE_HANDLE handle)
{
    my_gballoc_free(handle);
}

static char* my_dps_hsm_riot_get_certificate(DPS_SECURE_DEVICE_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static char* my_dps_hsm_riot_get_alias_key(DPS_SECURE_DEVICE_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_ALIAS_VALUE);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_ALIAS_VALUE);
    return result;
}

static int umocktypes_copy_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* dest, const RIOT_ECC_PRIVATE* src)
{
    int result;
    if (src == NULL)
    {
        dest = NULL;
        result = 0;
    }
    else
    {
        memcpy(dest->data, src->data, sizeof(dest->data));
        result = 0;
    }
    return result;
}

static void umocktypes_free_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* value)
{
    (void)value;
}

static char* umocktypes_stringify_RIOT_ECC_PRIVATE(const RIOT_ECC_PRIVATE* value)
{
    char* result;
    if (value == NULL)
    {
        result = (char*)my_gballoc_malloc(5);
        if (result != NULL)
        {
            (void)memcpy(result, "NULL", 5);
        }
    }
    else
    {
        int length = snprintf(NULL, 0, "{ %p }", value->data);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            result = (char*)my_gballoc_malloc(length + 1);
            (void)snprintf(result, length + 1, "{ %p }", value->data);
        }
    }
    return result;
}

static int umocktypes_are_equal_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* left, RIOT_ECC_PRIVATE* right)
{
    int result;
    if (left == right)
    {
        result = 1;
    }
    else if ((left == NULL) || (right == NULL))
    {
        result = 0;
    }
    else
    {
        result = memcmp(left->data, right->data, sizeof(left->data));
    }
    return result;
}

static int umocktypes_copy_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* dest, const RIOT_ECC_PUBLIC* src)
{
    int result;
    if (src == NULL)
    {
        dest = NULL;
        result = 0;
    }
    else
    {
        dest->infinity = src->infinity;
        memcpy(dest->x.data, src->x.data, sizeof(dest->x.data) );
        memcpy(dest->y.data, src->x.data, sizeof(dest->y.data));
        result = 0;
    }
    return result;
}

static void umocktypes_free_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* value)
{
    (void)value;
}

static char* umocktypes_stringify_RIOT_ECC_PUBLIC(const RIOT_ECC_PUBLIC* value)
{
    char* result;
    if (value == NULL)
    {
        result = (char*)my_gballoc_malloc(5);
        if (result != NULL)
        {
            (void)memcpy(result, "NULL", 5);
        }
    }
    else
    {
        int length = snprintf(NULL, 0, "{ %p, %p, %d }",
            value->x.data, value->y.data, (int)value->infinity);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            result = (char*)my_gballoc_malloc(length + 1);
            (void)snprintf(result, length + 1, "{ %p, %p, %d }",
                value->x.data, value->y.data, (int)value->infinity);
        }
    }
    return result;
}

static int umocktypes_are_equal_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* left, RIOT_ECC_PUBLIC* right)
{
    int result;
    if (left == right)
    {
        result = 1;
    }
    else if ((left == NULL) || (right == NULL))
    {
        result = 0;
    }
    else
    {
        result = memcmp(left->x.data, right->x.data, sizeof(left->x.data));
    }
    return result;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static int my_DERtoPEM(DERBuilderContext* Context, uint32_t Type, char* PEM, uint32_t* Length)
{
    (void)Context;
    (void)Type;

    strcpy(PEM, TEST_STRING_VALUE);
    *Length = (uint32_t)strlen(PEM);
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

BEGIN_TEST_SUITE(iothub_security_x509_ut)

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

        REGISTER_UMOCK_ALIAS_TYPE(SECURITY_DEVICE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DPS_SECURE_DEVICE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURE_DEVICE_TYPE, int);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(dps_hsm_riot_create, my_dps_hsm_riot_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_hsm_riot_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_hsm_riot_get_certificate, my_dps_hsm_riot_get_certificate);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_hsm_riot_get_certificate, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_hsm_riot_get_alias_key, my_dps_hsm_riot_get_alias_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(dps_hsm_riot_get_alias_key, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(dps_hsm_riot_destroy, my_dps_hsm_riot_destroy);
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

    static void iothub_security_x509_create_mock()
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(dps_hsm_riot_create());
        STRICT_EXPECTED_CALL(dps_hsm_riot_get_certificate(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_hsm_riot_get_alias_key(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(dps_hsm_riot_destroy(IGNORED_PTR_ARG));
    }

    /* Tests_SRS_IOTHUB_SECURITY_x509_07_001: [ On success iothub_security_x509_create shall allocate a new instance of the SECURITY_DEVICE_HANDLE interface. ] */
    /* Tests_SRS_IOTHUB_SECURITY_x509_07_002: [ iothub_security_x509_create shall call into the dps RIoT module to retrieve the DPS_SECURE_DEVICE_HANDLE. ]*/
    /* Tests_SRS_IOTHUB_SECURITY_x509_07_003: [ iothub_security_x509_create shall cache the x509_certificate from the RIoT module. ]*/
    /* Tests_SRS_IOTHUB_SECURITY_x509_07_004: [ iothub_security_x509_create shall cache the x509_alias_key from the RIoT module. ]*/
    /* Tests_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ]*/
    TEST_FUNCTION(secure_device_riot_create_succeed)
    {
        //arrange
        iothub_security_x509_create_mock();

        //act
        SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_security_x509_destroy(sec_handle);
    }

    /* Tests_SRS_IOTHUB_SECURITY_x509_07_006: [ If any failure is encountered iothub_security_x509_create shall return NULL ] */
    TEST_FUNCTION(iothub_security_x509_create_fail)
    {
        //arrange
        iothub_security_x509_create_mock();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 4 };

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            {
                continue;
            }

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "iothub_security_x509_create failure in test %zu/%zu", index, count);

            //act
            SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();

            //assert
            ASSERT_IS_NULL_WITH_MSG(sec_handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_IOTHUB_SECURITY_x509_07_008: [ iothub_security_x509_destroy shall free the SECURITY_DEVICE_HANDLE instance. ] */
    /* Tests_SRS_IOTHUB_SECURITY_x509_07_009: [ iothub_security_x509_destroy shall free all resources allocated in this module. ] */
    TEST_FUNCTION(iothub_security_x509_destroy_succeed)
    {
        //arrange
        SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(sec_handle));

        //act
        iothub_security_x509_destroy(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_IOTHUB_SECURITY_x509_07_007: [ if handle is NULL, iothub_security_x509_destroy shall do nothing. ] */
    TEST_FUNCTION(iothub_security_x509_destroy_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        iothub_security_x509_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Codes_SRS_IOTHUB_SECURITY_x509_07_010: [ if handle is NULL, iothub_security_x509_get_certificate shall return NULL. ] */
    TEST_FUNCTION(iothub_security_x509_get_certificate_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        const char* value = iothub_security_x509_get_certificate(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Codes_SRS_IOTHUB_SECURITY_x509_07_011: [ iothub_security_x509_get_certificate shall return the cached riot certificate. ] */
    TEST_FUNCTION(iothub_security_x509_get_certificate_succeed)
    {
        //arrange
        SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();
        umock_c_reset_all_calls();

        //act
        const char* value = iothub_security_x509_get_certificate(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_security_x509_destroy(sec_handle);
    }

    /* Codes_SRS_IOTHUB_SECURITY_x509_07_014: [ if handle is NULL, secure_device_riot_get_alias_key shall return NULL. ] */
    TEST_FUNCTION(iothub_security_x509_get_alias_key_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        const char* value = iothub_security_x509_get_alias_key(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Codes_SRS_IOTHUB_SECURITY_x509_07_015: [ secure_device_riot_get_alias_key shall allocate a char* to return the alias certificate. ] */
    TEST_FUNCTION(iothub_security_x509_get_alias_key_succeed)
    {
        //arrange
        SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();
        umock_c_reset_all_calls();

        //act
        const char* value = iothub_security_x509_get_alias_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_ALIAS_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_security_x509_destroy(sec_handle);
    }

    /* Codes_SRS_IOTHUB_SECURITY_x509_07_029: [ iothub_security_x509_interface shall return the X509_SECURITY_INTERFACE structure. ] */
    TEST_FUNCTION(iothub_security_x509_interface_succeed)
    {
        //arrange
        SECURITY_DEVICE_HANDLE sec_handle = iothub_security_x509_create();
        umock_c_reset_all_calls();

        //act
        const X509_SECURITY_INTERFACE* riot_iface = iothub_security_x509_interface();

        //assert
        ASSERT_IS_NOT_NULL(riot_iface);
        ASSERT_IS_NOT_NULL(riot_iface->secure_device_create);
        ASSERT_IS_NOT_NULL(riot_iface->secure_device_destroy);
        ASSERT_IS_NOT_NULL(riot_iface->secure_device_get_cert);
        ASSERT_IS_NOT_NULL(riot_iface->secure_device_get_ak);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_security_x509_destroy(sec_handle);
    }

    END_TEST_SUITE(iothub_security_x509_ut)
