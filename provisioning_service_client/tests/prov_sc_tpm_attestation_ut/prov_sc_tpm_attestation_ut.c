// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"

#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char*, string);
MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
MOCKABLE_FUNCTION(, const char*, json_object_get_string, const JSON_Object*, object, const char *, name);
MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, double, json_object_get_number, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, int, json_object_has_value, const JSON_Object*, object, const char*, name);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_value, JSON_Object*, object, const char*, name, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_string, JSON_Object*, object, const char*, name, const char*, string);
MOCKABLE_FUNCTION(, JSON_Status, json_object_set_number, JSON_Object*, object, const char*, name, double, number);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_object);
MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value*, value);
MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_wrapping_value, const JSON_Object*, object);
MOCKABLE_FUNCTION(, JSON_Value*, json_value_init_array);
MOCKABLE_FUNCTION(, JSON_Array*, json_value_get_array, const JSON_Value*, value);
MOCKABLE_FUNCTION(, JSON_Status, json_array_append_value, JSON_Array*, array, JSON_Value*, value);
MOCKABLE_FUNCTION(, size_t, json_array_get_count, const JSON_Array*, array);
MOCKABLE_FUNCTION(, JSON_Object*, json_array_get_object, const JSON_Array*, array, size_t, index);
MOCKABLE_FUNCTION(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_sc_tpm_attestation.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static const char* DUMMY_EK = "EK";
static const char* DUMMY_SRK = "SRK";
static char* DUMMY_JSON = "{\"json\":\"dummy\"}";
static const char* DUMMY_STRING = "dummy";

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static void register_global_mocks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);
    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    //parson
    REGISTER_GLOBAL_MOCK_RETURN(json_value_init_object, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_init_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_value_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_string, DUMMY_STRING);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, TEST_JSON_OBJECT);
    REGISTER_GLOBAL_MOCK_RETURN(json_parse_string, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_value, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_value, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_string, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_string, JSONFailure);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_wrapping_value, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_wrapping_value, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(json_serialize_to_string, DUMMY_JSON);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);
}

BEGIN_TEST_SUITE(prov_sc_tpm_attestation_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    register_global_mocks();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    if (skip_array != NULL)
    {
        for (size_t index = 0; index < length; index++)
        {
            if (current_index == skip_array[index])
            {
                result = __LINE__;
                break;
            }
        }
    }

    return result;
}

/* UNIT TESTS BEGIN */

TEST_FUNCTION(tpmAttestation_getEndorsementKey_null_handle)
{
    //arrange

    //act
    const char* res = tpmAttestation_getEndorsementKey(NULL);

    //assert
    ASSERT_IS_NULL(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(tpmAttestation_getEndorsementKey_success)
{
    //arrange
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    const char* res = tpmAttestation_getEndorsementKey(tpm_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_EK, res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_create_ek_only)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_EK));

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, NULL);

    //assert
    ASSERT_IS_NOT_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_EK, tpmAttestation_getEndorsementKey(tpm_att));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_create_ek_and_srk)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_EK));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_SRK));

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, DUMMY_SRK);

    //assert
    ASSERT_IS_NOT_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_EK, tpmAttestation_getEndorsementKey(tpm_att));
    //can't test that SRK was set because it's private
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_create_ek_null)
{
    //arrange

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(NULL, DUMMY_SRK);

    //assert
    ASSERT_IS_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(tpmAttestation_create_tags_and_srk_throw_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_EK));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_SRK));

    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0;
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "tpmAttestation_create_tags_and_srk_throw_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, DUMMY_SRK);

        //assert
        ASSERT_IS_NULL(tpm_att, tmp_msg);

        //cleanup
        tpmAttestation_destroy(tpm_att);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(tpmAttestation_destroy_success)
{
    //arrange
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, DUMMY_SRK);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    tpmAttestation_destroy(tpm_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(tpmAttestation_destroy_null_ptr)
{
    //arrange

    //act
    tpmAttestation_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(tpmAttestation_toJson_null)
{
    //arrange

    //act
    JSON_Value* json_val = tpmAttestation_toJson(NULL);

    //assert
    ASSERT_IS_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(tpmAttestation_toJson_only_ek)
{
    //arrange
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    JSON_Value* json_val = tpmAttestation_toJson(tpm_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_IS_TRUE(json_val == TEST_JSON_VALUE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_toJson_full_tpm)
{
    //arrange
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, DUMMY_SRK);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    JSON_Value* json_val = tpmAttestation_toJson(tpm_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_IS_TRUE(json_val == TEST_JSON_VALUE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_toJson_full_tpm_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_create(DUMMY_EK, DUMMY_SRK);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0;
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "tpmAttestation_toJson_full_tpm_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* result = tpmAttestation_toJson(tpm_att);

        //assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    //cleanup
    tpmAttestation_destroy(tpm_att);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(tpmAttestation_fromJson_null)
{
    //arrange

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_fromJson(NULL);

    //assert
    ASSERT_IS_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(tpmAttestation_fromJson_min)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_EK);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL);

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_EK, tpmAttestation_getEndorsementKey(tpm_att));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_fromJson_max)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_EK);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SRK);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(tpm_att);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_EK, tpmAttestation_getEndorsementKey(tpm_att));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    tpmAttestation_destroy(tpm_att);
}

TEST_FUNCTION(tpmAttestation_fromJson_min_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_EK); //can't really "fail" (yet)
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //can't really "fail"

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "tpmAttestation_fromJson_min_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(tpm_att, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(tpmAttestation_fromJson_max_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_EK); //can't really "fail" (yet)
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SRK); //can't really "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "tpmAttestation_fromJson_max_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        TPM_ATTESTATION_HANDLE tpm_att = tpmAttestation_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(tpm_att, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

END_TEST_SUITE(prov_sc_tpm_attestation_ut);
