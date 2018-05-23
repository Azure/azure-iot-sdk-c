// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
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

#include "azure_prov_client/internal/iothub_auth_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/urlencode.h"

#include "azure_prov_client/iothub_security_factory.h"
#include "hsm_client_data.h"

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, hsm_client_create);
MOCKABLE_FUNCTION(, void, hsm_client_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, hsm_client_import_key, HSM_CLIENT_HANDLE, handle, const unsigned char*, data, size_t, data_len);
MOCKABLE_FUNCTION(, int, hsm_client_get_ek, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, hsm_client_get_srk, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, hsm_client_sign_data, HSM_CLIENT_HANDLE, handle, const unsigned char*, data, size_t, data_len, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, char*, hsm_client_get_certificate, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_get_alias_key, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, hsm_client_get_common_name, HSM_CLIENT_HANDLE, handle);

MOCKABLE_FUNCTION(, const HSM_CLIENT_TPM_INTERFACE*, hsm_client_tpm_interface);
MOCKABLE_FUNCTION(, const HSM_CLIENT_X509_INTERFACE*, hsm_client_x509_interface);

#undef ENABLE_MOCKS

#ifdef __cplusplus
extern "C"
{
#endif
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }

    int my_size_tToString(char* destination, size_t destinationSize, size_t value)
    {
        (void)destinationSize;
        (void)value;
        sprintf(destination, "12345");
        return 0;
    }
#ifdef __cplusplus
}
#endif

#define TEST_CONCRETE_HANDLE (CONCRETE_XDA_HANDLE)0x11111111
#define TEST_PARAMETER_VALUE (void*)0x11111112
#define TEST_STRING_HANDLE_VALUE (STRING_HANDLE)0x11111113

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static char* TEST_STRING_VALUE = "Test_String_Value";
static char* TEST_CERT_VALUE = "Test_Cert_Value";

static const char* TEST_PARAM_INFO_VALUE = "Test Param Info";

TEST_DEFINE_ENUM_TYPE(DEVICE_AUTH_TYPE, DEVICE_AUTH_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DEVICE_AUTH_TYPE, DEVICE_AUTH_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE_VALUES);

static const HSM_CLIENT_TPM_INTERFACE test_tpm_interface = 
{
    hsm_client_create,
    hsm_client_destroy,
    hsm_client_import_key,
    hsm_client_get_ek,
    hsm_client_get_srk,
    hsm_client_sign_data
};

static const HSM_CLIENT_TPM_INTERFACE test_tpm_interface_fail =
{
    hsm_client_create,
    hsm_client_destroy,
    hsm_client_import_key,
    hsm_client_get_ek,
    hsm_client_get_srk,
    NULL
};

static const HSM_CLIENT_X509_INTERFACE test_x509_interface =
{
    hsm_client_create,
    hsm_client_destroy,
    hsm_client_get_certificate,
    hsm_client_get_alias_key,
    hsm_client_get_common_name
};

static const HSM_CLIENT_X509_INTERFACE test_x509_interface_fail =
{
    hsm_client_create,
    hsm_client_destroy,
    hsm_client_get_certificate,
    NULL,
    NULL
};

