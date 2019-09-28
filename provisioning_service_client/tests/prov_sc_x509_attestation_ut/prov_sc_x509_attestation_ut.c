// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
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
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/strings.h"
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

#include "prov_service_client/provisioning_sc_x509_attestation.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

//Control Parameters
static char* DUMMY_CERT1 = "DUMMY_CERT1";
static char* DUMMY_CERT1_64 = "DUMMY_CERT1_64";
static char* DUMMY_CERT2 = "DUMMY_CERT2";
static char* DUMMY_CERT2_64 = "DUMMY_CERT2_64";
static char* DUMMY_REF1 = "DUMMY_REF1";
static char* DUMMY_REF2 = "DUMMY_REF2";
static char* DUMMY_JSON = "{\"json\":\"dummy\"}";

static const char* DUMMY_STRING = "dummy";
static const char* DUMMY_STRING_64 = "dummy_64";
static int DUMMY_NUMBER = 1111;
static const char* DUMMY_SUBJECT_NAME = "SUBJECT_NAME";
static const char* DUMMY_SHA1_THUMBPRINT = "SHA1_THUMBPRINT";
static const char* DUMMY_SHA256_THUMBPRINT = "SHA256_THUMBPRINT";
static const char* DUMMY_ISSUER_NAME = "ISSUER_NAME";
static const char* DUMMY_NOT_BEFORE_UTC = "NOT_BEFORE_UTC";
static const char* DUMMY_NOT_AFTER_UTC = "NOT_AFTER_UTC";
static const char* DUMMY_SERIAL_NUMBER = "SERIAL_NUMBER";
static int DUMMY_VERSION = 4747;

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
#define TEST_STRING_HANDLE (STRING_HANDLE)0x11111113

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
    REGISTER_GLOBAL_MOCK_RETURN(json_object_get_number, DUMMY_NUMBER);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_number, 0);
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_number, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_number, JSONFailure);

    //base64
    REGISTER_GLOBAL_MOCK_RETURN(Azure_Base64_Encode_Bytes, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Azure_Base64_Encode_Bytes, NULL);

    //string
    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, DUMMY_STRING_64);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
}

BEGIN_TEST_SUITE(prov_sc_x509_attestation_ut)

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

