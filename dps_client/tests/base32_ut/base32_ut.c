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

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#undef ENABLE_MOCKS

#include "azure_hub_modules/base32.h"

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x11;

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

typedef struct TEST_BASE32_VALUE_TAG
{
    size_t input_len;
    const unsigned char* input_data;
    const char* base32_data;
} TEST_BASE32_VALUE;

static const TEST_BASE32_VALUE test_val_len[] =
{
    { 1, (const unsigned char*)"\x01", "ae======" },
    { 1, (const unsigned char*)"\x02", "ai======" },
    { 1, (const unsigned char*)"\x03", "am======" },
    { 1, (const unsigned char*)"\x04", "aq======" },
    { 1, (const unsigned char*)"\x05", "au======" },
    { 1, (const unsigned char*)"\x06", "ay======" },
    { 1, (const unsigned char*)"\x07", "a4======" },
    { 1, (const unsigned char*)"\x08", "ba======" },
    { 1, (const unsigned char*)"\x09", "be======" },
    { 1, (const unsigned char*)"\x0A", "bi======" },
    { 1, (const unsigned char*)"\x84", "qq======" },
    { 2, (const unsigned char*)"\x0b\x09", "bmeq====" },
    { 2, (const unsigned char*)"\x22\x99", "ekmq====" },
    { 2, (const unsigned char*)"\xFF\xFF", "777q====" },
    { 3, (const unsigned char*)"\x0A\x00\x0a", "biaau===" },
    { 3, (const unsigned char*)"\x99\xCC\xDD", "thgn2===" },
    { 4, (const unsigned char*)"\x01\x02\x03\x04", "aebagba=" },
    { 5, (const unsigned char*)"\x01\x02\x03\x04\x05", "aebagbaf" },
    { 10, (const unsigned char*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", "aaaaaaaaaaaaaaaa" },
    { 10, (const unsigned char*)"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", "7777777777777777" }
};

static STRING_HANDLE my_STRING_new(void)
{
    return TEST_STRING_HANDLE;
}

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static unsigned char* my_BUFFER_u_char(BUFFER_HANDLE handle)
{
    TEST_BASE32_VALUE* target = (TEST_BASE32_VALUE*)handle;
    return (unsigned char*)target->input_data;
}

static size_t my_BUFFER_length(BUFFER_HANDLE handle)
{
    TEST_BASE32_VALUE* target = (TEST_BASE32_VALUE*)handle;
    return target->input_len;
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    char* temp = (char*)my_gballoc_malloc(strlen(psz) + 1);
    ASSERT_IS_NOT_NULL(temp);
    (void)memcpy(temp, psz, strlen(psz) + 1);
    return (STRING_HANDLE)temp;
}

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

BEGIN_TEST_SUITE(base32_ut)

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

        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

        REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, my_BUFFER_length);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, my_BUFFER_u_char);

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

    /* Tests_SRS_BASE32_07_004: [ If source is NULL Base32_Encoder shall return NULL. ] */
    TEST_FUNCTION(Base32_Encode_Bytes_source_NULL)
    {
        char* result;

        //arrange

        //act
        result = Base32_Encode_Bytes(NULL, 10);

        //assert
        ASSERT_IS_NULL(result);

        //cleanup
    }

    /* Tests_SRS_BASE32_07_005: [ If size is 0 Base32_Encoder shall return an empty string. ] */
    TEST_FUNCTION(Base32_Encode_Bytes_len_is_0_success)
    {
        char* result;

        //arrange

        //act
        result = Base32_Encode_Bytes((const unsigned char*)"", 0);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, "", result);

        //cleanup
        free(result);
    }

    /* Tests_SRS_BASE32_07_006: [ If successful Base32_Encoder shall return the base32 value of input. ] */
    /* Tests_SRS_BASE32_07_007: [ Base32_Encode_Bytes shall call into base32_encoder_impl to encode the source data. ] */
    /* Tests_SRS_BASE32_07_009: [ base32_encode_impl shall allocate the buffer to the size of the encoding value. ] */
    /* Tests_SRS_BASE32_07_010: [ base32_encode_impl shall look through source and separate each block into 5 bit chunks ] */
    /* Tests_SRS_BASE32_07_011: [ base32_encode_impl shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. ] */
    /* Tests_SRS_BASE32_07_012: [ If the src_size is not divisible by 8, base32_encode_impl shall pad the remaining places with =. ] */
    TEST_FUNCTION(Base32_Encode_Bytes_success)
    {
        char* result;
        size_t num_elements = sizeof(test_val_len)/sizeof(test_val_len[0]);

        //arrange

        //act
        for (size_t index = 0; index < num_elements; index++)
        {
            char tmp_msg[64];
            sprintf(tmp_msg, "Base32_Encode_Bytes failure in test %zu", index);

            result = Base32_Encode_Bytes(test_val_len[index].input_data, test_val_len[index].input_len);

            //assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, test_val_len[index].base32_data, result, tmp_msg);

            //cleanup
            free(result);
        }
    }

    /* Tests_SRS_BASE32_07_001: [ If source is NULL Base32_Encoder shall return NULL. ] */
    TEST_FUNCTION(Base32_Encoder_source_failed)
    {
        STRING_HANDLE result;

        //arrange

        //act
        result = Base32_Encoder(NULL);

        //assert
        ASSERT_IS_NULL(result);
    }

    /* Tests_SRS_BASE32_07_015: [ If size is 0 Base32_Encoder shall return an empty string. ] */
    TEST_FUNCTION(Base32_Encoder_source_len_0_failed)
    {
        STRING_HANDLE result;

        //arrange
        TEST_BASE32_VALUE zero_length = { 0 };
        BUFFER_HANDLE input_buff = (BUFFER_HANDLE)&zero_length;

        //act
        result = Base32_Encoder(input_buff);

        //assert
        ASSERT_IS_TRUE(TEST_STRING_HANDLE == result);
    }

    /* Tests_SRS_BASE32_07_002: [ If successful Base32_Encoder shall return the base32 value of source. ] */
    /* Tests_SRS_BASE32_07_003: [ Base32_Encoder shall call into base32_encoder_impl to encode the source data. ] */
    /* Tests_SRS_BASE32_07_012: [ Base32_Encoder shall wrap the base32_encoder_impl result into a STRING_HANDLE. ] */
    /* Tests_SRS_BASE32_07_009: [ base32_encode_impl shall allocate the buffer to the size of the encoding value. ] */
    /* Tests_SRS_BASE32_07_010: [ base32_encode_impl shall look through source and separate each block into 5 bit chunks ] */
    /* Tests_SRS_BASE32_07_011: [ base32_encode_impl shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. ] */
    /* Tests_SRS_BASE32_07_012: [ If the src_size is not divisible by 8, base32_encode_impl shall pad the remaining places with =. ] */
    TEST_FUNCTION(Base32_Encoder_success)
    {
        STRING_HANDLE result;
        size_t num_elements = sizeof(test_val_len) / sizeof(test_val_len[0]);

        //arrange

        //act
        for (size_t index = 0; index < num_elements; index++)
        {
            char tmp_msg[64];
            sprintf(tmp_msg, "Base32_Encoder failure in test %zu", index);

            BUFFER_HANDLE input_buff = (BUFFER_HANDLE)&test_val_len[index];
            result = Base32_Encoder(input_buff);

            //assert
            ASSERT_ARE_EQUAL_WITH_MSG(char_ptr, test_val_len[index].base32_data, (const char*)result, tmp_msg);

            //cleanup
            my_STRING_delete(result);
        }
    }

    /* Tests_SRS_BASE32_07_002: [ If successful Base32_Encoder shall return the base32 value of source. ] */
    /* Tests_SRS_BASE32_07_003: [ Base32_Encoder shall call into base32_encoder_impl to encode the source data. ] */
    /* Tests_SRS_BASE32_07_012: [ Base32_Encoder shall wrap the base32_encoder_impl result into a STRING_HANDLE. ] */
    /* Tests_SRS_BASE32_07_009: [ base32_encode_impl shall allocate the buffer to the size of the encoding value. ] */
    /* Tests_SRS_BASE32_07_010: [ base32_encode_impl shall look through source and separate each block into 5 bit chunks ] */
    /* Tests_SRS_BASE32_07_011: [ base32_encode_impl shall then map the 5 bit chunks into one of the BASE32 values (a-z,2,3,4,5,6,7) values. ] */
    /* Tests_SRS_BASE32_07_012: [ If the src_size is not divisible by 8, base32_encode_impl shall pad the remaining places with =. ] */
    TEST_FUNCTION(Base32_Encoder_with_mocks_success)
    {
        STRING_HANDLE result;

        //arrange
        BUFFER_HANDLE input_buff = (BUFFER_HANDLE)&test_val_len[0];

        STRICT_EXPECTED_CALL(BUFFER_length(input_buff));
        STRICT_EXPECTED_CALL(BUFFER_u_char(input_buff));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        result = Base32_Encoder(input_buff);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, test_val_len[0].base32_data, (const char*)result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_STRING_delete(result);
    }

END_TEST_SUITE(base32_ut)