static HSM_CLIENT_HANDLE my_hsm_client_create(void)
{
    return (HSM_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static void my_hsm_client_destroy(HSM_CLIENT_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static char* my_hsm_client_get_alias_key(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static char* my_hsm_client_get_certificate(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_CERT_VALUE);
    char* result = (char*)my_gballoc_malloc(len+1);
    strcpy(result, TEST_CERT_VALUE);
    return result;
}

static char* my_hsm_client_get_signer_cert(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len+1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static int my_hsm_client_sign_data(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** key, size_t* key_len)
{
    (void)handle;
    (void)data;
    (void)data_len;
    *key = (unsigned char*)my_gballoc_malloc(1);
    *key_len = 1;
    return 0;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len+1);
    strcpy(*destination, source);
    return 0;
}

static STRING_HANDLE my_Base64_Encode_Bytes(const unsigned char* source, size_t length)
{
    (void)source;
    (void)length;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_Encode(STRING_HANDLE input)
{
    (void)input;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static DEVICE_AUTH_CREDENTIAL_INFO g_test_sas_cred;
static DEVICE_AUTH_CREDENTIAL_INFO g_test_sas_cred_no_keyname;
static DEVICE_AUTH_CREDENTIAL_INFO g_test_x509_cred;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(iothub_auth_client_ut)

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

        REGISTER_TYPE(IOTHUB_SECURITY_TYPE, IOTHUB_SECURITY_TYPE);

        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_SECURITY_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURITY_DEVICE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURE_DEVICE_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);

        REGISTER_GLOBAL_MOCK_HOOK(hsm_client_create, my_hsm_client_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(hsm_client_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(hsm_client_destroy, my_hsm_client_destroy);

        REGISTER_GLOBAL_MOCK_HOOK(hsm_client_get_alias_key, my_hsm_client_get_alias_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(hsm_client_get_alias_key, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(hsm_client_sign_data, my_hsm_client_sign_data);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(hsm_client_sign_data, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(hsm_client_get_certificate, my_hsm_client_get_certificate);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(hsm_client_get_certificate, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(Base64_Encode_Bytes, my_Base64_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Encode_Bytes, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(URL_Encode, my_URL_Encode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_Encode, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(size_tToString, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(iothub_security_type, IOTHUB_SECURITY_TYPE_SAS);
        REGISTER_GLOBAL_MOCK_RETURN(hsm_client_tpm_interface, &test_tpm_interface);
        REGISTER_GLOBAL_MOCK_RETURN(hsm_client_x509_interface, &test_x509_interface);

        g_test_sas_cred.dev_auth_type = AUTH_TYPE_SAS;
        g_test_sas_cred.sas_info.token_scope = "scope";
        g_test_sas_cred.sas_info.expiry_seconds = 123;
        g_test_sas_cred.sas_info.key_name = "key_name";

        g_test_sas_cred_no_keyname.dev_auth_type = AUTH_TYPE_SAS;
        g_test_sas_cred_no_keyname.sas_info.token_scope = "scope";
        g_test_sas_cred_no_keyname.sas_info.expiry_seconds = 123;
        g_test_sas_cred_no_keyname.sas_info.key_name = NULL;
        
        g_test_x509_cred.dev_auth_type = AUTH_TYPE_X509;
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

    static void setup_iothub_device_auth_generate_credentials_mocks(void)
    {
        STRICT_EXPECTED_CALL(size_tToString(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(hsm_client_sign_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_iothub_device_auth_generate_credentials_x509_mocks(void)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(hsm_client_get_certificate(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(hsm_client_get_alias_key(IGNORED_NUM_ARG));
    }

    /* Tests_IOTHUB_DEV_AUTH_07_003: [ If the function succeeds iothub_device_auth_create shall return a IOTHUB_SECURITY_HANDLE. ] */
    /* Tests_IOTHUB_DEV_AUTH_07_025: [ iothub_device_auth_create shall call the concrete_iothub_device_auth_create function associated with the XDA_INTERFACE_DESCRIPTION. ] */
    TEST_FUNCTION(iothub_device_auth_create_tpm_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_SAS);
        STRICT_EXPECTED_CALL(hsm_client_tpm_interface()).SetReturn(&test_tpm_interface);
        STRICT_EXPECTED_CALL(hsm_client_create());

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NOT_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_002: [ iothub_device_auth_create shall allocate the IOTHUB_SECURITY_INFO and shall fail if the allocation fails. ]*/
    TEST_FUNCTION(iothub_device_auth_create_tpm_interface_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_SAS);
        STRICT_EXPECTED_CALL(hsm_client_tpm_interface()).SetReturn(&test_tpm_interface_fail);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_025: [ iothub_device_auth_create shall call the concrete_iothub_device_auth_create function associated with the interface_desc. ] */
    /* Tests_IOTHUB_DEV_AUTH_07_026: [ if concrete_iothub_device_auth_create fails iothub_device_auth_create shall return NULL. ] */
    TEST_FUNCTION(iothub_device_auth_create_x509_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        STRICT_EXPECTED_CALL(hsm_client_create());

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NOT_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_002: [ iothub_device_auth_create shall allocate the IOTHUB_SECURITY_INFO and shall fail if the allocation fails. ]*/
    TEST_FUNCTION(iothub_device_auth_create_x509_Interface_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_002: [ iothub_device_auth_create shall allocate the XDA_INSTANCE and shall fail if the allocation fails. ] */
    TEST_FUNCTION(iothub_device_auth_create_malloc_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_026: [ if concrete_iothub_device_auth_create fails iothub_device_auth_create shall return NULL. ] */
    TEST_FUNCTION(iothub_device_auth_create_concrete_iothub_device_auth_create_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        STRICT_EXPECTED_CALL(hsm_client_create()).SetReturn(NULL);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();

        //assert
        ASSERT_IS_NULL(xda_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_004: [ iothub_device_auth_destroy shall free all resources associated with the IOTHUB_SECURITY_HANDLE handle ] */
    /* Tests_IOTHUB_DEV_AUTH_07_005: [ iothub_device_auth_destroy shall call the concrete_iothub_device_auth_destroy function associated with the XDA_INTERFACE_DESCRIPTION. ] */
    TEST_FUNCTION(iothub_device_auth_destroy_succeed)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(hsm_client_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(xda_handle));

        //act
        iothub_device_auth_destroy(xda_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_006: [ If the argument handle is NULL, iothub_device_auth_destroy shall do nothing ] */
    TEST_FUNCTION(iothub_device_auth_destroy_handle_NULL_succeed)
    {
        //arrange

        //act
        iothub_device_auth_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_007: [ If the argument handle is NULL, iothub_device_auth_get_auth_type shall return AUTH_TYPE_UNKNOWN. ] */
    TEST_FUNCTION(iothub_device_auth_get_auth_type_handle_NULL_fail)
    {
        //arrange

        //act
        DEVICE_AUTH_TYPE dev_auth_type = iothub_device_auth_get_type(NULL);

        //assert
        ASSERT_ARE_EQUAL(DEVICE_AUTH_TYPE, AUTH_TYPE_UNKNOWN, dev_auth_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_008: [ iothub_device_auth_get_auth_type shall call concrete_iothub_device_auth_type function associated with the XDA_INTERFACE_DESCRIPTION. ] */
    /* Tests_IOTHUB_DEV_AUTH_07_009: [ iothub_device_auth_get_auth_type shall return the DEVICE_AUTH_TYPE returned by the concrete_iothub_device_auth_type function. ] */
    TEST_FUNCTION(iothub_device_auth_get_auth_type_succeed)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        //act
        DEVICE_AUTH_TYPE dev_auth_type = iothub_device_auth_get_type(xda_handle);

        //assert
        ASSERT_ARE_EQUAL(DEVICE_AUTH_TYPE, AUTH_TYPE_SAS, dev_auth_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_010: [ If the argument handle, token_scope or sas_token is NULL, iothub_device_auth_generate_credentials shall return a non-zero value. ] */
    TEST_FUNCTION(iothub_device_auth_generate_credentials_handle_NULL_fail)
    {
        //arrange

        //act
        void* result = iothub_device_auth_generate_credentials(NULL, &g_test_sas_cred);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_IOTHUB_DEV_AUTH_07_010: [ If the argument handle, token_scope or sas_token is NULL, iothub_device_auth_generate_credentials shall return a non-zero value. ] */
    TEST_FUNCTION(iothub_device_auth_generate_credentials_cred_info_NULL_fail)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        //act
        void* result = iothub_device_auth_generate_credentials(xda_handle, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    TEST_FUNCTION(iothub_device_auth_generate_credentials_invalid_param_type_fail)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        DEVICE_AUTH_CREDENTIAL_INFO test_iothub_device_auth_credentials;
        test_iothub_device_auth_credentials.dev_auth_type = AUTH_TYPE_X509;

        //act
        void* result = iothub_device_auth_generate_credentials(xda_handle, &test_iothub_device_auth_credentials);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        iothub_device_auth_destroy(xda_handle);
    }

    /* Tests_IOTHUB_DEV_AUTH_07_011: [ iothub_device_auth_generate_credentials shall call concrete_iothub_device_auth_generate_sastoken function associated with the XDA_INTERFACE_DESCRIPTION. ]*/
    /* Tests_IOTHUB_DEV_AUTH_07_035: [ For tpm type iothub_device_auth_generate_credentials shall call the concrete_dev_auth_sign_data function to hash the data. ]*/
    TEST_FUNCTION(iothub_device_auth_generate_credentials_succeed)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        setup_iothub_device_auth_generate_credentials_mocks();

        //act
        void* result = iothub_device_auth_generate_credentials(xda_handle, &g_test_sas_cred);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        iothub_device_auth_destroy(xda_handle);
    }

    TEST_FUNCTION(iothub_device_auth_generate_credentials_no_key_succeed)
    {
        //arrange
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        setup_iothub_device_auth_generate_credentials_mocks();

        //act
        void* result = iothub_device_auth_generate_credentials(xda_handle, &g_test_sas_cred_no_keyname);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        iothub_device_auth_destroy(xda_handle);
    }

    TEST_FUNCTION(iothub_device_auth_generate_credentials_x509_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);

        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        setup_iothub_device_auth_generate_credentials_x509_mocks();

        //act
        CREDENTIAL_RESULT* result = iothub_device_auth_generate_credentials(xda_handle, &g_test_sas_cred);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        iothub_device_auth_destroy(xda_handle);
    }

    TEST_FUNCTION(iothub_device_auth_generate_credentials_fail)
    {
        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_iothub_device_auth_generate_credentials_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 5, 7, 9, 10, 11, 12, 13 };

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

            char tmp_msg[128];
            sprintf(tmp_msg, "iothub_device_auth_generate_credentials failure in test %zu/%zu", index, count);

            void* result = iothub_device_auth_generate_credentials(xda_handle, &g_test_sas_cred);

            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        iothub_device_auth_destroy(xda_handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(iothub_device_auth_generate_credentials_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(iothub_security_type()).SetReturn(IOTHUB_SECURITY_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);

        IOTHUB_SECURITY_HANDLE xda_handle = iothub_device_auth_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_iothub_device_auth_generate_credentials_x509_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[128];
            sprintf(tmp_msg, "iothub_device_auth_generate_credentials riot failure in test %zu/%zu", index, count);

            void* result = iothub_device_auth_generate_credentials(xda_handle, &g_test_sas_cred);

            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        iothub_device_auth_destroy(xda_handle);
        umock_c_negative_tests_deinit();
    }

    END_TEST_SUITE(iothub_auth_client_ut)