static void assert_is_valid_cert(X509_CERTIFICATE_HANDLE cert)
{
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SUBJECT_NAME, x509Certificate_getSubjectName(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SHA1_THUMBPRINT, x509Certificate_getSha1Thumbprint(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SHA256_THUMBPRINT, x509Certificate_getSha256Thumbprint(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ISSUER_NAME, x509Certificate_getIssuerName(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_NOT_BEFORE_UTC, x509Certificate_getNotBeforeUtc(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_NOT_AFTER_UTC, x509Certificate_getNotAfterUtc(cert));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SERIAL_NUMBER, x509Certificate_getSerialNumber(cert));
    ASSERT_ARE_EQUAL(int, DUMMY_VERSION, x509Certificate_getVersion(cert));
}

static void expected_calls_x509CertificateInfo_free(bool is_processed)
{
    if (is_processed)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static void expected_calls_x509CAReferences_free(bool has_secondary_ref)
{
    (void)has_secondary_ref;
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_x509CertificateWithInfo_free(bool is_processed)
{
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    expected_calls_x509CertificateInfo_free(is_processed);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_x509Certificates_free(bool has_secondary_cert, bool is_processed)
{
    expected_calls_x509CertificateWithInfo_free(is_processed);
    if (has_secondary_cert)
    {
        expected_calls_x509CertificateWithInfo_free(is_processed);
    }
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE cert_type, bool has_secondary_cert, bool is_processed)
{
    if (cert_type == X509_CERTIFICATE_TYPE_CLIENT || cert_type == X509_CERTIFICATE_TYPE_SIGNING)
    {
        expected_calls_x509Certificates_free(has_secondary_cert, is_processed);
    }
    else if (cert_type == X509_CERTIFICATE_TYPE_CA_REFERENCES)
    {
        expected_calls_x509CAReferences_free(has_secondary_cert);
    }
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_x509CAReferences_create(const char* primary_ref, const char* secondary_ref)
{
    (void)primary_ref;
    (void)secondary_ref;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (secondary_ref != NULL)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_x509CertificateWithInfo_create(const char* cert)
{
    (void)cert;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void expected_calls_x509Certificates_create(const char* primary_cert, const char* secondary_cert)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    expected_calls_x509CertificateWithInfo_create(primary_cert);
    if (secondary_cert != NULL)
    {
        expected_calls_x509CertificateWithInfo_create(secondary_cert);
    }
}

static void expected_calls_convert_cert_to_b64(const char* in, const char* out)
{
    STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes((const unsigned char*)in, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)).SetReturn(out);
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE cert_type, const char* primary_cert, const char* secondary_cert)
{
    expected_calls_convert_cert_to_b64(primary_cert, DUMMY_CERT1_64);
    primary_cert = DUMMY_CERT1_64;
    if (secondary_cert != NULL)
    {
        expected_calls_convert_cert_to_b64(secondary_cert, DUMMY_CERT2_64);
        secondary_cert = DUMMY_CERT2_64;
    }
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (cert_type == X509_CERTIFICATE_TYPE_CLIENT || cert_type == X509_CERTIFICATE_TYPE_SIGNING)
    {
        expected_calls_x509Certificates_create(primary_cert, secondary_cert);
    }
    else if (cert_type == X509_CERTIFICATE_TYPE_CA_REFERENCES)
    {
        expected_calls_x509CAReferences_create(primary_cert, secondary_cert);
    }
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_x509CertificateInfo_fromJson() //7 - 27
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SUBJECT_NAME); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SHA1_THUMBPRINT); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SHA256_THUMBPRINT); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_ISSUER_NAME); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_NOT_BEFORE_UTC); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_NOT_AFTER_UTC); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_SERIAL_NUMBER); //can't fail for test purpose
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_get_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_VERSION); //can't fail for test purpose
} //23 - 43

static void expected_calls_x509CertificateWithInfo_fromJson()
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //this shouldn't ever successfully return - certs are only sent, not received
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    expected_calls_x509CertificateInfo_fromJson();
}

static void expected_calls_x509Certificates_fromJson(bool has_secondary)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    //first cert
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    expected_calls_x509CertificateWithInfo_fromJson();

    //secondary cert
    if (has_secondary)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //optional so can't "fail"
        expected_calls_x509CertificateWithInfo_fromJson();
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //optional so can't "fail"
    }
}

static void expected_calls_x509CAReferences_fromJson(bool has_secondary)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_REF1); //cannot "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    if (has_secondary)
    {
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_REF2); //cannot "fail"
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot "fail"
    }
}

static void expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE cert_type, bool has_secondary)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    if (cert_type == X509_CERTIFICATE_TYPE_CLIENT)
    {
        STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(true); //cannot "fail"
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        expected_calls_x509Certificates_fromJson(has_secondary);
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(false); //cannot "fail"

        if (cert_type == X509_CERTIFICATE_TYPE_SIGNING)
        {
            STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(true); //cannot "fail"
            STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            expected_calls_x509Certificates_fromJson(has_secondary);
        }
        else
        {
            STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(false); //cannot "fail"

            if (cert_type == X509_CERTIFICATE_TYPE_CA_REFERENCES)
            {
                STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(true); //cannot "fail"
                STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
                expected_calls_x509CAReferences_fromJson(has_secondary);
            }
            else
            {
                STRICT_EXPECTED_CALL(json_object_has_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(false);
                //x509Attestation_destroy handle this later
            }
        }
    }
}

