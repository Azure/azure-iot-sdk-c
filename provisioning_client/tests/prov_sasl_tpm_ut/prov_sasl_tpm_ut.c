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
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/base64.h"

MOCKABLE_FUNCTION(, char*, on_challenge_callback, BUFFER_HANDLE, data, void*, user_ctx);

#undef ENABLE_MOCKS

#include "azure_prov_client/prov_sasl_tpm.h"

#define TEST_BUFFER_SIZE        32
#define TEST_KEY_SIZE           10
#define TEST_STRING_LEN         17
#define TEST_CHUNK_BUFF_SIZE    512
#define MAX_FRAME_DATA_LEN      470

static const char* MECHANISM_NAME = "TPM";
static const char* TEST_DPS_URI = "dps_uri";
static const char* TEST_SAS_TOKEN = "sas_token";
static const char* TEST_STRING_VALUE = "test string value";

static const char* TEST_REG_ID = "1234567890";
static unsigned char TEST_BUFFER[TEST_BUFFER_SIZE];

static BUFFER_HANDLE TEST_BUFFER_HANDLE = (BUFFER_HANDLE)0x00011;

static SASL_MECHANISM_CREATE prov_sasl_mechanism_create;
static SASL_MECHANISM_DESTROY prov_sasl_mechanism_destroy;
static SASL_MECHANISM_GET_INIT_BYTES prov_sasl_mechanism_get_init_bytes;
static SASL_MECHANISM_GET_MECHANISM_NAME prov_sasl_mechanism_get_mechanism_name;
static SASL_MECHANISM_CHALLENGE prov_sasl_mechanism_challenge;

