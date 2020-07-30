// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include <time.h>

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
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/azure_base32.h"
#include "azure_c_shared_utility/hmacsha256.h"
#include "hsm_client_data.h"
#undef ENABLE_MOCKS

#include "azure_prov_client/internal/prov_auth_client.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/sastoken.h"

MOCKABLE_FUNCTION(, HSM_CLIENT_HANDLE, secure_device_create);
MOCKABLE_FUNCTION(, void, secure_device_destroy, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, secure_device_import_key, HSM_CLIENT_HANDLE, handle, const unsigned char*, key, size_t, key_len);

MOCKABLE_FUNCTION(, int, secure_device_get_endorsement_key, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, secure_device_get_storage_key, HSM_CLIENT_HANDLE, handle, unsigned char**, key, size_t*, key_len);
MOCKABLE_FUNCTION(, int, secure_device_sign_data, HSM_CLIENT_HANDLE, handle, const unsigned char*, data, size_t, data_len, unsigned char**, key, size_t*, key_len);

MOCKABLE_FUNCTION(, char*, secure_device_get_certificate, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_device_get_alias_key, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_device_get_common_name, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, secure_device_get_symm_key, HSM_CLIENT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, secure_device_set_symmetrical_key_info, HSM_CLIENT_HANDLE, handle, const char*, reg_name, const char*, symm_key);

MOCKABLE_FUNCTION(, int, SHA256Reset, SHA256Context*, ctx);
MOCKABLE_FUNCTION(, int, SHA256Input, SHA256Context*, ctx, const uint8_t*, bytes, unsigned int, bytecount);
MOCKABLE_FUNCTION(, int, SHA256Result, SHA256Context*, ctx, uint8_t*, Message_Digest);

MOCKABLE_FUNCTION(, const HSM_CLIENT_TPM_INTERFACE*, hsm_client_tpm_interface);
MOCKABLE_FUNCTION(, const HSM_CLIENT_X509_INTERFACE*, hsm_client_x509_interface);
MOCKABLE_FUNCTION(, const HSM_CLIENT_KEY_INTERFACE*, hsm_client_key_interface);

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

#define TEST_BUFFER_VALUE (BUFFER_HANDLE)0x11111111
#define TEST_JSON_ROOT_VALUE (JSON_Value*)0x11111112
#define TEST_JSON_OBJECT_VALUE (JSON_Object*)0x11111113
#define TEST_JSON_STATUS_VALUE (JSON_Value*)0x11111114
#define TEST_EXPIRY_TIME_T_VALUE          (time_t)0

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static char* TEST_STRING_VALUE = "Test_String_Value";
static char* TEST_DPS_ASSIGNED_VALUE = "assigned";
static char* TEST_DPS_UNASSIGNED_VALUE = "unassigned";

static const char* TEST_PARAM_INFO_VALUE = "Test Param Info";
static const char* TEST_TOKEN_SCOPE_VALUE = "token_scope";
static const char* TEST_KEY_NAME_VALUE = "key_value";
static const char* TEST_BASE32_VALUE = "aebagbaf";

static const char* TEST_REGISTRATION_ID = "Registration Id";