static void expected_calls_x509CAReferences_toJson(bool has_secondary)
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (has_secondary)
    {
        STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_x509CertificateInfo_toJson()
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_number(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
}

static void expected_calls_x509CertificateWithInfo_toJson(bool is_processed)
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));

    if (is_processed)
    {
        expected_calls_x509CertificateInfo_toJson();
        STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_x509Certificates_toJson(bool has_secondary, bool is_processed)
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    expected_calls_x509CertificateWithInfo_toJson(is_processed);
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    if (has_secondary)
    {
        expected_calls_x509CertificateWithInfo_toJson(is_processed);
        STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE cert_type, bool has_secondary, bool is_processed)
{
    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));

    if (cert_type == X509_CERTIFICATE_TYPE_CLIENT || cert_type == X509_CERTIFICATE_TYPE_SIGNING)
    {
        expected_calls_x509Certificates_toJson(has_secondary, is_processed);
    }
    else //(cert_type == X509_CERTIFICATE_TYPE_CA_REFERENCES)
    {
        expected_calls_x509CAReferences_toJson(has_secondary);
    }

    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static X509_ATTESTATION_HANDLE create_dummy_processed_attestation(X509_CERTIFICATE_TYPE cert_type, bool has_secondary_cert)
{
    expected_calls_x509Attestation_fromJson(cert_type, has_secondary_cert);
    X509_ATTESTATION_HANDLE dummy = x509Attestation_fromJson(TEST_JSON_OBJECT);
    umock_c_reset_all_calls();
    return dummy;
}

/* UNIT TESTS BEGIN */