static BUFFER_HANDLE my_BUFFER_create(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_clone(BUFFER_HANDLE handle)
{
    (void)handle;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static BUFFER_HANDLE my_BUFFER_new(void)
{
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static STRING_HANDLE my_STRING_new(void)
{
    return (STRING_HANDLE)my_gballoc_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    my_gballoc_free(handle);
}

static BUFFER_HANDLE my_Base64_Decoder(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}

static void my_BUFFER_delete(BUFFER_HANDLE handle)
{
    my_gballoc_free(handle);
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static char* my_on_challenge_callback(BUFFER_HANDLE data, void* user_ctx)
{
    (void)data;
    (void)user_ctx;
    char* result;
    size_t src_len = strlen(TEST_SAS_TOKEN);
    result = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(result, TEST_SAS_TOKEN);
    return result;
}

static STRING_HANDLE my_STRING_from_byte_array(const unsigned char* source, size_t size)
{
    (void)source;
    (void)size;
    return (STRING_HANDLE)my_gballoc_malloc(1);
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

BEGIN_TEST_SUITE(prov_sasl_tpm_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, my_BUFFER_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, my_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, my_BUFFER_delete);

    REGISTER_GLOBAL_MOCK_HOOK(on_challenge_callback, my_on_challenge_callback);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(on_challenge_callback, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(Base64_Decoder, my_Base64_Decoder);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Decoder, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_new, my_STRING_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_concat_with_STRING, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, __LINE__);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
    REGISTER_GLOBAL_MOCK_RETURN(STRING_length, TEST_STRING_LEN);
    REGISTER_GLOBAL_MOCK_HOOK(STRING_from_byte_array, my_STRING_from_byte_array);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_from_byte_array, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_append_build, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, TEST_BUFFER);
    REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, TEST_BUFFER_SIZE);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

    TEST_BUFFER[0] = 0xc1;
    for (size_t index = 1; index < TEST_BUFFER_SIZE; index++)
    {
        TEST_BUFFER[index] = (unsigned char)(index + 1);
    }

    /* Tests_SRS_PROV_SASL_TPM_07_021: [ prov_sasltpm_get_interface shall return the SASL_MECHANISM_INTERFACE_DESCRIPTION structure. ] */
    prov_sasl_mechanism_create = prov_sasltpm_get_interface()->concrete_sasl_mechanism_create;
    prov_sasl_mechanism_destroy = prov_sasltpm_get_interface()->concrete_sasl_mechanism_destroy;
    prov_sasl_mechanism_get_init_bytes = prov_sasltpm_get_interface()->concrete_sasl_mechanism_get_init_bytes;
    prov_sasl_mechanism_get_mechanism_name = prov_sasltpm_get_interface()->concrete_sasl_mechanism_get_mechanism_name;
    prov_sasl_mechanism_challenge = prov_sasltpm_get_interface()->concrete_sasl_mechanism_challenge;
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

static void setup_dps_hsm_tpm_create_mock(void)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_new());
}

static void setup_prov_sasl_mechanism_challenge_sastoken_mock(bool last_seq)
{
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    if (!last_seq)
    {
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(Base64_Decoder(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(on_challenge_callback(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

/* Tests_SRS_PROV_SASL_TPM_07_002: [ If config is NULL, prov_sasltpm_create shall return NULL. ] */
TEST_FUNCTION(prov_sasltpm_create_config_NULL_fail)
{
    //arrange

    //act
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(NULL);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup

}

/* Tests_SRS_PROV_SASL_TPM_07_004: [ if SASL_TPM_CONFIG_INFO, challenge_cb, endorsement_key, storage_root_key, or hostname, registration_id is NULL, prov_sasltpm_create shall fail and return NULL. ] */
TEST_FUNCTION(prov_sasltpm_create_ek_NULL_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = NULL;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;

    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_004: [ if SASL_TPM_CONFIG_INFO, challenge_cb, endorsement_key, storage_root_key, or hostname, registration_id is NULL, prov_sasltpm_create shall fail and return NULL. ] */
TEST_FUNCTION(prov_sasltpm_create_challenge_cb_NULL_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = NULL;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;

    //arrange

    //act
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    //assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_001: [ On success prov_sasltpm_create shall allocate a new instance of the SASL_TPM_INSTANCE. ] */
/* Tests_SRS_PROV_SASL_TPM_07_029: [ prov_sasltpm_create shall copy the config data where needed. ] */
TEST_FUNCTION(prov_sasltpm_create_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;

    //arrange
    setup_dps_hsm_tpm_create_mock();

    //act
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    //assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_003: [ If any error is encountered, prov_sasltpm_create shall return NULL. ] */
TEST_FUNCTION(dps_hsm_tpm_create_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;

    //arrange
    setup_dps_hsm_tpm_create_mock();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_negative_tests_snapshot();

    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "prov_sasl_mechanism_create failure in test %zu/%zu", index, count);

        //act
        CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

        //assert
        ASSERT_IS_NULL_WITH_MSG(handle, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

/* Tests_SRS_PROV_SASL_TPM_07_005: [ if handle is NULL, prov_sasltpm_destroy shall do nothing. ] */
TEST_FUNCTION(prov_sasltpm_destroy_handle_NULL_fail)
{
    //arrange

    //act
    prov_sasl_mechanism_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_PROV_SASL_TPM_07_006: [ prov_sasltpm_create shall free the SASL_TPM_INSTANCE instance. ] */
/* Tests_SRS_PROV_SASL_TPM_07_007: [ prov_sasltpm_create shall free all resources allocated in this module. ] */
TEST_FUNCTION(prov_sasltpm_destroy_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    prov_sasl_mechanism_destroy(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_PROV_SASL_TPM_07_008: [ if handle is NULL, prov_sasltpm_get_mechanism_name shall return NULL. ] */
TEST_FUNCTION(prov_sasltpm_get_mechanism_name_handle_NULL_fail)
{
    //arrange

    //act
    const char* mechanism_name = prov_sasl_mechanism_get_mechanism_name(NULL);

    //assert
    ASSERT_IS_NULL(mechanism_name);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_PROV_SASL_TPM_07_009: [ prov_sasltpm_get_mechanism_name shall return the mechanism name TPM. ] */
TEST_FUNCTION(prov_sasltpm_get_mechanism_name_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);
    umock_c_reset_all_calls();

    //arrange

    //act
    const char* mechanism_name = prov_sasl_mechanism_get_mechanism_name(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, MECHANISM_NAME, mechanism_name);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_010: [ If handle or init_bytes are NULL, prov_sasltpm_get_init_bytes shall return NULL. ] */
TEST_FUNCTION(prov_sasltpm_get_init_bytes_handle_NULL_fail)
{
    SASL_MECHANISM_BYTES init_bytes;
    init_bytes.bytes = TEST_BUFFER;
    init_bytes.length = TEST_BUFFER_SIZE;

    //arrange

    //act
    int reply = prov_sasl_mechanism_get_init_bytes(NULL, &init_bytes);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_PROV_SASL_TPM_07_028: [ If the data is less than 470 bytes the control byte shall be set to 0. ] */
/* Tests_SRS_PROV_SASL_TPM_07_012: [ prov_sasltpm_get_init_bytes shall send the control byte along with the EK value, ctrl byte detailed in Send Data to dps sasltpm ] */
/* Tests_SRS_PROV_SASL_TPM_07_014: [ On success prov_sasltpm_get_init_bytes shall return a zero value. ] */
TEST_FUNCTION(prov_sasltpm_get_init_bytes_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES init_bytes;
    init_bytes.bytes = NULL;
    init_bytes.length = 0;
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //act
    int reply = prov_sasl_mechanism_get_init_bytes(handle, &init_bytes);

    //assert
    ASSERT_ARE_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_024: [ If the data to be sent is greater than 470 bytes the data will be chunked to the server. ] */
/* Tests_SRS_PROV_SASL_TPM_07_026: [ If the current bytes are the last chunk in the sequence construct_send_data shall mark the Last seg Sent bit ] */
/* Tests_SRS_PROV_SASL_TPM_07_025: [ If data is chunked a single control byte will be prepended to the data as described below: ] */
/* Tests_SRS_PROV_SASL_TPM_07_027: [ The sequence number shall be incremented after every send of chunked bytes. ] */
TEST_FUNCTION(prov_sasltpm_get_init_bytes_chunk_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    unsigned char TEST_CHUNK_BUFFER[TEST_CHUNK_BUFF_SIZE];
    for (size_t index = 0; index < TEST_CHUNK_BUFF_SIZE; index++)
    {
        TEST_CHUNK_BUFFER[index] = (unsigned char)(index + 1);
    }

    SASL_MECHANISM_BYTES init_bytes;
    init_bytes.bytes = NULL;
    init_bytes.length = 0;
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG)).SetReturn(TEST_CHUNK_BUFFER);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG)).SetReturn(TEST_CHUNK_BUFF_SIZE);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //act
    int reply = prov_sasl_mechanism_get_init_bytes(handle, &init_bytes);

    //assert
    unsigned char* test_bytes = ((unsigned char*)init_bytes.bytes) + 1;
    ASSERT_ARE_EQUAL(int, MAX_FRAME_DATA_LEN, init_bytes.length);
    ASSERT_ARE_EQUAL(int, 0, memcmp(TEST_CHUNK_BUFFER, test_bytes, init_bytes.length-1) );
    ASSERT_ARE_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_013: [ If any error is encountered, prov_sasltpm_get_init_bytes shall return a non-zero value. ] */
TEST_FUNCTION(prov_sasltpm_get_init_bytes_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES init_bytes;
    init_bytes.bytes = TEST_BUFFER;
    init_bytes.length = TEST_BUFFER_SIZE;
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

    //act
    int reply = prov_sasl_mechanism_get_init_bytes(handle, &init_bytes);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_015: [ if handle or resp_bytes are NULL, prov_sasltpm_challenge shall return the X509_SECURITY_INTERFACE structure. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_handle_NULL_succeed)
{
    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange

    //act
    int reply = prov_sasl_mechanism_challenge(NULL, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/* Tests_SRS_PROV_SASL_TPM_07_016: [ If the challenge_bytes->bytes first byte is NULL prov_sasltpm_challenge shall send the SRK data to the server. ] */
/* Tests_SRS_PROV_SASL_TPM_07_022: [ prov_sasltpm_challenge shall send the control byte along with the SRK value, ctrl byte detailed in Send Data to dps sasl tpm ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_srk_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //act
    int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_ARE_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_020: [ If any error is encountered prov_sasltpm_challenge shall return a non-zero value. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_srk_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

    //act
    int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_017: [ prov_sasltpm_challenge accumulates challenge bytes and waits for the last sequence bit to be set. ] */
/* Tests_SRS_PROV_SASL_TPM_07_019: [ The Sas Token shall be put into the response bytes buffer to be return to the DPS SASL server. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_sastoken_last_seq_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0xC0;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange
    setup_prov_sasl_mechanism_challenge_sastoken_mock(false);

    //act
    int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_IS_NOT_NULL(reply_bytes.bytes);
    ASSERT_ARE_EQUAL(int, (int)strlen(TEST_SAS_TOKEN)+1, (int)reply_bytes.length);
    ASSERT_ARE_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_023: [ If the last sequence bit is not encountered prov_sasltpm_challenge shall return 1 byte to the service. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_sastoken_not_last_seq_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0x80;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange
    setup_prov_sasl_mechanism_challenge_sastoken_mock(true);

    //act
    int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_IS_NOT_NULL(reply_bytes.bytes);
    ASSERT_ARE_EQUAL(int, 1, reply_bytes.length);
    ASSERT_ARE_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_020: [ If any error is encountered prov_sasltpm_challenge shall return a non-zero value. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_sastoken_invalid_seq_succeed)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;
    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0x83;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(STRING_from_byte_array(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, reply);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sasl_mechanism_destroy(handle);
}

/* Tests_SRS_PROV_SASL_TPM_07_020: [ If any error is encountered prov_sasltpm_challenge shall return a non-zero value. ] */
TEST_FUNCTION(prov_sasl_mechanism_challenge_sastoken_fail)
{
    SASL_TPM_CONFIG_INFO sasl_tpm_config;
    sasl_tpm_config.challenge_cb = on_challenge_callback;
    sasl_tpm_config.endorsement_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.storage_root_key = TEST_BUFFER_HANDLE;
    sasl_tpm_config.registration_id = TEST_REG_ID;
    sasl_tpm_config.hostname = TEST_DPS_URI;

    CONCRETE_SASL_MECHANISM_HANDLE handle = prov_sasl_mechanism_create(&sasl_tpm_config);

    SASL_MECHANISM_BYTES challenge_bytes;
    TEST_BUFFER[0] = 0xC0;
    challenge_bytes.bytes = TEST_BUFFER;
    challenge_bytes.length = TEST_BUFFER_SIZE;
    SASL_MECHANISM_BYTES reply_bytes = { 0 };
    umock_c_reset_all_calls();

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    setup_prov_sasl_mechanism_challenge_sastoken_mock(false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 2, 4, 6, 7, 8 };

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
        sprintf(tmp_msg, "prov_sasl_mechanism_challenge failure in test %zu/%zu", index, count);

        //act
        int reply = prov_sasl_mechanism_challenge(handle, &challenge_bytes, &reply_bytes);

        //assert
        ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, reply, tmp_msg);
    }

    //cleanup
    prov_sasl_mechanism_destroy(handle);
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(prov_sasl_tpm_ut)