TEST_DEFINE_ENUM_TYPE(PROV_AUTH_RESULT, PROV_AUTH_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_AUTH_RESULT, PROV_AUTH_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(PROV_AUTH_TYPE, PROV_AUTH_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PROV_AUTH_TYPE, PROV_AUTH_TYPE_VALUES);

TEST_DEFINE_ENUM_TYPE(HMACSHA256_RESULT, HMACSHA256_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HMACSHA256_RESULT, HMACSHA256_RESULT_VALUES);

static const HSM_CLIENT_TPM_INTERFACE test_tpm_interface =
{
    secure_device_create,
    secure_device_destroy,
    secure_device_import_key,
    secure_device_get_endorsement_key,
    secure_device_get_storage_key,
    secure_device_sign_data,
};

static const HSM_CLIENT_TPM_INTERFACE test_tpm_interface_fail =
{
    secure_device_create,
    secure_device_destroy,
    NULL,
    secure_device_get_endorsement_key,
    secure_device_get_storage_key,
    NULL
};

static const HSM_CLIENT_X509_INTERFACE test_x509_interface =
{
    secure_device_create,
    secure_device_destroy,
    secure_device_get_certificate,
    secure_device_get_alias_key,
    secure_device_get_common_name
};

static const HSM_CLIENT_X509_INTERFACE test_x509_interface_fail =
{
    secure_device_create,
    secure_device_destroy,
    NULL,
    secure_device_get_alias_key,
    secure_device_get_common_name
};

static const HSM_CLIENT_KEY_INTERFACE test_key_interface =
{
    secure_device_create,
    secure_device_destroy,
    secure_device_get_symm_key,
    secure_device_get_common_name,
    secure_device_set_symmetrical_key_info
};

static HSM_CLIENT_HANDLE my_secure_device_create(void)
{
    return (HSM_CLIENT_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free((void*)handle);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_secure_device_destroy(HSM_CLIENT_HANDLE handle)
{
    my_gballoc_free(handle);
}

static char* my_secure_device_get_alias_key(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static int my_secure_device_get_storage_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    (void)handle;
    *key = (unsigned char*)my_gballoc_malloc(1);
    *key_len = 1;
    return 0;
}

static int my_secure_device_get_endorsement_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    (void)handle;
    *key = (unsigned char*)my_gballoc_malloc(1);
    *key_len = 1;
    return 0;
}

static char* my_secure_device_get_common_name(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len+1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static char* my_secure_device_get_symm_key(HSM_CLIENT_HANDLE handle)
{
    (void)handle;
    size_t len = strlen(TEST_STRING_VALUE);
    char* result = (char*)my_gballoc_malloc(len + 1);
    strcpy(result, TEST_STRING_VALUE);
    return result;
}

static int my_secure_device_sign_data(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** key, size_t* key_len)
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

static STRING_HANDLE my_Base64_Encode(BUFFER_HANDLE input)
{
    (void)input;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_Azure_Base64_Encode_Bytes(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static char* my_Base32_Encode_Bytes(const unsigned char* source, size_t size)
{
    char* result;
    (void)source;
    (void)size;
    size_t src_len = strlen(TEST_BASE32_VALUE);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_BASE32_VALUE);
    return result;
}

static BUFFER_HANDLE my_Azure_Base64_Decode(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_Encode(STRING_HANDLE input)
{
    (void)input;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
    (void)textEncode;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(prov_auth_client_ut)

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

        REGISTER_TYPE(HMACSHA256_RESULT, HMACSHA256_RESULT);

        REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURE_DEVICE_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

        REGISTER_GLOBAL_MOCK_HOOK(secure_device_create, my_secure_device_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(secure_device_destroy, my_secure_device_destroy);
        REGISTER_GLOBAL_MOCK_HOOK(secure_device_get_endorsement_key, my_secure_device_get_endorsement_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_endorsement_key, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(secure_device_get_common_name, my_secure_device_get_common_name);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_common_name, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(secure_device_get_storage_key, my_secure_device_get_storage_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_storage_key, __LINE__);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_import_key, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(secure_device_get_alias_key, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_alias_key, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(secure_device_get_certificate, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_certificate, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(secure_device_get_symm_key, my_secure_device_get_symm_key);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_get_symm_key, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(secure_device_set_symmetrical_key_info, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_set_symmetrical_key_info, __LINE__);

        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, TEST_DATA);
        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, TEST_DATA_LEN);

        REGISTER_GLOBAL_MOCK_RETURN(SHA256Reset, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SHA256Reset, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(SHA256Input, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SHA256Input, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(SHA256Result, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SHA256Result, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(secure_device_sign_data, my_secure_device_sign_data);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(secure_device_sign_data, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode, my_Base64_Encode);
        REGISTER_GLOBAL_MOCK_RETURN(Azure_Base64_Encode, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_create, TEST_BUFFER_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, TEST_DATA_LEN);
        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, TEST_DATA);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_RETURN(size_tToString, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(size_tToString, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Encode_Bytes, my_Azure_Base64_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base32_Encode_Bytes, my_Base32_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base32_Encode_Bytes, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(Azure_Base64_Decode, my_Azure_Base64_Decode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Decode, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_length, strlen(TEST_STRING_VALUE));
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
        REGISTER_GLOBAL_MOCK_HOOK(URL_Encode, my_URL_Encode);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_Encode, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(HMACSHA256_ComputeHash, HMACSHA256_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(HMACSHA256_ComputeHash, HMACSHA256_ERROR);

        REGISTER_GLOBAL_MOCK_RETURN(prov_dev_security_get_type, SECURE_DEVICE_TYPE_TPM);
        REGISTER_GLOBAL_MOCK_RETURN(hsm_client_tpm_interface, &test_tpm_interface);
        REGISTER_GLOBAL_MOCK_RETURN(hsm_client_x509_interface, &test_x509_interface);
        REGISTER_GLOBAL_MOCK_RETURN(hsm_client_key_interface, &test_key_interface);
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

    static void setup_sign_sas_data(bool use_key)
    {
        if (use_key)
        {
            STRICT_EXPECTED_CALL(secure_device_get_symm_key(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(Azure_Base64_Decode(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(BUFFER_new());
            STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
            STRICT_EXPECTED_CALL(HMACSHA256_ComputeHash(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).CallCannotFail();
            STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).CallCannotFail();
            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(secure_device_sign_data(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        }
    }

    static void setup_prov_auth_construct_sas_token_mocks(bool use_key)
    {
        STRICT_EXPECTED_CALL(size_tToString(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        setup_sign_sas_data(use_key);
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(URL_Encode(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    static void setup_load_registration_id_mocks(bool use_tpm)
    {
        if (use_tpm)
        {
            STRICT_EXPECTED_CALL(secure_device_get_endorsement_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(SHA256Reset(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(SHA256Input(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(SHA256Result(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(Azure_Base32_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(secure_device_get_common_name(IGNORED_PTR_ARG));
        }
    }

    static void setup_prov_auth_get_registration_id_mocks()
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_002: [ If any failure is encountered prov_auth_create shall return NULL ] */
    TEST_FUNCTION(prov_auth_client_create_tpm_malloc_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_002: [ If any failure is encountered prov_auth_create shall return NULL ] */
    TEST_FUNCTION(prov_auth_client_create_tpm_interface_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_TPM);
        STRICT_EXPECTED_CALL(hsm_client_tpm_interface()).SetReturn(&test_tpm_interface_fail);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_001: [ prov_auth_create shall allocate the DPS_SEC_INFO. ] */
    /* Tests_SRS_DPS_SEC_CLIENT_07_003: [ prov_auth_create shall validate the specified secure enclave interface to ensure. ] */
    /* Tests_SRS_DPS_SEC_CLIENT_07_004: [ prov_auth_create shall call secure_device_create on the secure enclave interface. ] */
    TEST_FUNCTION(prov_auth_client_create_tpm_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_TPM);
        STRICT_EXPECTED_CALL(hsm_client_tpm_interface()).SetReturn(&test_tpm_interface);
        STRICT_EXPECTED_CALL(secure_device_create());

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_002: [ If any failure is encountered prov_auth_create shall return NULL ] */
    TEST_FUNCTION(prov_auth_client_create_x509_interface_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_001: [ prov_auth_create shall allocate the DPS_SEC_INFO. ] */
    /* Tests_SRS_DPS_SEC_CLIENT_07_003: [ prov_auth_create shall validate the specified secure enclave interface to ensure. ] */
    /* Tests_SRS_DPS_SEC_CLIENT_07_004: [ prov_auth_create shall call secure_device_create on the secure enclave interface. ] */
    TEST_FUNCTION(prov_auth_client_create_x509_succeed)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        STRICT_EXPECTED_CALL(secure_device_create());

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_002: [ If any failure is encountered prov_auth_create shall return NULL ] */
    TEST_FUNCTION(prov_auth_client_create_x509_sec_device_create_NULL_fail)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();

        //assert
        ASSERT_IS_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_006: [ prov_auth_destroy shall free the PROV_AUTH_HANDLE instance. ] */
    /* Tests_SRS_DPS_SEC_CLIENT_07_007: [ prov_auth_destroy shall free all resources allocated in this module. ] */
    TEST_FUNCTION(prov_auth_destroy_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(secure_device_destroy(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        prov_auth_destroy(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_005: [ if handle is NULL, prov_auth_destroy shall do nothing. ] */
    TEST_FUNCTION(prov_auth_destroy_handle_NULL_succeed)
    {
        umock_c_reset_all_calls();

        //arrange

        //act
        prov_auth_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_008: [ if handle is NULL prov_auth_get_type shall return PROV_AUTH_TYPE_UNKNOWN ] */
    TEST_FUNCTION(prov_auth_get_type_handle_NULL_fail)
    {
        //arrange

        //act
        PROV_AUTH_TYPE sec_type = prov_auth_get_type(NULL);

        //assert
        ASSERT_ARE_EQUAL(PROV_AUTH_TYPE, PROV_AUTH_TYPE_UNKNOWN, sec_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_009: [ prov_auth_get_type shall return the PROV_AUTH_TYPE of the underlying secure enclave. ] */
    TEST_FUNCTION(prov_auth_get_type_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        PROV_AUTH_TYPE sec_type = prov_auth_get_type(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(PROV_AUTH_TYPE, PROV_AUTH_TYPE_TPM, sec_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_009: [ prov_auth_get_type shall return the PROV_AUTH_TYPE of the underlying secure enclave. ] */
    TEST_FUNCTION(prov_auth_get_type_x509_succeed)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        PROV_AUTH_TYPE sec_type = prov_auth_get_type(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(PROV_AUTH_TYPE, PROV_AUTH_TYPE_X509, sec_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_010: [ if handle is NULL prov_auth_get_registration_id shall return NULL. ] */
    TEST_FUNCTION(prov_auth_get_registration_id_handle_NULL_fail)
    {
        //arrange

        //act
        char* result = prov_auth_get_registration_id(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_012: [ If a failure is encountered, prov_auth_get_registration_id shall return NULL. ] */
    TEST_FUNCTION(prov_auth_get_registration_id_tpm_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_load_registration_id_mocks(true);
        setup_prov_auth_get_registration_id_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                char* result = prov_auth_get_registration_id(sec_handle);

                //assert
                ASSERT_IS_NULL(result, "prov_auth_get_registration_id failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        prov_auth_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_013: [ Upon success prov_auth_get_registration_id shall return the registration id of the secure enclave. ] */
    TEST_FUNCTION(prov_auth_get_registration_id_tpm_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        setup_load_registration_id_mocks(true);
        setup_prov_auth_get_registration_id_mocks();

        //act
        char* result = prov_auth_get_registration_id(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        prov_auth_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_012: [ If a failure is encountered, prov_auth_get_registration_id shall return NULL. ] */
    TEST_FUNCTION(prov_auth_get_registration_id_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_load_registration_id_mocks(false);
        setup_prov_auth_get_registration_id_mocks();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                char* result = prov_auth_get_registration_id(sec_handle);

                //assert
                ASSERT_IS_NULL(result, "prov_auth_get_registration_id failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_DPS_SEC_CLIENT_07_013: [ Upon success prov_auth_get_registration_id shall return the registration id of the secure enclave. ] */
    TEST_FUNCTION(prov_auth_get_registration_id_x509_succeed)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        setup_load_registration_id_mocks(false);
        setup_prov_auth_get_registration_id_mocks();

        //act
        char* result = prov_auth_get_registration_id(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_endorsement_key_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        BUFFER_HANDLE result = prov_auth_get_endorsement_key(sec_handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_endorsement_key_tpm_handle_NULL_fail)
    {
        //arrange

        //act
        BUFFER_HANDLE result = prov_auth_get_endorsement_key(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_get_endorsement_key_tpm_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_get_endorsement_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        BUFFER_HANDLE result = prov_auth_get_endorsement_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_storage_key_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        BUFFER_HANDLE result = prov_auth_get_storage_key(sec_handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_storage_key_tpm_handle_NULL_fail)
    {
        //arrange

        //act
        BUFFER_HANDLE result = prov_auth_get_storage_key(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_get_storage_key_tpm_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_get_storage_key(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        BUFFER_HANDLE result = prov_auth_get_storage_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_import_key_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_auth_import_key(sec_handle, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_import_key_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_auth_import_key(NULL, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_import_key_key_len_0_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_auth_import_key(sec_handle, TEST_DATA, 0);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_import_key_key_value_NULL_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_auth_import_key(sec_handle, NULL, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_import_key_tpm_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_import_key(IGNORED_PTR_ARG, TEST_DATA, TEST_DATA_LEN));

        //act
        int result = prov_auth_import_key(sec_handle, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_import_key_tpm_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_import_key(IGNORED_PTR_ARG, TEST_DATA, TEST_DATA_LEN)).SetReturn(__LINE__);

        //act
        int result = prov_auth_import_key(sec_handle, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_handle_NULL_fail)
    {

        //arrange

        //act
        char* result = prov_auth_construct_sas_token(NULL, TEST_TOKEN_SCOPE_VALUE, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_key_name_NULL_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        char* result = prov_auth_construct_sas_token(sec_handle, TEST_TOKEN_SCOPE_VALUE, NULL, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_token_scope_NULL_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        char* result = prov_auth_construct_sas_token(sec_handle, NULL, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_x509_fail)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface_fail);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        char* result = prov_auth_construct_sas_token(sec_handle, TEST_TOKEN_SCOPE_VALUE, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_symm_key_succeed)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_SYMMETRIC_KEY);
        STRICT_EXPECTED_CALL(hsm_client_key_interface());
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        setup_prov_auth_construct_sas_token_mocks(true);

        //act
        char* result = prov_auth_construct_sas_token(sec_handle, TEST_TOKEN_SCOPE_VALUE, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_succeed)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        setup_prov_auth_construct_sas_token_mocks(false);

        //act
        char* result = prov_auth_construct_sas_token(sec_handle, TEST_TOKEN_SCOPE_VALUE, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_construct_sas_token_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        setup_prov_auth_construct_sas_token_mocks(false);

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            if (umock_c_negative_tests_can_call_fail(index))
            {
                umock_c_negative_tests_reset();
                umock_c_negative_tests_fail_call(index);

                char* result = prov_auth_construct_sas_token(sec_handle, TEST_TOKEN_SCOPE_VALUE, TEST_KEY_NAME_VALUE, TEST_EXPIRY_TIME_T_VALUE);

                //assert
                ASSERT_IS_NULL(result, "prov_auth_construct_sas_token failure in test %zu/%zu", index, count);
            }
        }

        //cleanup
        prov_auth_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    TEST_FUNCTION(prov_auth_get_certificate_handle_NULL_fail)
    {
        //arrange

        //act
        char* result = prov_auth_get_certificate(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_get_certificate_tpm_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        char* result = prov_auth_get_certificate(sec_handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_set_registration_id_handle_NULL_fail)
    {
        //arrange

        //act
        int result = prov_auth_set_registration_id(NULL, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_set_registration_id_registration_id_NULL_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        int result = prov_auth_set_registration_id(sec_handle, NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_set_registration_id_success)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        int result = prov_auth_set_registration_id(sec_handle, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_set_registration_id_2_calls_success)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        int result = prov_auth_set_registration_id(sec_handle, TEST_REGISTRATION_ID);
        ASSERT_ARE_EQUAL(int, 0, result);
        result = prov_auth_set_registration_id(sec_handle, TEST_REGISTRATION_ID);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_certificate_succeed)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_get_certificate(IGNORED_PTR_ARG));

        //act
        char* result = prov_auth_get_certificate(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_alias_key_handle_NULL_fail)
    {
        //arrange

        //act
        char* result = prov_auth_get_alias_key(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(prov_auth_get_alias_key_tpm_fail)
    {
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange

        //act
        char* result = prov_auth_get_alias_key(sec_handle);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    TEST_FUNCTION(prov_auth_get_alias_key_succeed)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(prov_dev_security_get_type()).SetReturn(SECURE_DEVICE_TYPE_X509);
        STRICT_EXPECTED_CALL(hsm_client_x509_interface()).SetReturn(&test_x509_interface);
        PROV_AUTH_HANDLE sec_handle = prov_auth_create();
        umock_c_reset_all_calls();

        //arrange
        STRICT_EXPECTED_CALL(secure_device_get_alias_key(IGNORED_PTR_ARG));

        //act
        char* result = prov_auth_get_alias_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        prov_auth_destroy(sec_handle);
    }

    END_TEST_SUITE(prov_auth_client_ut)