/*Tests_X509_ATTESTATION_22_001: [ If the given handle, x509_att is NULL, x509Attestation_getCertificateType shall fail and return X509_CERTIFICATE_TYPE_NONE ]*/
TEST_FUNCTION(x509Attesation_getCertificateType_null_handle)
{
    //arrange

    //act
    X509_CERTIFICATE_TYPE type = x509Attestation_getCertificateType(NULL);

    //assert
    ASSERT_IS_TRUE(type == X509_CERTIFICATE_TYPE_NONE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_002: [ Otherwise, x509Attestation_getCertificateType shall return the certificate type of x509_att ]*/
TEST_FUNCTION(x509Attestation_getCertificateType_client)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_TYPE type = x509Attestation_getCertificateType(x509_att);

    //assert
    ASSERT_IS_TRUE(type == X509_CERTIFICATE_TYPE_CLIENT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_002: [ Otherwise, x509Attestation_getCertificateType shall return the certificate type of x509_att ]*/
TEST_FUNCTION(x509Attestation_getCertificateType_signing)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_TYPE type = x509Attestation_getCertificateType(x509_att);

    //assert
    ASSERT_IS_TRUE(type == X509_CERTIFICATE_TYPE_SIGNING);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_002: [ Otherwise, x509Attestation_getCertificateType shall return the certificate type of x509_att ]*/
TEST_FUNCTION(x509Attestation_getCertificateType_ca)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_TYPE type = x509Attestation_getCertificateType(x509_att);

    //assert
    ASSERT_IS_TRUE(type == X509_CERTIFICATE_TYPE_CA_REFERENCES);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_003: [ If the given handle, x509_att is NULL, x509Attestation_getPrimaryCertificate shall fail and return NULL ]*/
TEST_FUNCTION(x509Attestation_getPrimaryCertificate_null)
{
    //arrange

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(NULL);

    //assert
    ASSERT_IS_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_004: [ If x509_att is an X509 attestation with client certificate(s), x509Attestation_getPrimaryCertificate shall return the primary client certificate as an X509_CERTIFICATE_HANDLE ]*/
TEST_FUNCTION(x509Attestation_getPrimaryCertificate_client)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_005: [ If x509_att is an X509 attestation with signing certificate(s), x509Attestation_getPrimaryCertificate shall return the primary signing certificate as an X509_CERTIFICATE_HANDLE ]*/
TEST_FUNCTION(x509Attestation_getPrimaryCertificate_signing)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_006: [ If x509_att is another type of X509 attestation, x509Attestation_getPrimaryCertificate shall fail and return NULL ]*/
TEST_FUNCTION(x509Attestation_getPrimaryCertificate_ca_ref)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //assert
    ASSERT_IS_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_007: [ If the given handle, x509_att is NULL, x509Attestation_getSecondaryCertificate shall fail and return NULL ]*/
TEST_FUNCTION(x509Attestation_getSecondaryCertificate_null)
{
    //arrange

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getSecondaryCertificate(NULL);

    //assert
    ASSERT_IS_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_008: [ If x509_att is an X509 attestation with client certificate(s), x509Attestation_getSecondaryCertificate shall return the secondary client certificate as an X509_CERTIFICATE_HANDLE ]*/
TEST_FUNCTION(x509Attestation_getSecondaryCertificate_client)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getSecondaryCertificate(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_009: [ If x509_att is an X509 attestation with signing certificate(s), x509Attestation_getSecondaryCertificate shall return the secondary signing certificate as an X509_CERTIFICATE_HANDLE ]*/
TEST_FUNCTION(x509Attestation_getSecondaryCertificate_signing)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getSecondaryCertificate(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Test_X509_ATTESTATION_22_010: [ If x509_att is another type of X509 attestation, x509Attestation_getSecondaryCertificate shall fail and return NULL ]*/
TEST_FUNCTION(x509Attestation_getSecondaryCertificate_ca)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getSecondaryCertificate(x509_att);

    //assert
    ASSERT_IS_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_035: [ If x509_att does not have a secondary certificate, x509Attestation_getSecondaryCertificate shall return NULL ]*/
TEST_FUNCTION(x509Attestation_getSecondaryCertificate_no_secondary_cert)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    //act
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getSecondaryCertificate(x509_att);

    //assert
    ASSERT_IS_NULL(cert);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_011: [ If the given handle, x509_cert is NULL, x509Certificate_getSubjectName shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSubjectName_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getSubjectName(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTAITON_22_012: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getSubjectName shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSubjectName_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getSubjectName(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_013: [ Otherwise, x509Certificate_getSubjectName shall return the subject name from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getSubjectName_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getSubjectName(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SUBJECT_NAME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_014: [ If the given handle, x509_cert is NULL, x509Certificate_getSha1Thumbprint shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSha1Thumbprint_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getSha1Thumbprint(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_015: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getSha1Thumbprint shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSha1Thumbprint_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getSha1Thumbprint(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_016: [ Otherwise, x509Certificate_getSha1Thumbprint shall return the sha1 thumbprint from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getSha1Thumbprint_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getSha1Thumbprint(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SHA1_THUMBPRINT, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_017: [ If the given handle, x509_cert is NULL, x509Certificate_getSha256Thumbprint shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSha256Thumbprint_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getSha256Thumbprint(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_018: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getSha256Thumbprint shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSha256Thumbprint_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getSha256Thumbprint(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_019: [ Otherwise, x509Certificate_getSha256Thumbprint shall return the sha256 thumbprint from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getSha256Thumbprint_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getSha256Thumbprint(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SHA256_THUMBPRINT, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_020: [ If the given handle, x509_cert is NULL, x509Certificate_getIssuerName shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getIssuerName_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getIssuerName(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_021: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getIssuerName shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getIssuerName_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getIssuerName(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_022: [ Otherwise, x509Certificate_getIssuerName shall return the issuer name from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getIssuerName_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getIssuerName(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ISSUER_NAME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_023: [ If the given handle, x509_cert is NULL, x509Certificate_getNotBeforeUtc shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getNotBeforeUtc_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getNotBeforeUtc(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_024: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getNotBeforeUtc shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getNotBeforeUtc_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getNotBeforeUtc(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_025: [ Otherwise, x509Certificate_getNotBeforeUtc shall return the "not before utc" from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getNotBeforeUtc_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getNotBeforeUtc(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_NOT_BEFORE_UTC, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_026: [ If the given handle, x509_cert is NULL, x509Certificate_getNotAfterUtc shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getNotAfterUtc_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getNotAfterUtc(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_027: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getNotAfterUtc shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getNotAfterUtc_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getNotAfterUtc(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_028: [ Otherwise, x509Certificate_getNotAfterUtc shall return the "not after utc" from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getNotAfterUtc_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getNotAfterUtc(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_NOT_AFTER_UTC, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_029: [ If the given handle, x509_cert is NULL, x509Certificate_getSerialNumber shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSerialNumber_null)
{
    //arrange

    //act
    const char* result = x509Certificate_getSerialNumber(NULL);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_030: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getSerialNumber shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getSerialNumber_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    const char* result = x509Certificate_getSerialNumber(cert);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_031: [ Otherwise, x509Certificate_getSerialNumber shall return the serial number from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getSerialNumber_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    const char* result = x509Certificate_getSerialNumber(cert);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_SERIAL_NUMBER, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_032: [ If the given handle, x509_cert is NULL, x509Certificate_getVersion shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getVersion_null)
{
    //arrange

    //act
    int result = x509Certificate_getVersion(NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_X509_ATTESTATION_22_033: [ If the given handle, x509_cert has not yet been processed by the Provisioning Service, x509Certificate_getVersion shall fail and return NULL ]*/
TEST_FUNCTION(x509Certificate_getVersion_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);
    umock_c_reset_all_calls();

    //act
    int result = x509Certificate_getVersion(cert);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

/*Tests_X509_ATTESTATION_22_034: [ Otherwise, x509Certificate_getVersion shall return the version from x509_cert ]*/
TEST_FUNCTION(x509Certificate_getVersion_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    X509_CERTIFICATE_HANDLE cert = x509Attestation_getPrimaryCertificate(x509_att);

    //act
    int result = x509Certificate_getVersion(cert);

    //assert
    ASSERT_ARE_EQUAL(int, DUMMY_VERSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_no_cert_type)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_NONE, DUMMY_CERT1, DUMMY_CERT2);

    //assert
    ASSERT_IS_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_create_no_primary_cert)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, NULL, DUMMY_CERT2);

    //asssert
    ASSERT_IS_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_create_client_only_primary_cert)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_CLIENT);
    ASSERT_IS_NOT_NULL(x509Attestation_getPrimaryCertificate(x509_att));
    ASSERT_IS_NULL(x509Attestation_getSecondaryCertificate(x509_att));

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_client_both_certs)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_CLIENT);
    ASSERT_IS_NOT_NULL(x509Attestation_getPrimaryCertificate(x509_att));
    ASSERT_IS_NOT_NULL(x509Attestation_getSecondaryCertificate(x509_att));

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_client_both_certs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3, 5, 7, 14, 15};
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
        sprintf(tmp_msg, "attestationMechanism_create_client_both_certs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_create_signing_no_primary_cert)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, NULL, DUMMY_CERT2);

    //assert
    ASSERT_IS_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_create_signing_only_primary_cert)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_SIGNING);
    ASSERT_IS_NOT_NULL(x509Attestation_getPrimaryCertificate(x509_att));
    ASSERT_IS_NULL(x509Attestation_getSecondaryCertificate(x509_att));

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_signing_both_certs)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_SIGNING);
    ASSERT_IS_NOT_NULL(x509Attestation_getPrimaryCertificate(x509_att));
    ASSERT_IS_NOT_NULL(x509Attestation_getSecondaryCertificate(x509_att));

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_signing_both_certs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3, 5, 7, 14, 15};
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
        sprintf(tmp_msg, "attestationMechanism_create_signing_both_certs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_create_ca_no_primary_ref)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, NULL, DUMMY_REF2);

    //assert
    ASSERT_IS_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_create_ca_only_primary_ref)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_CA_REFERENCES);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_ca_both_refs)
{
    //arrange
    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_IS_TRUE(x509Attestation_getCertificateType(x509_att) == X509_CERTIFICATE_TYPE_CA_REFERENCES);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_create_ca_both_refs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 3, 5, 7, 12, 13 };
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
        sprintf(tmp_msg, "attestationMechanism_create_ca_both_refs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_CERT1, DUMMY_CERT2);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_destroy_null)
{
    //arrange

    //act
    x509Attestation_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_client_one_cert_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CLIENT, false, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_client_one_cert_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CLIENT, false, true);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_client_two_certs_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CLIENT, true, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_client_two_certs_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, true);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CLIENT, true, true);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_signing_one_cert_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_SIGNING, false, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_signing_one_cert_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_SIGNING, false);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_SIGNING, false, true);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_signing_two_certs_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_SIGNING, true, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_signing_two_certs_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_SIGNING, true);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_SIGNING, true, true);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_ca_one_ref)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CA_REFERENCES, false, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_destroy_ca_two_refs)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_destroy(X509_CERTIFICATE_TYPE_CA_REFERENCES, true, false);

    //act
    x509Attestation_destroy(x509_att);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_fromJson_null)
{
    //arrange

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(NULL);

    //assert
    ASSERT_IS_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_fromJson_client_one_cert)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CLIENT, false);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    X509_CERTIFICATE_HANDLE primary_cert = x509Attestation_getPrimaryCertificate(x509_att);
    X509_CERTIFICATE_HANDLE secondary_cert = x509Attestation_getSecondaryCertificate(x509_att);
    ASSERT_IS_NOT_NULL(primary_cert);
    assert_is_valid_cert(primary_cert);
    ASSERT_IS_NULL(secondary_cert);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_client_two_certs)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CLIENT, true);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    X509_CERTIFICATE_HANDLE primary_cert = x509Attestation_getPrimaryCertificate(x509_att);
    X509_CERTIFICATE_HANDLE secondary_cert = x509Attestation_getSecondaryCertificate(x509_att);
    ASSERT_IS_NOT_NULL(primary_cert);
    assert_is_valid_cert(primary_cert);
    ASSERT_IS_NOT_NULL(secondary_cert);
    assert_is_valid_cert(secondary_cert);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_client_two_certs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CLIENT, true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 6, 9, 11, 13, 15, 17, 19, 21, 23, 24, 26, 29, 31, 33, 35, 37, 39, 41, 43};
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
        sprintf(tmp_msg, "attestationMechanism_fromJson_client_two_certs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_fromJson_signing_one_cert)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_SIGNING, false);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    X509_CERTIFICATE_HANDLE primary_cert = x509Attestation_getPrimaryCertificate(x509_att);
    X509_CERTIFICATE_HANDLE secondary_cert = x509Attestation_getSecondaryCertificate(x509_att);
    ASSERT_IS_NOT_NULL(primary_cert);
    assert_is_valid_cert(primary_cert);
    ASSERT_IS_NULL(secondary_cert);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_signing_two_certs)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_SIGNING, true);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    X509_CERTIFICATE_HANDLE primary_cert = x509Attestation_getPrimaryCertificate(x509_att);
    X509_CERTIFICATE_HANDLE secondary_cert = x509Attestation_getSecondaryCertificate(x509_att);
    ASSERT_IS_NOT_NULL(primary_cert);
    assert_is_valid_cert(primary_cert);
    ASSERT_IS_NOT_NULL(secondary_cert);
    assert_is_valid_cert(secondary_cert);

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_signing_two_certs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_SIGNING, true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 7, 10, 12, 14, 16, 18, 20, 22, 24, 25, 27, 30, 32, 34, 36, 38, 40, 42, 44 };
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
        sprintf(tmp_msg, "attestationMechanism_fromJson_signing_two_certs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_fromJson_ca_one_ref)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, false);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_ca_two_refs)
{
    //arrange
    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, true);

    //act
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

    //assert
    ASSERT_IS_NOT_NULL(x509_att);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_fromJson_ca_two_refs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    expected_calls_x509Attestation_fromJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
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
        sprintf(tmp_msg, "attestationMechanism_fromJson_ca_two_refs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        X509_ATTESTATION_HANDLE x509_att = x509Attestation_fromJson(TEST_JSON_OBJECT);

        //assert
        ASSERT_IS_NULL(x509_att, tmp_msg);
    }

    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(x509Attestation_toJson_null)
{
    //arrange

    //act
    JSON_Value* json_val = x509Attestation_toJson(NULL);

    //assert
    ASSERT_IS_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(x509Attestation_toJson_client_one_cert_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, false, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_client_two_certs_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, true, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_client_two_certs_unprocessed_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CLIENT, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, true, false);
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        //if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
        //    continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "x509Attestation_toJson_client_two_certs_unprocessed_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* json_val = x509Attestation_toJson(x509_att);

        //assert
        ASSERT_IS_NULL(json_val, tmp_msg);
    }

    umock_c_negative_tests_deinit();
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_client_one_cert_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, false);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, false, true);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_client_two_certs_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, true);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, true, true);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_client_two_certs_processed_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_CLIENT, true);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CLIENT, true, true);
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        //if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
        //    continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "x509Attestation_toJson_client_two_certs_processed_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* json_val = x509Attestation_toJson(x509_att);

        //assert
        ASSERT_IS_NULL(json_val, tmp_msg);
    }

    umock_c_negative_tests_deinit();
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_one_cert_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, NULL);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, false, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_two_certs_unprocessed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, true, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_two_certs_unprocessed_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_SIGNING, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, true, false);
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        //if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
        //    continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "x509Attestation_toJson_signing_two_certs_unprocessed_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* json_val = x509Attestation_toJson(x509_att);

        //assert
        ASSERT_IS_NULL(json_val, tmp_msg);
    }

    umock_c_negative_tests_deinit();
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_one_cert_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_SIGNING, false);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, false, true);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_two_certs_processed)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_SIGNING, true);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, true, true);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_signing_two_certs_processed_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    X509_ATTESTATION_HANDLE x509_att = create_dummy_processed_attestation(X509_CERTIFICATE_TYPE_SIGNING, true);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_SIGNING, true, true);
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        //if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
        //    continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "x509Attestation_toJson_signing_two_certs_processed_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* json_val = x509Attestation_toJson(x509_att);

        //assert
        ASSERT_IS_NULL(json_val, tmp_msg);
    }

    umock_c_negative_tests_deinit();
    x509Attestation_destroy(x509_att);
}














