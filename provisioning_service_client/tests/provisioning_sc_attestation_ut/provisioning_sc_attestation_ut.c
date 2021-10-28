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
#include "azure_c_shared_utility/const_defines.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"
#include "prov_service_client/provisioning_sc_tpm_attestation.h"
#include "prov_service_client/provisioning_sc_x509_attestation.h"

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

#include "prov_service_client/provisioning_sc_attestation_mechanism.h"
#include "prov_service_client/provisioning_sc_json_const.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static char* DUMMY_EK = "DUMMY_EK";
static char* DUMMY_SRK = "DUMMY_SRK";
static char* DUMMY_CERT1 = "DUMMY_CERT1";
static char* DUMMY_CERT2 = "DUMMY_CERT2";
static char* DUMMY_REF1 = "DUMMY_REF1";
static char* DUMMY_REF2 = "DUMMY_REF2";
static char* DUMMY_JSON = "{\"json\":\"dummy\"}";
static const char* DUMMY_STRING = "dummy";
static const char* INVALID_STRING = "invalid";

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

static TPM_ATTESTATION_HANDLE my_tpmAttestation_create(const char* endorsement_key, const char* storage_root_key)
{
    AZURE_UNREFERENCED_PARAMETER(endorsement_key);
    AZURE_UNREFERENCED_PARAMETER(storage_root_key);
    return (TPM_ATTESTATION_HANDLE)real_malloc(1);
}

static void my_tpmAttestation_destroy(TPM_ATTESTATION_HANDLE tpm_att)
{
    real_free(tpm_att);
}

static TPM_ATTESTATION_HANDLE my_tpmAttestation_fromJson(JSON_Object* root_object)
{
    AZURE_UNREFERENCED_PARAMETER(root_object);
    return (TPM_ATTESTATION_HANDLE)real_malloc(1);
}

static JSON_Value* my_tpmAttestation_toJson(const TPM_ATTESTATION_HANDLE tpm_att)
{
    AZURE_UNREFERENCED_PARAMETER(tpm_att);
    return TEST_JSON_VALUE;
}

static X509_ATTESTATION_HANDLE my_x509Attestation_create(X509_CERTIFICATE_TYPE cert_type, const char* primary_cert, const char* secondary_cert)
{
    AZURE_UNREFERENCED_PARAMETER(cert_type);
    AZURE_UNREFERENCED_PARAMETER(primary_cert);
    AZURE_UNREFERENCED_PARAMETER(secondary_cert);

    return (X509_ATTESTATION_HANDLE)real_malloc(1);
}

static void my_x509Attestation_destroy(X509_ATTESTATION_HANDLE x509_att)
{
    real_free(x509_att);
}

static X509_ATTESTATION_HANDLE my_x509Attestation_fromJson(JSON_Object* root_object)
{
    AZURE_UNREFERENCED_PARAMETER(root_object);
    return (X509_ATTESTATION_HANDLE)real_malloc(1);
}

