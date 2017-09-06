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
#include "azure_hub_modules/device_auth.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/base64.h"

#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/bio.h"

#include "parson.h"

#include "azure_c_shared_utility/umock_c_prod.h"

MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
MOCKABLE_FUNCTION(, JSON_Status, json_serialize_to_file, const JSON_Value*, value, const char *, filename);
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_file, const char*, string);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char *, name);
MOCKABLE_FUNCTION(, const char*, json_value_get_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);

#if OPENSSL_VERSION_NUMBER <= 0x10001110L
MOCKABLE_FUNCTION(, BIO*, BIO_new_mem_buf, void*, buf, int, len);
#else
MOCKABLE_FUNCTION(, BIO*, BIO_new_mem_buf, const void*, buf, int, len);
#endif
MOCKABLE_FUNCTION(, BIO_METHOD*, BIO_s_mem);
MOCKABLE_FUNCTION(, RSA*, PEM_read_bio_RSA_PUBKEY, BIO*, bp, RSA**, x, pem_password_cb*, pwd_cb, void*, ctx);
MOCKABLE_FUNCTION(, RSA*, PEM_read_bio_RSAPrivateKey, BIO*, bp, RSA**, x, pem_password_cb*, pwd_cb, void*, ctx);
MOCKABLE_FUNCTION(, BIGNUM*, BN_new);
MOCKABLE_FUNCTION(, int, BN_set_word, BIGNUM*, a, BN_ULONG, w);
MOCKABLE_FUNCTION(, RSA*, RSA_new);
MOCKABLE_FUNCTION(, int, RSA_generate_key_ex, RSA*, rsa, int, bits, BIGNUM*, e, BN_GENCB*, cb);
MOCKABLE_FUNCTION(, BIO*, BIO_new, BIO_METHOD*, type);
MOCKABLE_FUNCTION(, int, PEM_write_bio_RSAPublicKey, BIO*, bp, const RSA*, x);
MOCKABLE_FUNCTION(, int, PEM_write_bio_RSAPrivateKey, BIO*, bp, RSA*, x, const EVP_CIPHER*, enc, unsigned char*, kstr, int, klen, pem_password_cb*, cb, void*, u);
MOCKABLE_FUNCTION(, void, BIO_free_all, BIO*, bio);
MOCKABLE_FUNCTION(, void, RSA_free, RSA*, rsa_value);
MOCKABLE_FUNCTION(, void, BN_free, BIGNUM*, bne);
MOCKABLE_FUNCTION(, int, BIO_read, BIO*, b, void*, buf, int, len);
MOCKABLE_FUNCTION(, long, BIO_ctrl, BIO*, bp, int, cmd, long, larg, void*, parg);
MOCKABLE_FUNCTION(, int, RSA_private_decrypt, int, flen, const unsigned char*, from, unsigned char*, to, RSA*, rsa, int, padding);
MOCKABLE_FUNCTION(, int, PEM_write_bio_RSA_PUBKEY, BIO*, bp, RSA*, x);

#undef ENABLE_MOCKS

#include "azure_hub_modules/device_auth_emulator.h"

#define TEST_PARAMETER_VALUE (void*)0x11111112
#define TEST_STRING_HANDLE_VALUE (STRING_HANDLE)0x11111113
#define TEST_TIME_T ((time_t)-1)
static const BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x11111116;
static BIO* TEST_BIO_HANDLE = (BIO*)0x11111117;
static RSA* TEST_RSA_HANDLE = (RSA*)0x11111118;
static BIGNUM* TEST_BIGNUM_HANDLE = (BIGNUM*)0x11111119;
static BIO_METHOD* TEST_BIO_METHOD_HANDLE = (BIO_METHOD*)0x1111111A;

static JSON_Value* TEST_JSON_VALUE = (JSON_Value*)0x11111117;
static JSON_Object* TEST_JSON_OBJECT = (JSON_Object*)0x11111118;

static unsigned char TEST_DATA[] = { 'k', 'e', 'y' };
static const size_t TEST_DATA_LEN = 3;
static char* TEST_STRING_VALUE = "Test_String_Value";

static const char* TEST_PARAM_INFO_VALUE = "Test Param Info";
static size_t TEST_STRING_LENGTH = sizeof("Test_String_Value");
static const char* TEST_NODE_STRING = "{ node: \"data\" }";