TEST_FUNCTION(x509Attestation_toJson_ca_one_ref)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, NULL);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, false, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_ca_two_refs)
{
    //arrange
    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_REF1, DUMMY_REF2);
    umock_c_reset_all_calls();
    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, true, false);

    //act
    JSON_Value* json_val = x509Attestation_toJson(x509_att);

    //assert
    ASSERT_IS_NOT_NULL(json_val);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    x509Attestation_destroy(x509_att);
}

TEST_FUNCTION(x509Attestation_toJson_ca_two_refs_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    X509_ATTESTATION_HANDLE x509_att = x509Attestation_create(X509_CERTIFICATE_TYPE_CA_REFERENCES, DUMMY_CERT1, DUMMY_CERT2);
    umock_c_reset_all_calls();

    expected_calls_x509Attestation_toJson(X509_CERTIFICATE_TYPE_CA_REFERENCES, true, false);
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { 1, 2, 3, 6, 8 };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        //if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
        //    continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "x509Attestation_toJson_ca_two_certs_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        JSON_Value* json_val = x509Attestation_toJson(x509_att);

        //assert
        ASSERT_IS_NULL(json_val, tmp_msg);
    }

    umock_c_negative_tests_deinit();
    x509Attestation_destroy(x509_att);
}

END_TEST_SUITE(prov_sc_x509_attestation_ut);