static JSON_Value* my_x509Attestation_toJson(const X509_ATTESTATION_HANDLE x509_att)
{
    AZURE_UNREFERENCED_PARAMETER(x509_att);
    return TEST_JSON_VALUE;
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
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);
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

    //tpm attestation
    REGISTER_UMOCK_ALIAS_TYPE(TPM_ATTESTATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const TPM_ATTESTATION_HANDLE, void*);
    REGISTER_GLOBAL_MOCK_HOOK(tpmAttestation_create, my_tpmAttestation_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tpmAttestation_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(tpmAttestation_destroy, my_tpmAttestation_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(tpmAttestation_fromJson, my_tpmAttestation_fromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tpmAttestation_fromJson, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(tpmAttestation_toJson, my_tpmAttestation_toJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tpmAttestation_toJson, NULL);

    //x509 attestation
    REGISTER_UMOCK_ALIAS_TYPE(X509_ATTESTATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const X509_ATTESTATION_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(X509_CERTIFICATE_TYPE, int);
    REGISTER_GLOBAL_MOCK_HOOK(x509Attestation_create, my_x509Attestation_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(x509Attestation_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(x509Attestation_destroy, my_x509Attestation_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(x509Attestation_fromJson, my_x509Attestation_fromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(x509Attestation_fromJson, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(x509Attestation_toJson, my_x509Attestation_toJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(x509Attestation_toJson, NULL);
}

BEGIN_TEST_SUITE(provisioning_sc_attestation_ut)

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

TEST_FUNCTION(attestationMechanism_createWithTpm_null_ek)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(NULL, DUMMY_SRK);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_createWithTpm_null_srk)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_create(DUMMY_EK, NULL));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getTpmAttestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_TPM);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithTpm_ek_and_srk)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_create(DUMMY_EK, DUMMY_SRK));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, DUMMY_SRK);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getTpmAttestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_TPM);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithTpm_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_create(DUMMY_EK, DUMMY_SRK));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_createWithTpm_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, DUMMY_SRK);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_createWithX509ClientCert_null_primary_cert)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(NULL, DUMMY_CERT2);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_createWithX509ClientCert_null_secondary_cert)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509ClientCert_both_certs)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, DUMMY_CERT2);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509ClientCert_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_createWithX509ClientCert_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, DUMMY_CERT2);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_createWithX509SigningCert_null_primary_cert)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(NULL, DUMMY_CERT2);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_createWithX509SigningCert_null_secondary_cert)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(DUMMY_CERT1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509SigningCert_both_certs)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(DUMMY_CERT1, DUMMY_CERT2);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509SigningCert_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_createWithX509SigningCert_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(DUMMY_CERT1, DUMMY_CERT2);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_createWithX509CAReference_null_primary_ref)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(NULL, DUMMY_REF2);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_createWithX509CAReference_null_secondary_ref)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509CAReference_both_refs)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, DUMMY_REF2);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_createWithX509CAReference_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_createWithX509CAReference_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, DUMMY_REF2);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_destroy_null_handle)
{
    //arrange

    //act
    attestationMechanism_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_destroy_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(tpmAttestation_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(att));

    //act
    attestationMechanism_destroy(att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_destroy_x509)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(att));

    //act
    attestationMechanism_destroy(att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_isValidForIndividualEnrollment_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    bool res = attestationMechanism_isValidForIndividualEnrollment(att);

    //assert
    ASSERT_IS_TRUE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForIndividualEnrollment_x509_client)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_CLIENT);

    //act
    bool res = attestationMechanism_isValidForIndividualEnrollment(att);

    //assert
    ASSERT_IS_TRUE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForIndividualEnrollment_x509_signing)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_SIGNING);

    //act
    bool res = attestationMechanism_isValidForIndividualEnrollment(att);

    //assert
    ASSERT_IS_FALSE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForIndividualEnrollment_x509_ca)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_CA_REFERENCES);

    //act
    bool res = attestationMechanism_isValidForIndividualEnrollment(att);

    //assert
    ASSERT_IS_FALSE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForIndividualEnrollment_x509_invalid)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_NONE); //this shouldn't happen (indicates error)

    //act
    bool res = attestationMechanism_isValidForIndividualEnrollment(att);

    //assert
    ASSERT_IS_FALSE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForEnrollmentGroup_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    bool res = attestationMechanism_isValidForEnrollmentGroup(att);

    //assert
    ASSERT_IS_FALSE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForEnrollmentGroup_x509_client)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_CLIENT);

    //act
    bool res = attestationMechanism_isValidForEnrollmentGroup(att);

    //assert
    ASSERT_IS_FALSE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForEnrollmentGroup_x509_signing)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509SigningCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_SIGNING);

    //act
    bool res = attestationMechanism_isValidForEnrollmentGroup(att);

    //assert
    ASSERT_IS_TRUE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForEnrollmentGroup_x509_ca)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_NONE); //this shouldn't happen (indicates an error)

    //act
    bool res = attestationMechanism_isValidForEnrollmentGroup(att);

    //assert
    ASSERT_IS_TRUE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_isValidForEnrollmentGroup_x509_invalid)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509CAReference(DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(x509Attestation_getCertificateType(IGNORED_PTR_ARG)).SetReturn(X509_CERTIFICATE_TYPE_CA_REFERENCES);

    //act
    bool res = attestationMechanism_isValidForEnrollmentGroup(att);

    //assert
    ASSERT_IS_TRUE(res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getType_null)
{
    //arrange

    //act
    ATTESTATION_TYPE type = attestationMechanism_getType(NULL);

    //assert
    ASSERT_IS_TRUE(type == ATTESTATION_TYPE_NONE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(attestationMechanism_getType_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    ATTESTATION_TYPE type = attestationMechanism_getType(att);

    //assert
    ASSERT_IS_TRUE(type == ATTESTATION_TYPE_TPM);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getType_x509)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    ATTESTATION_TYPE type = attestationMechanism_getType(att);

    //assert
    ASSERT_IS_TRUE(type == ATTESTATION_TYPE_X509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getTpmAttestation_null)
{
    //arrange

    //act
    TPM_ATTESTATION_HANDLE tpm = attestationMechanism_getTpmAttestation(NULL);

    //assert
    ASSERT_IS_NULL(tpm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_getTpmAttestation_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    TPM_ATTESTATION_HANDLE tpm = attestationMechanism_getTpmAttestation(att);

    //assert
    ASSERT_IS_NOT_NULL(tpm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getTpmAttestation_x509)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    TPM_ATTESTATION_HANDLE tpm = attestationMechanism_getTpmAttestation(att);

    //assert
    ASSERT_IS_NULL(tpm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getX509Attestation_null)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509 = attestationMechanism_getX509Attestation(NULL);

    //assert
    ASSERT_IS_NULL(x509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_getX509Attestation_x509)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    X509_ATTESTATION_HANDLE x509 = attestationMechanism_getX509Attestation(att);

    //assert
    ASSERT_IS_NOT_NULL(x509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_getX509Attestation_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    //act
    X509_ATTESTATION_HANDLE x509 = attestationMechanism_getX509Attestation(att);

    //assert
    ASSERT_IS_NULL(x509);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_fromJson_null)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(NULL);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_fromJson_invalid_attestation_type)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, IGNORED_PTR_ARG)).SetReturn(INVALID_STRING);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NULL(att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_fromJson_tpm)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, IGNORED_PTR_ARG)).SetReturn(ATTESTATION_TYPE_JSON_VALUE_TPM);
    STRICT_EXPECTED_CALL(json_object_get_object(TEST_JSON_OBJECT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_fromJson(IGNORED_PTR_ARG));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_TPM);
    ASSERT_IS_NOT_NULL(attestationMechanism_getTpmAttestation(att));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_fromJson_tpm_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, IGNORED_PTR_ARG)).SetReturn(ATTESTATION_TYPE_JSON_VALUE_TPM);
    STRICT_EXPECTED_CALL(json_object_get_object(TEST_JSON_OBJECT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_fromJson(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_fromJson_tpm_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_fromJson_x509)
{
    //arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, IGNORED_PTR_ARG)).SetReturn(ATTESTATION_TYPE_JSON_VALUE_X509);
    STRICT_EXPECTED_CALL(json_object_get_object(TEST_JSON_OBJECT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_fromJson(IGNORED_PTR_ARG));

    //act
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_TRUE(attestationMechanism_getType(att) == ATTESTATION_TYPE_X509);
    ASSERT_IS_NOT_NULL(attestationMechanism_getX509Attestation(att));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_fromJson_x509_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(TEST_JSON_OBJECT, IGNORED_PTR_ARG)).SetReturn(ATTESTATION_TYPE_JSON_VALUE_X509);
    STRICT_EXPECTED_CALL(json_object_get_object(TEST_JSON_OBJECT, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_fromJson(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_fromJson_tpm_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(attestationMechanism_toJson_null_handle)
{
    //arrange

    //act
    JSON_Value* result = attestationMechanism_toJson(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(attestationMechanism_toJson_tpm)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    JSON_Value* result = attestationMechanism_toJson(att);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_toJson_tpm_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithTpm(DUMMY_EK, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(tpmAttestation_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_toJson_tpm_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* result = attestationMechanism_toJson(att);

        //assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    umock_c_negative_tests_deinit();

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_toJson_x509)
{
    //arrange
    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    JSON_Value* result = attestationMechanism_toJson(att);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    attestationMechanism_destroy(att);
}

TEST_FUNCTION(attestationMechanism_toJson_x509_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    ATTESTATION_MECHANISM_HANDLE att = attestationMechanism_createWithX509ClientCert(DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(x509Attestation_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "attestationMechanism_toJson_x509_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* result = attestationMechanism_toJson(att);

        //assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    umock_c_negative_tests_deinit();

    //cleanup
    attestationMechanism_destroy(att);
}

END_TEST_SUITE(provisioning_sc_attestation_ut);