TEST_DEFINE_ENUM_TYPE(DEVICE_AUTH_TYPE, DEVICE_AUTH_TYPE_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(DEVICE_AUTH_TYPE, DEVICE_AUTH_TYPE_VALUES);

static SECURITY_INFO test_security_info;

static const char* RSA_READ_VALUE = "DATA_TO_BE_RETURNED";
static int RSA_READ_VALUE_LEN = sizeof("DATA_TO_BE_RETURNED");

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)my_gballoc_malloc(1);
    }
#ifdef __cplusplus
}
#endif

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static BUFFER_HANDLE my_Base64_Decoder(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    (void)handle;
    my_gballoc_free(handle);
}

static STRING_HANDLE my_STRING_new(void)
{
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

static STRING_HANDLE my_SASToken_Create(STRING_HANDLE key, STRING_HANDLE scope, STRING_HANDLE keyName, size_t expiry)
{
    (void)key, (void)scope, (void)keyName, (void)expiry;
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len+1);
    strcpy(*destination, source);
    return 0;
}

static int my_BIO_read(BIO* b, void* buf, int len)
{
    (void)b;
    memcpy(buf, RSA_READ_VALUE, len);
    return RSA_READ_VALUE_LEN;
}

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

BEGIN_TEST_SUITE(device_auth_emulator_ut)

    TEST_SUITE_INITIALIZE(suite_init)
    {
        int result;

        TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);

        result = umocktypes_charptr_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_UMOCK_ALIAS_TYPE(XDA_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(CONCRETE_XDA_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(DEVICE_AUTH_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BIO, void*);
        REGISTER_UMOCK_ALIAS_TYPE(RSA, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BIGNUM, void*);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_LENGTH);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_HOOK(SASToken_Create, my_SASToken_Create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_Create, NULL);

        REGISTER_GLOBAL_MOCK_RETURN(get_time, TEST_TIME_T);

        REGISTER_GLOBAL_MOCK_RETURN(json_parse_file, TEST_JSON_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_file, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_value, TEST_JSON_VALUE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, TEST_NODE_STRING);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_value_get_string, TEST_NODE_STRING);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_string, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(json_object_get_object, TEST_JSON_OBJECT);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);

        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_unbuild, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_unbuild, __LINE__);
        REGISTER_GLOBAL_MOCK_RETURN(BUFFER_build, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __LINE__);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

        REGISTER_GLOBAL_MOCK_HOOK(Base64_Decoder, my_Base64_Decoder);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Decoder, NULL);

        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BIO_new_mem_buf, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BIO_new_mem_buf, TEST_BIO_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BIO_s_mem, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BIO_s_mem, TEST_BIO_METHOD_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(PEM_read_bio_RSA_PUBKEY, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(PEM_read_bio_RSA_PUBKEY, TEST_RSA_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(PEM_read_bio_RSAPrivateKey, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(PEM_read_bio_RSAPrivateKey, TEST_RSA_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BN_new, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BN_new, TEST_BIGNUM_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BN_set_word, 0);
        REGISTER_GLOBAL_MOCK_RETURN(BN_set_word, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RSA_new, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(RSA_new, TEST_RSA_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RSA_generate_key_ex, 0);
        REGISTER_GLOBAL_MOCK_RETURN(RSA_generate_key_ex, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BIO_new, NULL);
        REGISTER_GLOBAL_MOCK_RETURN(BIO_new, TEST_BIO_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(PEM_write_bio_RSAPublicKey, 0);
        REGISTER_GLOBAL_MOCK_RETURN(PEM_write_bio_RSAPublicKey, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(PEM_write_bio_RSAPrivateKey, 0);
        REGISTER_GLOBAL_MOCK_RETURN(PEM_write_bio_RSAPrivateKey, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(BIO_read, 0);
        REGISTER_GLOBAL_MOCK_RETURN(BIO_read, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RSA_private_decrypt, 0);
        REGISTER_GLOBAL_MOCK_RETURN(RSA_private_decrypt, 1);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(PEM_write_bio_RSA_PUBKEY, 0);
        REGISTER_GLOBAL_MOCK_RETURN(PEM_write_bio_RSA_PUBKEY, 1);

        REGISTER_GLOBAL_MOCK_RETURN(BIO_ctrl, RSA_READ_VALUE_LEN);
        REGISTER_GLOBAL_MOCK_HOOK(BIO_read, my_BIO_read);

        test_security_info.password_value = "device_id";
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

    static void setup_dev_auth_emulator_get_endorsement_key_mocks(void)
    {
        STRICT_EXPECTED_CALL(BIO_s_mem());
        STRICT_EXPECTED_CALL(BIO_new(IGNORED_PTR_ARG))
            .IgnoreArgument_type();
        STRICT_EXPECTED_CALL(PEM_write_bio_RSA_PUBKEY(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_bp()
            .IgnoreArgument_x();
        STRICT_EXPECTED_CALL(BIO_ctrl(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(BIO_read(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreAllArguments();
        STRICT_EXPECTED_CALL(BIO_free_all(IGNORED_PTR_ARG))
            .IgnoreArgument_bio();
    }

    static void setup_dev_auth_emulator_retrieve_data_mocks(void)
    {
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .SetReturn(TEST_DATA_LEN);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument_handle()
            .SetReturn(TEST_DATA);
    }

    static void setup_dev_auth_emulator_generate_credentials_mocks(const char* token_scope)
    {
        STRICT_EXPECTED_CALL(STRING_new());
        STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(token_scope));
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG))
            .IgnoreArgument_psz();
        STRICT_EXPECTED_CALL(SASToken_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_scope()
            .IgnoreArgument_keyName()
            .IgnoreArgument_expiry()
            .IgnoreArgument_key();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_destination()
            .IgnoreArgument_source();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
    }
    
    static void setup_persisted_keys_info_mocks(void)
    {

    }

    static void setup_retrieve_persisted_key_mocks(bool json_file_found)
    {
        if (json_file_found)
        {
            STRICT_EXPECTED_CALL(json_parse_file(IGNORED_PTR_ARG))
                .IgnoreArgument_string();
            STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG))
                .IgnoreArgument_value();
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_name()
                .IgnoreArgument_object();
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_name()
                .IgnoreArgument_object();
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_name()
                .IgnoreArgument_object();
            STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_name()
                .IgnoreArgument_object();
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG))
                .IgnoreArgument_value();
            STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_PTR_ARG))
                .IgnoreArgument_source();
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG))
                .IgnoreArgument_value();
            STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_PTR_ARG))
                .IgnoreArgument_source();
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG))
                .IgnoreArgument_value();
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
                .IgnoreArgument_destination()
                .IgnoreArgument_source();
            STRICT_EXPECTED_CALL(json_value_get_string(IGNORED_PTR_ARG))
                .IgnoreArgument_value();
            STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_PTR_ARG))
                .IgnoreArgument_source();

            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BIO_new_mem_buf(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
                .IgnoreArgument_buf()
                .IgnoreArgument_len();
            STRICT_EXPECTED_CALL(PEM_read_bio_RSA_PUBKEY(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
                .IgnoreAllArguments();

            STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BIO_new_mem_buf(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
                .IgnoreArgument_buf()
                .IgnoreArgument_len();
            STRICT_EXPECTED_CALL(PEM_read_bio_RSAPrivateKey(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
                .IgnoreAllArguments();

            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();
            STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
                .IgnoreArgument_handle();

        }
        else
        {
            STRICT_EXPECTED_CALL(json_parse_file(IGNORED_PTR_ARG))
                .IgnoreArgument_string()
                .SetReturn(NULL);
        }
    }

    static void setup_generate_key_mocks(void)
    {
        STRICT_EXPECTED_CALL(BN_new());
        STRICT_EXPECTED_CALL(BN_set_word(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_a()
            .IgnoreArgument_w();
        STRICT_EXPECTED_CALL(RSA_new());
        STRICT_EXPECTED_CALL(RSA_generate_key_ex(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_rsa()
            .IgnoreArgument_bits()
            .IgnoreArgument_e()
            .IgnoreArgument_cb();
        STRICT_EXPECTED_CALL(BIO_s_mem());
        STRICT_EXPECTED_CALL(BIO_new(IGNORED_PTR_ARG))
            .IgnoreArgument_type();
        STRICT_EXPECTED_CALL(BIO_s_mem());
        STRICT_EXPECTED_CALL(BIO_new(IGNORED_PTR_ARG))
            .IgnoreArgument_type();
        STRICT_EXPECTED_CALL(PEM_write_bio_RSA_PUBKEY(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_bp()
            .IgnoreArgument_x();
        STRICT_EXPECTED_CALL(PEM_write_bio_RSAPrivateKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_bp()
            .IgnoreArgument_x()
            .IgnoreArgument_enc()
            .IgnoreArgument_kstr()
            .IgnoreArgument_klen()
            .IgnoreArgument_cb()
            .IgnoreArgument_u();
        STRICT_EXPECTED_CALL(RSA_free(IGNORED_PTR_ARG))
            .IgnoreArgument_rsa_value();
        STRICT_EXPECTED_CALL(BN_free(IGNORED_PTR_ARG))
            .IgnoreArgument_bne();
    }

    static void setup_dev_auth_emulator_create_mocks(bool use_persist_file)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();

        if (use_persist_file)
        {
            setup_retrieve_persisted_key_mocks(true);
        }
        else
        {
            setup_retrieve_persisted_key_mocks(false);
            setup_generate_key_mocks();
            setup_persisted_keys_info_mocks();
        }

        STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG))
            .IgnoreArgument_value();
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_001: [ If xda_create_parameters is NULL dev_auth_emulator_create shall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_create_security_info_NULL_fail)
    {
        //arrange

        //act
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(NULL);

        //assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_003: [ On success dev_auth_emulator_create shall allocate a new instance of the device auth interface. ] */
    TEST_FUNCTION(dev_auth_emulator_create_security_info_w_persist_file_succees)
    {
        //arrange
        setup_dev_auth_emulator_create_mocks(true);

        //act
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);

        //assert
        ASSERT_IS_NOT_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_002: [ If any failure is encountered dev_auth_emulator_create shall return NULL ] */
    TEST_FUNCTION(dev_auth_emulator_create_security_info_w_persist_file_fail)
    {
        //arrange
        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_dev_auth_emulator_create_mocks(true);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = {1, 10, 11, 14, 15, 18, 19, 20 };

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
            sprintf(tmp_msg, "dev_auth_emulator_create failure in test %zu/%zu", index, count);

            CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);

            //assert
            // This needs to be there
            dev_auth_emulator_destroy(handle);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_005: [ if handle is NULL, dev_auth_emulator_destroy shall do nothing. ] */
    TEST_FUNCTION(dev_auth_emulator_destroy_handle_NULL_fail)
    {
        //arrange

        //act
        dev_auth_emulator_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_006: [ dev_auth_emulator_destroy shall free all resources associated with CONCRETE_XDA_HANDLE. ] */
    /* Tests_SRS_DEV_AUTH_EMULATOR_07_004: [ dev_auth_emulator_destroy shall free the CONCRETE_XDA_HANDLE instance. ] */
    TEST_FUNCTION(dev_auth_emulator_destroy_succees)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BIO_free_all(IGNORED_PTR_ARG))
            .IgnoreArgument_bio();
        STRICT_EXPECTED_CALL(BIO_free_all(IGNORED_PTR_ARG))
            .IgnoreArgument_bio();
        STRICT_EXPECTED_CALL(RSA_free(IGNORED_PTR_ARG))
            .IgnoreArgument_rsa_value();
        STRICT_EXPECTED_CALL(RSA_free(IGNORED_PTR_ARG))
            .IgnoreArgument_rsa_value();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument_ptr();

        //act
        dev_auth_emulator_destroy(handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_007: [ if handle is NULL, dev_auth_emulator_get_auth_type shall return AUTH_TYPE_UNKNOWN ] */
    TEST_FUNCTION(dev_auth_emulator_get_auth_type_handle_NULL_fail)
    {
        //arrange

        //act
        DEVICE_AUTH_TYPE dev_auth_type = dev_auth_emulator_get_auth_type(NULL);

        //assert
        ASSERT_ARE_EQUAL(DEVICE_AUTH_TYPE, AUTH_TYPE_UNKNOWN, dev_auth_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_008: [ dev_auth_emulator_get_auth_type shall return AUTH_TYPE_SAS ] */
    TEST_FUNCTION(dev_auth_emulator_get_auth_type_succees)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        //act
        DEVICE_AUTH_TYPE dev_auth_type = dev_auth_emulator_get_auth_type(handle);

        //assert
        ASSERT_ARE_EQUAL(DEVICE_AUTH_TYPE, AUTH_TYPE_SAS, dev_auth_type);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_009: [ if handle or dev_auth_cred is NULL, dev_auth_emulator_generate_credentials shall return NULL. ]*/
    TEST_FUNCTION(dev_auth_emulator_generate_credentials_handle_NULL_fail)
    {
        //arrange
        umock_c_reset_all_calls();
        DEVICE_AUTH_CREDENTIAL_INFO dev_cred_info;
        DEVICE_AUTH_SAS_INFO dev_auth_sas;
        dev_auth_sas.expiry_seconds = 3600;
        dev_auth_sas.token_scope = "token_scope";

        dev_cred_info.cred_type = AUTH_TYPE_SAS;
        dev_cred_info.auth_cred_info.sas_info = dev_auth_sas;

        //act
        void* result = dev_auth_emulator_generate_credentials(NULL, &dev_cred_info);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_009: [ if handle or dev_auth_cred is NULL, dev_auth_emulator_generate_credentials shall return NULL. ]*/
    TEST_FUNCTION(dev_auth_emulator_generate_credentials_dev_auth_cred_NULL_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        //act
        void* result = dev_auth_emulator_generate_credentials(handle, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_012: [ Upon success dev_auth_emulator_generate_credentials shall return the created SAS Token. ] */
    TEST_FUNCTION(dev_auth_emulator_generate_credentials_succees)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();
        DEVICE_AUTH_CREDENTIAL_INFO dev_cred_info;
        DEVICE_AUTH_SAS_INFO dev_auth_sas;
        dev_auth_sas.expiry_seconds = 3600;
        dev_auth_sas.token_scope = "token_scope";

        dev_cred_info.cred_type = AUTH_TYPE_SAS;
        dev_cred_info.auth_cred_info.sas_info = dev_auth_sas;

        setup_dev_auth_emulator_generate_credentials_mocks(dev_auth_sas.token_scope);

        //act
        void* result = dev_auth_emulator_generate_credentials(handle, &dev_cred_info);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(result);
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_011: [ If a failure is encountered, dev_auth_emulator_generate_credentials shall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_generate_credentials_fails)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        DEVICE_AUTH_CREDENTIAL_INFO dev_cred_info;
        DEVICE_AUTH_SAS_INFO dev_auth_sas;
        dev_auth_sas.expiry_seconds = 3600;
        dev_auth_sas.token_scope = "token_scope";

        dev_cred_info.cred_type = AUTH_TYPE_SAS;
        dev_cred_info.auth_cred_info.sas_info = dev_auth_sas;
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_dev_auth_emulator_generate_credentials_mocks(dev_auth_sas.token_scope);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 1, 5, 7, 8, 9, 10 };

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
            sprintf(tmp_msg, "dev_auth_emulator_create failure in test %zu/%zu", index, count);

            void* result = dev_auth_emulator_generate_credentials(handle, &dev_cred_info);

            //assert
            ASSERT_IS_NULL(result);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_013: [ If handle is NULL dev_auth_emulator_get_endorsement_key shall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_get_endorsement_key_handle_NULL_fail)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        char* result = dev_auth_emulator_get_endorsement_key(NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_014: [ dev_auth_emulator_get_endorsement_key shall allocate and return the device key as the endorsement key. ] */
    TEST_FUNCTION(dev_auth_emulator_get_endorsement_key_success)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        setup_dev_auth_emulator_get_endorsement_key_mocks();

        //act
        char* result = dev_auth_emulator_get_endorsement_key(handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
        my_gballoc_free(result);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_015: [ If a failure is encountered, dev_auth_emulator_get_endorsement_key shall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_get_endorsement_key_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        setup_dev_auth_emulator_get_endorsement_key_mocks();

        //act
        char* result = dev_auth_emulator_get_endorsement_key(handle);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
        my_gballoc_free(result);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_016: [ If handle or data is NULL or data_len is 0, dev_auth_emulator_store_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_handle_NULL_fail)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        int result = dev_auth_emulator_store_data(NULL, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_016: [ If handle or data is NULL or data_len is 0, dev_auth_emulator_store_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_data_NULL_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        //act
        int result = dev_auth_emulator_store_data(handle, NULL, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_016: [ If handle or data is NULL or data_len is 0, dev_auth_emulator_store_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_length_zero_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        //act
        int result = dev_auth_emulator_store_data(handle, TEST_DATA, 0);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_025: [ On success dev_auth_emulator_store_data shall return zero. ] */
    /* Tests_SRS_DEV_AUTH_EMULATOR_07_017: [ dev_auth_emulator_store_data shall store the the data in a BUFFER variable. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_success)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(BUFFER_unbuild(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, TEST_DATA, TEST_DATA_LEN))
            .IgnoreArgument_handle();

        //act
        int result = dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_018: [ If the a data object has been previously stored in the BUFFER variable, dev_auth_emulator_store_data shall overwrite the existing value with the new data object. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_2nd_call_success)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        int result = dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);
        umock_c_reset_all_calls();

        EXPECTED_CALL(BUFFER_unbuild(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_handle()
            .IgnoreArgument_source()
            .IgnoreArgument_size();

        //act
        result = dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_019: [ If any failure is encountered, dev_auth_emulator_store_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_store_data_2nd_call_fails)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        EXPECTED_CALL(BUFFER_unbuild(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument_handle()
            .IgnoreArgument_source()
            .IgnoreArgument_size();

        umock_c_negative_tests_snapshot();

        //act
        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            int result = dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);

            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "dev_auth_emulator_store_data failure in test %zu/%zu", index, count);

            result = dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_020: [ If handle or data_len is NULL, dev_auth_emulator_retrieve_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_retrieve_data_handle_NULL_fail)
    {
        //arrange
        size_t data_len;

        //act
        unsigned char* result = dev_auth_emulator_retrieve_data(NULL, &data_len);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_020: [ If handle or data_len is NULL, dev_auth_emulator_retrieve_data shall return a non-zero value. ] */
    TEST_FUNCTION(dev_auth_emulator_retrieve_data_data_len_NULL_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);
        umock_c_reset_all_calls();

        //act
        unsigned char* result = dev_auth_emulator_retrieve_data(handle, NULL);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_021: [ dev_auth_emulator_retrieve_data shall return the data previously stored by dev_auth_emulator_store_data and add the length of the data in data_len. ] */
    TEST_FUNCTION(dev_auth_emulator_retrieve_data_success)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);
        umock_c_reset_all_calls();

        size_t data_len;
        setup_dev_auth_emulator_retrieve_data_mocks();

        //act
        unsigned char* result = dev_auth_emulator_retrieve_data(handle, &data_len);

        //assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(int, 0, memcmp(result, TEST_DATA, TEST_DATA_LEN) );
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
        my_gballoc_free(result);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_022: [ If dev_auth_emulator_store_data has not been called dev_auth_emulator_retrieve_data shall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_retrieve_data_no_data_success)
    {
        //arrange
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument_size();
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument_handle();
        STRICT_EXPECTED_CALL(json_parse_file(IGNORED_PTR_ARG))
            .IgnoreArgument_string()
            .SetReturn(NULL);
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        umock_c_reset_all_calls();

        size_t data_len;

        //act
        unsigned char* result = dev_auth_emulator_retrieve_data(handle, &data_len);

        //assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_023: [ If an error is encountered dev_auth_emulator_store_datashall return NULL. ] */
    TEST_FUNCTION(dev_auth_emulator_retrieve_data_fail)
    {
        //arrange
        CONCRETE_XDA_HANDLE handle = dev_auth_emulator_create(&test_security_info);
        dev_auth_emulator_store_data(handle, TEST_DATA, TEST_DATA_LEN);
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        size_t data_len;
        setup_dev_auth_emulator_retrieve_data_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0, 2 };

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
            sprintf(tmp_msg, "dev_auth_emulator_retrieve_data failure in test %zu/%zu", index, count);

            unsigned char* result = dev_auth_emulator_retrieve_data(handle, &data_len);

            // assert
            ASSERT_IS_NULL_WITH_MSG(result, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
        dev_auth_emulator_destroy(handle);
    }

    /* Tests_SRS_DEV_AUTH_EMULATOR_07_026: [ dev_auth_emulator_interface_desc shall return the XDA_INTERFACE_DESCRIPTION structure. ] */
    TEST_FUNCTION(dev_auth_emulator_interface_desc_success)
    {
        //arrange

        //act
        const XDA_INTERFACE_DESCRIPTION* interface_desc = dev_auth_emulator_interface_desc();

        //assert
        ASSERT_IS_TRUE(interface_desc->concrete_device_auth_create == dev_auth_emulator_create);
        ASSERT_IS_TRUE(interface_desc->concrete_device_auth_destroy == dev_auth_emulator_destroy);
        ASSERT_IS_TRUE(interface_desc->concrete_device_auth_get_type == dev_auth_emulator_get_auth_type);
        ASSERT_IS_TRUE(interface_desc->concrete_device_auth_generate_cred == dev_auth_emulator_generate_credentials);
        ASSERT_IS_TRUE(interface_desc->concrete_get_endorsement_key == dev_auth_emulator_get_endorsement_key);
        ASSERT_IS_TRUE(interface_desc->concrete_store_data == dev_auth_emulator_store_data);
        ASSERT_IS_TRUE(interface_desc->concrete_retrieve_data == dev_auth_emulator_retrieve_data);
        ASSERT_IS_TRUE(interface_desc->concrete_decrypt_value == dev_auth_emulator_decrypt_value);

        //cleanup
    }

    END_TEST_SUITE(device_auth_emulator_ut)
