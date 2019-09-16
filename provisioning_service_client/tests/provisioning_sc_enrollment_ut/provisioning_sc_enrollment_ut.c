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
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "parson.h"
#include "prov_service_client/provisioning_sc_device_registration_state.h"
#include "prov_service_client/provisioning_sc_twin.h"
#include "prov_service_client/provisioning_sc_attestation_mechanism.h"
#include "prov_service_client/provisioning_sc_device_capabilities.h"

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

#include "prov_service_client/provisioning_sc_enrollment.h"
#include "prov_service_client/provisioning_sc_models_serializer.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

//Control Parameters
static char* DUMMY_JSON = "{\"json\":\"dummy\"}";
static const char* DUMMY_STRING = "dummy";
static const char* DUMMY_REGISTRATION_ID = "REGISTRATION_ID";
static const char* DUMMY_DEVICE_ID = "DEVICE_ID";
static const char* DUMMY_GROUP_ID = "GROUP_ID";
static const char* DUMMY_IOTHUB_HOSTNAME = "IOTHUB_HOSTNAME";
static const char* DUMMY_ETAG = "ETAG";
static const char* DUMMY_CREATED_TIME = "CREATED TIME";
static const char* DUMMY_UPDATED_TIME = "UPDATED_TIME";
static const char* DUMMY_PROVISIONING_STATUS = "enabled";


static DEVICE_CAPABILITIES_HANDLE TEST_DEV_CAP_2 = (DEVICE_CAPABILITIES_HANDLE)0x11111128;
static DEVICE_CAPABILITIES_HANDLE TEST_DEV_CAP = (DEVICE_CAPABILITIES_HANDLE)0x11111118;
static DEVICE_REGISTRATION_STATE_HANDLE TEST_DEV_REG = (DEVICE_REGISTRATION_STATE_HANDLE)0x11111119;
static ATTESTATION_MECHANISM_HANDLE TEST_ATTESTATION_MECHANISM = (ATTESTATION_MECHANISM_HANDLE)0x11111113;
static INITIAL_TWIN_HANDLE TEST_INITIAL_TWIN = (INITIAL_TWIN_HANDLE)0x11111114;

#define TEST_JSON_VALUE (JSON_Value*)0x11111111
#define TEST_JSON_OBJECT (JSON_Object*)0x11111112
#define TEST_REGISTRATION_STATE (DEVICE_REGISTRATION_STATE_HANDLE)0x11111115
#define TEST_ATTESTATION_MECHANISM_2 (ATTESTATION_MECHANISM_HANDLE)0x11111116
#define TEST_INITIAL_TWIN_2 (INITIAL_TWIN_HANDLE)0x11111117

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
    (void)umocktypes_bool_register_types();
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
    REGISTER_GLOBAL_MOCK_RETURN(json_object_set_number, JSONSuccess);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_set_number, JSONFailure);

    //attestation mechanism
    REGISTER_UMOCK_ALIAS_TYPE(ATTESTATION_MECHANISM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const ATTESTATION_MECHANISM_HANDLE, void*);
    REGISTER_GLOBAL_MOCK_RETURN(attestationMechanism_isValidForIndividualEnrollment, true);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(attestationMechanism_isValidForIndividualEnrollment, false);
    REGISTER_GLOBAL_MOCK_RETURN(attestationMechanism_isValidForEnrollmentGroup, true);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(attestationMechanism_isValidForEnrollmentGroup, false);
    REGISTER_GLOBAL_MOCK_RETURN(attestationMechanism_toJson, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(attestationMechanism_toJson, NULL);

    //twin
    REGISTER_UMOCK_ALIAS_TYPE(INITIAL_TWIN_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const INITIAL_TWIN_HANDLE, void*);
    REGISTER_GLOBAL_MOCK_RETURN(initialTwin_toJson, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(initialTwin_toJson, NULL);

    //device registration state
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_REGISTRATION_STATE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_CAPABILITIES_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_RETURN(deviceCapabilities_create, TEST_DEV_CAP);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceCapabilities_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(deviceCapabilities_toJson, TEST_JSON_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceCapabilities_toJson, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(deviceCapabilities_fromJson, TEST_DEV_CAP);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceCapabilities_fromJson, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(deviceRegistrationState_fromJson, TEST_DEV_REG);
    REGISTER_GLOBAL_MOCK_RETURN(attestationMechanism_fromJson, TEST_ATTESTATION_MECHANISM);
    REGISTER_GLOBAL_MOCK_RETURN(initialTwin_fromJson, TEST_INITIAL_TWIN);
}

BEGIN_TEST_SUITE(provisioning_sc_enrollment_ut)

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

static void copy_json_field(const char* return_value)
{
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(return_value); //reg id //cannot fail
    if (return_value != NULL)
    {
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void individualEnrollment_deserializeFromJson_expected_calls(bool use_all_fields)
{
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_DEV_CAP);
    copy_json_field(DUMMY_REGISTRATION_ID);
    copy_json_field(use_all_fields ? DUMMY_DEVICE_ID : NULL);
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(use_all_fields ? TEST_JSON_OBJECT : NULL); //reg state //cannot fail
    if (use_all_fields)
    {
        STRICT_EXPECTED_CALL(deviceRegistrationState_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_REGISTRATION_STATE);
    }
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation mechanism
    STRICT_EXPECTED_CALL(attestationMechanism_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_ATTESTATION_MECHANISM);
    copy_json_field(use_all_fields ? DUMMY_IOTHUB_HOSTNAME : NULL);
    STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(use_all_fields ? TEST_JSON_OBJECT : NULL); //twin //cannot fail
    if (use_all_fields)
    {
        STRICT_EXPECTED_CALL(initialTwin_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_INITIAL_TWIN);
    }
    copy_json_field(use_all_fields ? DUMMY_ETAG : NULL);
    STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_PROVISIONING_STATUS); //provisioning status
    copy_json_field(use_all_fields ? DUMMY_CREATED_TIME : NULL);
    copy_json_field(use_all_fields ? DUMMY_UPDATED_TIME : NULL);
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
}

static void enrollmentGroup_deserializeFromJson_expected_calls(bool use_all_fields)
{
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (use_all_fields)
    {
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_GROUP_ID); //group id
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation mechanism
        STRICT_EXPECTED_CALL(attestationMechanism_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_ATTESTATION_MECHANISM);
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_IOTHUB_HOSTNAME);
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //iothub hostname
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //twin
        STRICT_EXPECTED_CALL(initialTwin_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_INITIAL_TWIN);
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_ETAG); //etag
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_PROVISIONING_STATUS); //provisioning status
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_CREATED_TIME); //created time
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_UPDATED_TIME); //updated time
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    else
    {
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_GROUP_ID); //group id
        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation mechanism
        STRICT_EXPECTED_CALL(attestationMechanism_fromJson(IGNORED_PTR_ARG)).SetReturn(TEST_ATTESTATION_MECHANISM);
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //iothubhostname
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //twin
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //etag
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(DUMMY_PROVISIONING_STATUS); //provisioning status
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //created time
        STRICT_EXPECTED_CALL(json_object_get_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //updated time

    }
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
}

static INDIVIDUAL_ENROLLMENT_HANDLE get_ie_from_json()
{
    individualEnrollment_deserializeFromJson_expected_calls(true);
    INDIVIDUAL_ENROLLMENT_HANDLE result = individualEnrollment_deserializeFromJson(DUMMY_JSON);
    umock_c_reset_all_calls();
    return result;
}

static ENROLLMENT_GROUP_HANDLE get_eg_from_json()
{
    enrollmentGroup_deserializeFromJson_expected_calls(true);
    ENROLLMENT_GROUP_HANDLE result = enrollmentGroup_deserializeFromJson(DUMMY_JSON);
    umock_c_reset_all_calls();
    return result;
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

/*Tests_ENROLLMENT_22_001: [ If reg_id is NULL, individualEnrollment_create shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_create_null_reg_id)
{
    //arrange

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(NULL, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_IS_NULL(ie);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_002: [ If att_mech is NULL, individualEnrollment_create shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_create_null_attestation)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(NULL)).SetReturn(false);

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, NULL);

    //assert
    ASSERT_IS_NULL(ie);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_003: [ If att_mech has an invalid Attestation Type for Individual Enrollment, individualEnrollment_create shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_create_invalid_attestation)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(TEST_ATTESTATION_MECHANISM)).SetReturn(false);

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_IS_NULL(ie);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_005: [ Upon success, individualEnrollment_create shall return a handle for the new individual enrollment ]*/
TEST_FUNCTION(individualEnrollment_create_success)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_create());
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_IS_NOT_NULL(ie);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, individualEnrollment_getRegistrationId(ie));
    ASSERT_IS_TRUE(individualEnrollment_getProvisioningStatus(ie) == PROVISIONING_STATUS_ENABLED);
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM);
    ASSERT_IS_NULL(individualEnrollment_getInitialTwin(ie));
    ASSERT_IS_NULL(individualEnrollment_getDeviceRegistrationState(ie));
    ASSERT_IS_NULL(individualEnrollment_getIotHubHostName(ie));
    ASSERT_IS_NULL(individualEnrollment_getDeviceId(ie));
    ASSERT_IS_NULL(individualEnrollment_getEtag(ie));
    ASSERT_IS_NULL(individualEnrollment_getCreatedDateTime(ie));
    ASSERT_IS_NULL(individualEnrollment_getUpdatedDateTime(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_004: [ If allocating memory for the new individual enrollment fails, individualEnrollment_create shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_create_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_create());
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;

        char tmp_msg[128];
        sprintf(tmp_msg, "individualEnrollment_create_error failure in test %zu/%zu", index, count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);

        //assert
        ASSERT_IS_NULL(ie, tmp_msg);
    }
}

/*Tests_ENROLLMENT_22_006: [ individualEnrollment_destroy shall free all memory contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_destroy_NULL)
{
    //arrange

    //act
    individualEnrollment_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_006: [ individualEnrollment_destroy shall free all memory contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_destroy_min_ie)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceCapabilities_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //registration id
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //device id
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //etag
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //iothubhostname
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //created date time
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //updated date time
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM)); //attestation mechanism
    STRICT_EXPECTED_CALL(initialTwin_destroy(NULL)); //initialTwin
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(NULL)); //deviceRegistrationState
    STRICT_EXPECTED_CALL(gballoc_free(ie)); //individualEnrollment

    //act
    individualEnrollment_destroy(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_006: [ individualEnrollment_destroy shall free all memory contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_destroy_max_ie)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = get_ie_from_json();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceCapabilities_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //registration id
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //device id
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //iothubhostname
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //created date time
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //updated date time
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM)); //attestation mechanism
    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN)); //initialTwin
    STRICT_EXPECTED_CALL(deviceRegistrationState_destroy(TEST_REGISTRATION_STATE)); //deviceRegistrationState
    STRICT_EXPECTED_CALL(gballoc_free(ie)); //individualEnrollment

    //act
    individualEnrollment_destroy(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_007: [ If group_id is NULL, enrollmentGroup_create shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_create_null_group_id)
{
    //arrange

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(NULL, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(eg);

    //cleanup
}

/*Tests_ENROLLMENT_22_008: [ If att_mech is NULL, enrollmentGroup_create shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_create_null_attestation)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(NULL)).SetReturn(false);

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(eg);

    //cleanup
}

/*Tests_ENROLLMENT_22_009: [ If att_mech has an invalid Attestation Type for Enrollment Group, enrollmentGroup_create shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_create_invalid_attestation)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(TEST_ATTESTATION_MECHANISM)).SetReturn(false);

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(eg);

    //cleanup
}

/*Tests_ENROLLMENT_22_011: [ Upon success, enrollmentGroup_create shall return a handle for the new enrollment group ]*/
TEST_FUNCTION(enrollmentGroup_create_success)
{
    //arrange
    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_GROUP_ID));

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(eg);
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM);
    ASSERT_IS_NULL(enrollmentGroup_getInitialTwin(eg));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_GROUP_ID, enrollmentGroup_getGroupId(eg));
    ASSERT_IS_NULL(enrollmentGroup_getEtag(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getProvisioningStatus(eg) == PROVISIONING_STATUS_ENABLED);
    ASSERT_IS_NULL(enrollmentGroup_getCreatedDateTime(eg));
    ASSERT_IS_NULL(enrollmentGroup_getUpdatedDateTime(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_010: [ If allocating memory for the new enrollment group fails, enrollmentGroup_create shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_create_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_GROUP_ID));
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
        sprintf(tmp_msg, "enrollmentGroup_create_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);

        //assert
        ASSERT_IS_NULL(eg, tmp_msg);
    }
}

/*Tests_ENROLLMENT_22_012: [ enrollmentGroup_destroy shall free all memory contained within handle ]*/
TEST_FUNCTION(enrollmentGroup_destroy_null)
{
    //arrange

    //act
    enrollmentGroup_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_012: [ enrollmentGroup_destroy shall free all memory contained within handle ]*/
TEST_FUNCTION(enrollmentGroup_destroy_min_eg)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM)); //attestationMechanism
    STRICT_EXPECTED_CALL(initialTwin_destroy(NULL)); //initialTwin
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //etag
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //iothub hostname
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //created date time
    STRICT_EXPECTED_CALL(gballoc_free(NULL)); //updated date time
    STRICT_EXPECTED_CALL(gballoc_free(eg)); //enrollmentGroup

    //act
    enrollmentGroup_destroy(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_012: [ enrollmentGroup_destroy shall free all memory contained within handle ]*/
TEST_FUNCTION(enrollmentGroup_destroy_max_eg)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = get_eg_from_json();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM)); //attestationMechanism
    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN)); //initialTwin
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //iothub hostname
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //created date time
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //updated date time
    STRICT_EXPECTED_CALL(gballoc_free(eg)); //enrollmentGroup

    //act
    enrollmentGroup_destroy(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

/*Tests_ENROLLMENT_22_013: [ If enrollment is NULL, individualEnrollment_getAttestationMechanism shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getAttestationMechanism_null)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = individualEnrollment_getAttestationMechanism(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(att);

    //cleanup
}

/*Tests_ENROLLMENT_22_014: [ Otherwise, individualEnrollment_getAttestationMechanism shall return the attestation mechanism contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_getAttestationMechanism_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    ATTESTATION_MECHANISM_HANDLE att = individualEnrollment_getAttestationMechanism(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_TRUE(att == TEST_ATTESTATION_MECHANISM);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_015: [ If enrollment is NULL, individualEnrollment_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setAttestationMechanism_null_enrollment)
{
    //arrange

    //act
    int result = individualEnrollment_setAttestationMechanism(NULL, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_016: [ If att_mech is NULL or has an invalid type (e.g. X509 Signing Certificate), individualEnrollment_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setAttestationMechanism_null_att)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(NULL)).SetReturn(false);

    //act
    int result = individualEnrollment_setAttestationMechanism(ie, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_016: [ If att_mech is NULL or has an invalid type (e.g. X509 Signing Certificate), individualEnrollment_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setAttestationMechanism_invalid_att)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(TEST_ATTESTATION_MECHANISM_2)).SetReturn(false);

    //act
    int result = individualEnrollment_setAttestationMechanism(ie, TEST_ATTESTATION_MECHANISM_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_017: [ The new attestation mechanism, att_mech, will be attached to the individual enrollment enrollment, and any existing attestation mechanism will have its memory freed. Then individualEnrollment_setAttestationMechanism shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setAttestationMechanism_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForIndividualEnrollment(TEST_ATTESTATION_MECHANISM_2)).SetReturn(true);
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM));

    //act
    int result = individualEnrollment_setAttestationMechanism(ie, TEST_ATTESTATION_MECHANISM_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM_2);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_018: [ If enrollment is NULL, individualEnrollment_getInitialTwin shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getInitialTwin_null)
{
    //arrange

    //act
    INITIAL_TWIN_HANDLE twin = individualEnrollment_getInitialTwin(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(twin);

    //cleanup
}

/*Tests_ENROLLMENT_22_019: [ Otherwise, individualEnrollment_getInitialTwin shall return the initial twin contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_getInitialTwin_no_twin)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    INITIAL_TWIN_HANDLE twin = individualEnrollment_getInitialTwin(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(twin);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_019: [ Otherwise, individualEnrollment_getInitialTwin shall return the initial twin contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_getInitialTwin_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    //act
    INITIAL_TWIN_HANDLE twin = individualEnrollment_getInitialTwin(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_IS_TRUE(twin == TEST_INITIAL_TWIN);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_020: [ If enrollment is NULL, individualEnrollment_setInitialTwin shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setInitialTwin_null_enrollment)
{
    //arrange

    //act
    int result = individualEnrollment_setInitialTwin(NULL, TEST_INITIAL_TWIN);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_021: [ Upon success, the new initial twin, twin, will be attached to the individual enrollment enrollment, and any existing initial twin will have its memory freed. Then individualEnrollment_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setInitialTwin_success_first_set)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(NULL));

    //act
    int result = individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getInitialTwin(ie) == TEST_INITIAL_TWIN);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_021: [ Upon success, the new initial twin, twin, will be attached to the individual enrollment enrollment, and any existing initial twin will have its memory freed. Then individualEnrollment_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setInitialTwin_success_overwrite)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN));

    //act
    int result = individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getInitialTwin(ie) == TEST_INITIAL_TWIN_2);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_021: [ Upon success, the new initial twin, twin, will be attached to the individual enrollment enrollment, and any existing initial twin will have its memory freed. Then individualEnrollment_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setInitialTwin_success_delete)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN));

    //act
    int result = individualEnrollment_setInitialTwin(ie, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(individualEnrollment_getInitialTwin(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_022: [ If enrollment is NULL, individualEnrollment_getDeviceRegistrationState shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getDeviceRegistrationState_null)
{
    //arrange

    //act
    DEVICE_REGISTRATION_STATE_HANDLE drs = individualEnrollment_getDeviceRegistrationState(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(drs);

    //cleanup
}

/*Tests_ENROLLMENT_22_023: [ Otherwise, individualEnrollment_getDeviceRegistrationState shall return the device registration state contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_getDeviceRegistrationState_success_no_drs)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    DEVICE_REGISTRATION_STATE_HANDLE drs = individualEnrollment_getDeviceRegistrationState(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(drs);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_023: [ Otherwise, individualEnrollment_getDeviceRegistrationState shall return the device registration state contained within enrollment ]*/
TEST_FUNCTION(individualEnrollment_getDeviceRegistrationState_success_return)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = get_ie_from_json();
    umock_c_reset_all_calls();

    //act
    DEVICE_REGISTRATION_STATE_HANDLE drs = individualEnrollment_getDeviceRegistrationState(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(drs);
    ASSERT_IS_TRUE(drs == TEST_REGISTRATION_STATE);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_024: [ If enrollment is NULL, individualEnrollment_getRegistrationId shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getRegistrationId_null)
{
    //arrange

    //act
    const char* reg_id = individualEnrollment_getRegistrationId(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(reg_id);

    //cleanup
}

/*Tests_ENROLLMENT_22_025: [ Otherwise, individualEnrollment_getRegistrationId shall return the registartion id of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getRegistrationId_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* reg_id = individualEnrollment_getRegistrationId(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(reg_id);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, reg_id);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_026: [ If enrollment is NULL, individualEnrollment_getDeviceId shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getDeviceId_null)
{
    //arrange

    //act
    const char* device_id = individualEnrollment_getDeviceId(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(device_id);

    //cleanup
}

/*Tests_ENROLLMENT_22_027: [ Otherwise, individualEnrollment_getDeviceId shall return the device id of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getDeviceId_success_no_value)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* device_id = individualEnrollment_getDeviceId(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(device_id);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_027: [ Otherwise, individualEnrollment_getDeviceId shall return the device id of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getDeviceId_success_return)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);
    umock_c_reset_all_calls();

    //act
    const char* device_id = individualEnrollment_getDeviceId(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(device_id);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, device_id);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_028: [ If enrollment is NULL, individualEnrollment_setDeviceId shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setDeviceId_null_enrollment)
{
    //arrange

    //act
    int result = individualEnrollment_setDeviceId(NULL, DUMMY_DEVICE_ID);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_029: [ Otherwise, device_id will be set as the device id of enrollment and individualEnrollment_setDeviceId shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setDeviceId_first_set)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_DEVICE_ID));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, individualEnrollment_getDeviceId(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_029: [ Otherwise, device_id will be set as the device id of enrollment and individualEnrollment_setDeviceId shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setDeviceId_overwrite)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setDeviceId(ie, DUMMY_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_DEVICE_ID));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, individualEnrollment_getDeviceId(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_029: [ Otherwise, device_id will be set as the device id of enrollment and individualEnrollment_setDeviceId shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setDeviceId_erase)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setDeviceId(ie, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(individualEnrollment_getDeviceId(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_030: [ If enrollment is NULL, individualEnrollment_getIotHubHostName shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getIotHubHostName_null)
{
    //arrange

    //act
    const char* hostname = individualEnrollment_getIotHubHostName(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(hostname);

    //cleanup
}

/*Tests_ENROLLMENT_22_031: [ Otherwise, individualEnrollment_getIotHubHostName shall return the IoT Hub hostname of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getIotHubHostName_success_no_hostname)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* hostname = individualEnrollment_getIotHubHostName(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(hostname);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_031: [ Otherwise, individualEnrollment_getIotHubHostName shall return the IoT Hub hostname of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getIotHubHostName_success_return)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = get_ie_from_json();
    umock_c_reset_all_calls();

    //act
    const char* hostname = individualEnrollment_getIotHubHostName(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(hostname);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_IOTHUB_HOSTNAME, hostname);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_032: [ If enrollment is NULL, individualEnrollment_getEtag shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getEtag_null)
{
    //arrange

    //act
    const char* etag = individualEnrollment_getEtag(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(etag);

    //cleanup
}

/*Tests_ENROLLMENT_22_033: [ Otherwise, individualEnrollment_getEtag shall return the etag of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getEtag_success_no_etag)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* etag = individualEnrollment_getEtag(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(etag);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_033: [ Otherwise, individualEnrollment_getEtag shall return the etag of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getEtag_success_return)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setEtag(ie, DUMMY_ETAG);
    umock_c_reset_all_calls();

    //act
    const char* etag = individualEnrollment_getEtag(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(etag);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, etag);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_034: [ If enrollment is NULL, individualEnrollment_setEtag shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setEtag_null_enrollment)
{
    //arrange

    //act
    int result = individualEnrollment_setEtag(NULL, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_035: [ Otherwise, etag will be set as the etag of enrollment and individualEnrollment_setEtag shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setEtag_first_set)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_ETAG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setEtag(ie, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, individualEnrollment_getEtag(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_035: [ Otherwise, etag will be set as the etag of enrollment and individualEnrollment_setEtag shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setEtag_overwrite)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setEtag(ie, DUMMY_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_ETAG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setEtag(ie, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, individualEnrollment_getEtag(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_035: [ Otherwise, etag will be set as the etag of enrollment and individualEnrollment_setEtag shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setEtag_erase)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setEtag(ie, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = individualEnrollment_setEtag(ie, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(individualEnrollment_getEtag(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_036: [ If enrollment is NULL, individualEnrollment_getProvisioningStatus shall fail and return PROVISIONING_STATUS_NONE ]*/
TEST_FUNCTION(individualEnrollment_getProvisioningStatus_null)
{
    //arrange

    //act
    PROVISIONING_STATUS ps = individualEnrollment_getProvisioningStatus(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(ps == PROVISIONING_STATUS_NONE);

    //cleanup
}

/*Tests_ENROLLMENT_22_037: [ Otherwise, individualEnrollment_getProvisioningStatus shall return the provisioning status of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getProvisioningStatus_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    PROVISIONING_STATUS ps = individualEnrollment_getProvisioningStatus(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(ps == PROVISIONING_STATUS_ENABLED);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_038: [ If enrollment is NULL, individualEnrollment_setProvisioningStatus shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setProvisioningStatus_null_enrollment)
{
    //arrange

    //act
    int result = individualEnrollment_setProvisioningStatus(NULL, PROVISIONING_STATUS_ENABLED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_039: [ If prov_status is invalid type (i.e. PROVISIONING_STATUS_NONE), individualEnrollment_setProvisioningStatus shall fail and return a non-zero number ]*/
TEST_FUNCTION(individualEnrollment_setProvisioningStatus_STATUS_NONE)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    int result = individualEnrollment_setProvisioningStatus(NULL, PROVISIONING_STATUS_NONE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*ENROLLMENT_22_040: [ Otherwise, prov_status will be set as the provisioning status of enrollment and individualEnrollment_setProvisioningStatus shall return 0 ]*/
TEST_FUNCTION(individualEnrollment_setProvisioningStatus_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    int result = individualEnrollment_setProvisioningStatus(ie, PROVISIONING_STATUS_DISABLED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(individualEnrollment_getProvisioningStatus(ie) == PROVISIONING_STATUS_DISABLED);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_041: [ If enrollment is NULL, individualEnrollment_getCreatedDateTime shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getCreatedDateTime_null)
{
    //arrange

    //act
    const char* created = individualEnrollment_getCreatedDateTime(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(created);

    //cleanup
}

/*Tests_ENROLLMENT_22_042: [ Otherwise, individualEnrollment_getCreatedDateTime shall return the created date time of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getCreatedDateTime_success_no_value)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* created = individualEnrollment_getCreatedDateTime(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(created);

    //cleanup
    individualEnrollment_destroy(ie);
}


/*Tests_ENROLLMENT_22_042: [ Otherwise, individualEnrollment_getCreatedDateTime shall return the created date time of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getCreatedDateTime_success_return)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = get_ie_from_json();
    umock_c_reset_all_calls();

    //act
    const char* created = individualEnrollment_getCreatedDateTime(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(created);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_TIME, created);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_043: [ If enrollment is NULL, individualEnrollment_getUpdatedDateTime shall fail and return NULL ]*/
TEST_FUNCTION(individualEnrollment_getUpdatedDateTime_null)
{
    //arrange

    //act
    const char* updated = individualEnrollment_getUpdatedDateTime(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(updated);

    //cleanup
}

/*Tests_ENROLLMENT_22_044: [ Otherwise, individualEnrollment_getUpdatedDateTime shall return the updated date time of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getUpdatedDateTime_success_no_value)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* updated = individualEnrollment_getUpdatedDateTime(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(updated);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_044: [ Otherwise, individualEnrollment_getUpdatedDateTime shall return the updated date time of enrollment ]*/
TEST_FUNCTION(individualEnrollment_getUpdatedDateTime_success_return_value)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = get_ie_from_json();
    umock_c_reset_all_calls();

    //act
    const char* updated = individualEnrollment_getUpdatedDateTime(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(updated);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_TIME, updated);

    //cleanup
    individualEnrollment_destroy(ie);
}

/*Tests_ENROLLMENT_22_045: [ If enrollment is NULL, enrollmentGroup_getAttestationMechanism shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getAttestationMechanism_null)
{
    //arrange

    //act
    ATTESTATION_MECHANISM_HANDLE att = enrollmentGroup_getAttestationMechanism(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(att);

    //cleanup
}

/*Tests_ENROLLMENT_22_046: [ Otherwise, enrollmentGroup_getAttestationMechanism shall return the attestation mechanism contained within enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getAttestationMechanism_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    ATTESTATION_MECHANISM_HANDLE att = enrollmentGroup_getAttestationMechanism(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(att);
    ASSERT_IS_TRUE(att == TEST_ATTESTATION_MECHANISM);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_047: [ If enrollment is NULL, enrollmentGroup_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setAttestationMechanism_null_enrollment)
{
    //arrange

    //act
    int result = enrollmentGroup_setAttestationMechanism(NULL, TEST_ATTESTATION_MECHANISM);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_048: [ If att_mech is NULL or has an invalid type (e.g. X509 Signing Certificate), enrollmentGroup_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setAttestationMechanism_null_att)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(NULL)).SetReturn(false);

    //act
    int result = enrollmentGroup_setAttestationMechanism(eg, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_048: [ If att_mech is NULL or has an invalid type (e.g. X509 Signing Certificate), enrollmentGroup_setAttestationMechanism shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setAttestationMechanism_invalid_att)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(TEST_ATTESTATION_MECHANISM_2)).SetReturn(false);

    //act
    int result = enrollmentGroup_setAttestationMechanism(eg, TEST_ATTESTATION_MECHANISM_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_049: [ The new attestation mechanism, att_mech, will be attached to the individual enrollment enrollment, and any existing attestation mechanism will have its memory freed. Then enrollmentGroup_setAttestationMechanism shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setAttestationMechanism_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(attestationMechanism_isValidForEnrollmentGroup(TEST_ATTESTATION_MECHANISM_2)).SetReturn(true);
    STRICT_EXPECTED_CALL(attestationMechanism_destroy(TEST_ATTESTATION_MECHANISM));

    //act
    int result = enrollmentGroup_setAttestationMechanism(eg, TEST_ATTESTATION_MECHANISM_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM_2);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_050: [ If enrollment is NULL, enrollmentGroup_getInitialTwin shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getInitialTwin_null)
{
    //arrange

    //act
    INITIAL_TWIN_HANDLE twin = enrollmentGroup_getInitialTwin(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(twin);

    //cleanup
}

/*Tests_ENROLLMENT_22_051: [ Otherwise, enrollmentGroup_getInitialTwin shall return the initial twin contained within enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getInitialTwin_success_no_twin)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    INITIAL_TWIN_HANDLE twin = enrollmentGroup_getInitialTwin(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(twin);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_051: [ Otherwise, enrollmentGroup_getInitialTwin shall return the initial twin contained within enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getInitialTwin_success_has_twin)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    //act
    INITIAL_TWIN_HANDLE twin = enrollmentGroup_getInitialTwin(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(twin);
    ASSERT_IS_TRUE(twin == TEST_INITIAL_TWIN);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_052: [ If enrollment is NULL, enrollmentGroup_setInitialTwin shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setInitialTwin_null_enrollment)
{
    //arrange

    //act
    int result = enrollmentGroup_setInitialTwin(NULL, TEST_INITIAL_TWIN);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_053: [ Upon success, the new initial twin, twin, will be attached to the enrollment group enrollment, and any existing initial twin will have its memory freed. Then enrollmentGroup_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setInitialTwin_success_first_set)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(NULL));

    //act
    int result = enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getInitialTwin(eg) == TEST_INITIAL_TWIN);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_053: [ Upon success, the new initial twin, twin, will be attached to the enrollment group enrollment, and any existing initial twin will have its memory freed. Then enrollmentGroup_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setInitialTwin_success_overwrite)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN));

    //act
    int result = enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN_2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getInitialTwin(eg) == TEST_INITIAL_TWIN_2);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_053: [ Upon success, the new initial twin, twin, will be attached to the enrollment group enrollment, and any existing initial twin will have its memory freed. Then enrollmentGroup_setInitialTwin shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setInitialTwin_success_erase)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(initialTwin_destroy(TEST_INITIAL_TWIN));

    //act
    int result = enrollmentGroup_setInitialTwin(eg, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getInitialTwin(eg) == NULL);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_054: [ If enrollment is NULL, enrollmentGroup_getGroupId shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getGroupId_null)
{
    //arrange

    //act
    const char* group_id = enrollmentGroup_getGroupId(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(group_id);

    //cleanup
}

/*Tests_ENROLLMENT_22_055: [ Otherwise, enrollmentGroup_getGroupId shall return the group id of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getGroupId_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* group_id = enrollmentGroup_getGroupId(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(group_id);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_GROUP_ID, group_id);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_056: [ If enrollment is NULL, enrollmentGroup_getIotHubHostName shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getIotHubHostName_null)
{
    //arrange

    //act
    const char* hostname = enrollmentGroup_getIotHubHostName(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(hostname);

    //cleanup
}

/*Tests_ENROLLMENT_22_057: [ Otherwise, enrollmentGroup_getIotHubHostName shall return the IoT Hub hostname of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getIotHubHostName_success_no_hostname)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* hostname = enrollmentGroup_getIotHubHostName(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(hostname);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_057: [ Otherwise, enrollmentGroup_getIotHubHostName shall return the IoT Hub hostname of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getIotHubHostName_success_return_hostname)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = get_eg_from_json();
    umock_c_reset_all_calls();

    //act
    const char* hostname = enrollmentGroup_getIotHubHostName(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(hostname);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_IOTHUB_HOSTNAME, hostname);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_058: [ If enrollment is NULL, enrollmentGroup_getEtag shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getEtag_null)
{
    //arrange

    //act
    const char* etag = enrollmentGroup_getEtag(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(etag);

    //cleanup
}

/*Tests_ENROLLMENT_22_059: [ Otherwise, enrollmentGroup_getEtag shall return the etag of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getEtag_success_no_etag)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* etag = enrollmentGroup_getEtag(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(etag);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_059: [ Otherwise, enrollmentGroup_getEtag shall return the etag of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getEtag_success_return_etag)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setEtag(eg, DUMMY_ETAG);
    umock_c_reset_all_calls();

    //act
    const char* etag = enrollmentGroup_getEtag(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(etag);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, etag);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_060: [ If enrollment is NULL, enrollmentGroup_setEtag shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setEtag_null_enrollment)
{
    //arrange

    //act
    int result = enrollmentGroup_setEtag(NULL, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_061: [ Otherwise, etag will be set as the etag of enrollment and enrollmentGroup_setEtag shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setEtag_success_first_set)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_ETAG));
    STRICT_EXPECTED_CALL(gballoc_free(NULL));

    //act
    int result = enrollmentGroup_setEtag(eg, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, enrollmentGroup_getEtag(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_061: [ Otherwise, etag will be set as the etag of enrollment and enrollmentGroup_setEtag shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setEtag_success_overwrite)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setEtag(eg, DUMMY_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, DUMMY_ETAG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = enrollmentGroup_setEtag(eg, DUMMY_ETAG);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, enrollmentGroup_getEtag(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_061: [ Otherwise, etag will be set as the etag of enrollment and enrollmentGroup_setEtag shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setEtag_success_erase)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setEtag(eg, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = enrollmentGroup_setEtag(eg, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(enrollmentGroup_getEtag(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_062: [ If enrollment is NULL, enrollmentGroup_getProvisioningStatus shall fail and return PROVISIONING_STATUS_NONE ]*/
TEST_FUNCTION(enrollmentGroup_getProvisioningStatus_null)
{
    //arrange

    //act
    PROVISIONING_STATUS ps = enrollmentGroup_getProvisioningStatus(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(ps == PROVISIONING_STATUS_NONE);

    //cleanup
}

/*Tests_ENROLLMENT_22_063: [ Otherwise, enrollmentGroup_getProvisioningStatus shall return the provisioning status of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getProvisioningStatus_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    PROVISIONING_STATUS ps = enrollmentGroup_getProvisioningStatus(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(ps == PROVISIONING_STATUS_ENABLED);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_064: [ If enrollment is NULL, enrollmentGroup_setProvisioningStatus shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setProvisioningStatus_null_enrollment)
{
    //arrange

    //act
    int result = enrollmentGroup_setProvisioningStatus(NULL, PROVISIONING_STATUS_ENABLED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

/*Tests_ENROLLMENT_22_065: [ If prov_status is invalid type (i.e. PROVISIONING_STATUS_NONE), enrollmentGroup_setProvisioningStatus shall fail and return a non-zero number ]*/
TEST_FUNCTION(enrollmentGroup_setProvisioningStatus_PROVISIONING_STATUS_NONE)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    int result = enrollmentGroup_setProvisioningStatus(eg, PROVISIONING_STATUS_NONE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getProvisioningStatus(eg) == PROVISIONING_STATUS_ENABLED);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_066: [ Otherwise, prov_status will be set as the provisioning status of enrollment and enrollmentGroup_setProvisioningStatus shall return 0 ]*/
TEST_FUNCTION(enrollmentGroup_setProvisioningStatus_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    int result = enrollmentGroup_setProvisioningStatus(eg, PROVISIONING_STATUS_DISABLED);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(enrollmentGroup_getProvisioningStatus(eg) == PROVISIONING_STATUS_DISABLED);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_067: [ If enrollment is NULL, enrollmentGroup_getCreatedDateTime shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getCreatedDateTime_null)
{
    //arrange

    //act
    const char* created = enrollmentGroup_getCreatedDateTime(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(created);

    //cleanup
}

/*Tests_ENROLLMENT_22_068: [ Otherwise, enrollmentGroup_getCreatedDateTime shall return the created date time of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getCreatedDateTime_success_no_value)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* created = enrollmentGroup_getCreatedDateTime(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(created);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_068: [ Otherwise, enrollmentGroup_getCreatedDateTime shall return the created date time of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getCreatedDateTime_success_return_value)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = get_eg_from_json();
    umock_c_reset_all_calls();

    //act
    const char* created = enrollmentGroup_getCreatedDateTime(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(created);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_TIME, created);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_069: [ If enrollment is NULL, enrollmentGroup_getUpdatedDateTime shall fail and return NULL ]*/
TEST_FUNCTION(enrollmentGroup_getUpdatedDateTime_null)
{
    //arrange

    //act
    const char* updated = enrollmentGroup_getUpdatedDateTime(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(updated);

    //cleanup
}

/*Tests_ENROLLMENT_22_070: [ Otherwise, enrollmentGroup_getUpdatedDateTime shall return the updated date time of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getUpdatedDateTime_success_no_value)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    const char* updated = enrollmentGroup_getUpdatedDateTime(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(updated);

    //cleanup
    enrollmentGroup_destroy(eg);
}

/*Tests_ENROLLMENT_22_070: [ Otherwise, enrollmentGroup_getUpdatedDateTime shall return the updated date time of enrollment ]*/
TEST_FUNCTION(enrollmentGroup_getUpdatedDateTime_success_return_value)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = get_eg_from_json();
    umock_c_reset_all_calls();

    //act
    const char* updated = enrollmentGroup_getUpdatedDateTime(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(updated);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_TIME, updated);

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(individualEnrollment_serializeToJson_null)
{
    //arrange

    //act
    char* json = individualEnrollment_serializeToJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(individualEnrollment_serializeToJson_min_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //reg id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //att mech
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = individualEnrollment_serializeToJson(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_JSON, json);

    //cleanup
    individualEnrollment_destroy(ie);
    free(json);
}

TEST_FUNCTION(individualEnrollment_serializeToJson_min_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //reg id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //att mech
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 10, 11 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;

        char tmp_msg[128];
        sprintf(tmp_msg, "individualEnrollment_serializeToJson_min_error failure in test %zu/%zu", index, count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = individualEnrollment_serializeToJson(ie);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_serializeToJson_max_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);
    individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);
    individualEnrollment_setEtag(ie, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //dev caps
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //reg id
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //device id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //att mech
    STRICT_EXPECTED_CALL(initialTwin_toJson(TEST_INITIAL_TWIN));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //twin
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = individualEnrollment_serializeToJson(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_JSON, json);

    //cleanup
    individualEnrollment_destroy(ie);
    free(json);
}

TEST_FUNCTION(individualEnrollment_deserializeToJson_max_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    individualEnrollment_setDeviceId(ie, DUMMY_DEVICE_ID);
    individualEnrollment_setInitialTwin(ie, TEST_INITIAL_TWIN);
    individualEnrollment_setEtag(ie, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceCapabilities_toJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //reg id
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //reg id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //att mech
    STRICT_EXPECTED_CALL(initialTwin_toJson(TEST_INITIAL_TWIN));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //twin
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 14, 15 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;

        char tmp_msg[128];
        sprintf(tmp_msg, "individualEnrollment_serializeToJson_max_error failure in test %zu/%zu", index, count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = individualEnrollment_serializeToJson(ie);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_deserializeFromJson_null)
{
    //arrange

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_deserializeFromJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(ie);

    //cleanup
}

TEST_FUNCTION(individualEnrollment_deserializeFromJson_min_success)
{
    //arrange
    individualEnrollment_deserializeFromJson_expected_calls(false);

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, individualEnrollment_getRegistrationId(ie));
    ASSERT_IS_NULL(individualEnrollment_getDeviceId(ie));
    ASSERT_IS_NULL(individualEnrollment_getDeviceRegistrationState(ie));
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM);
    ASSERT_IS_NULL(individualEnrollment_getIotHubHostName(ie));
    ASSERT_IS_NULL(individualEnrollment_getInitialTwin(ie));
    ASSERT_IS_NULL(individualEnrollment_getEtag(ie));
    ASSERT_IS_TRUE(individualEnrollment_getProvisioningStatus(ie) == PROVISIONING_STATUS_ENABLED);
    ASSERT_IS_NULL(individualEnrollment_getCreatedDateTime(ie));
    ASSERT_IS_NULL(individualEnrollment_getUpdatedDateTime(ie));


    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_deserializeFromJson_min_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();

    individualEnrollment_deserializeFromJson_expected_calls(false);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 7, 8, 11, 12, 13, 14, 15, 16, 17 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;

        char tmp_msg[128];
        sprintf(tmp_msg, "individualEnrollment_deserializeFromJson_min_error failure in test %zu/%zu", index, count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(ie, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(individualEnrollment_deserializeFromJson_max_success)
{
    //arrange
    individualEnrollment_deserializeFromJson_expected_calls(true);

    //act
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_REGISTRATION_ID, individualEnrollment_getRegistrationId(ie));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_DEVICE_ID, individualEnrollment_getDeviceId(ie));
    ASSERT_IS_TRUE(individualEnrollment_getDeviceRegistrationState(ie) == TEST_REGISTRATION_STATE);
    ASSERT_IS_TRUE(individualEnrollment_getAttestationMechanism(ie) == TEST_ATTESTATION_MECHANISM);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_IOTHUB_HOSTNAME, individualEnrollment_getIotHubHostName(ie));
    ASSERT_IS_TRUE(individualEnrollment_getInitialTwin(ie) == TEST_INITIAL_TWIN);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, individualEnrollment_getEtag(ie));
    ASSERT_IS_TRUE(individualEnrollment_getProvisioningStatus(ie) == PROVISIONING_STATUS_ENABLED);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_TIME, individualEnrollment_getCreatedDateTime(ie));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_TIME, individualEnrollment_getUpdatedDateTime(ie));

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_deserializeFromJson_max_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();

    individualEnrollment_deserializeFromJson_expected_calls(true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 5, 7, 11, 13, 15, 18, 20, 22, 24 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;

        char tmp_msg[128];
        sprintf(tmp_msg, "individualEnrollment_deserializeFromJson_max_error failure in test %zu/%zu", index, count);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(ie, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(individualEnrollment_setDeviceCapabilities_ie_handle_NULL_fail)
{
    //arrange


    //act
    int result = individualEnrollment_setDeviceCapabilities(NULL, TEST_DEV_CAP);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //cleanup
}

TEST_FUNCTION(individualEnrollment_setDeviceCapabilities_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceCapabilities_destroy(IGNORED_PTR_ARG)); //group id

    //act
    int result = individualEnrollment_setDeviceCapabilities(ie, TEST_DEV_CAP);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_getDeviceCapabilities_handle_NULL_fail)
{
    //arrange

    //act
    DEVICE_CAPABILITIES_HANDLE dev_caps = individualEnrollment_getDeviceCapabilities(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(dev_caps);

    //cleanup
}

TEST_FUNCTION(individualEnrollment_getDeviceCapabilities_with_set_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    (void)individualEnrollment_setDeviceCapabilities(ie, TEST_DEV_CAP_2);
    umock_c_reset_all_calls();

    //act
    DEVICE_CAPABILITIES_HANDLE dev_caps = individualEnrollment_getDeviceCapabilities(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dev_caps);
    ASSERT_ARE_EQUAL(void_ptr, (void*)TEST_DEV_CAP_2, (void*)dev_caps);

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(individualEnrollment_getDeviceCapabilities_success)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(DUMMY_REGISTRATION_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    //act
    DEVICE_CAPABILITIES_HANDLE dev_caps = individualEnrollment_getDeviceCapabilities(ie);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(dev_caps);

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(enrollmentGroup_serializeToJson_null)
{
    //arrange

    //act
    char* json = enrollmentGroup_serializeToJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(json);

    //cleanup
}

TEST_FUNCTION(enrollmentGroup_serializeToJson_min_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = enrollmentGroup_serializeToJson(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(json);

    //cleanup
    enrollmentGroup_destroy(eg);
    free(json);
}

TEST_FUNCTION(enrollmentGroup_serializeToJson_min_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 8, 9 };
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
        sprintf(tmp_msg, "enrollmentGroup_serializeToJson_min_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = enrollmentGroup_serializeToJson(eg);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(enrollmentGroup_serializeToJson_max_success)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);
    enrollmentGroup_setEtag(eg, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation
    STRICT_EXPECTED_CALL(initialTwin_toJson(TEST_INITIAL_TWIN));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //twin
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));

    //act
    char* json = enrollmentGroup_serializeToJson(eg);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(json);

    //cleanup
    enrollmentGroup_destroy(eg);
    free(json);
}

TEST_FUNCTION(enrollmentGroup_serializeToJson_max_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(DUMMY_GROUP_ID, TEST_ATTESTATION_MECHANISM);
    enrollmentGroup_setInitialTwin(eg, TEST_INITIAL_TWIN);
    enrollmentGroup_setEtag(eg, DUMMY_ETAG);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(json_value_init_object());
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //group id
    STRICT_EXPECTED_CALL(attestationMechanism_toJson(TEST_ATTESTATION_MECHANISM));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //attestation
    STRICT_EXPECTED_CALL(initialTwin_toJson(TEST_INITIAL_TWIN));
    STRICT_EXPECTED_CALL(json_object_set_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //twin
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //etag
    STRICT_EXPECTED_CALL(json_object_set_string(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //provisioning status
    STRICT_EXPECTED_CALL(json_serialize_to_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG)); //cannot fail
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 11, 12 };
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
        sprintf(tmp_msg, "enrollmentGroup_serializeToJson_max_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        char* json = enrollmentGroup_serializeToJson(eg);

        //assert
        ASSERT_IS_NULL(json, tmp_msg);
    }

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(enrollmentGroup_deserializeFromJson_null)
{
    //arrange

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_deserializeFromJson(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(eg);

    //cleanup
}

TEST_FUNCTION(enrollmentGroup_deserializeFromJson_min_success)
{
    //arrange
    enrollmentGroup_deserializeFromJson_expected_calls(false);

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(eg);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_GROUP_ID, enrollmentGroup_getGroupId(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM);
    ASSERT_IS_NULL(enrollmentGroup_getIotHubHostName(eg));
    ASSERT_IS_NULL(enrollmentGroup_getInitialTwin(eg));
    ASSERT_IS_NULL(enrollmentGroup_getEtag(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getProvisioningStatus(eg) == PROVISIONING_STATUS_ENABLED);
    ASSERT_IS_NULL(enrollmentGroup_getCreatedDateTime(eg));
    ASSERT_IS_NULL(enrollmentGroup_getUpdatedDateTime(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(enrollmentGroup_deserializeFromJson_min_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();

    enrollmentGroup_deserializeFromJson_expected_calls(false);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 7, 8, 9, 11, 12, 13 };
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
        sprintf(tmp_msg, "enrollmentGroup_deserializeFromJson_min_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(eg, tmp_msg);
    }

    //cleanup
}

TEST_FUNCTION(enrollmentGroup_deserializeFromJson_max_success)
{
    //arrange
    enrollmentGroup_deserializeFromJson_expected_calls(true);

    //act
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_deserializeFromJson(DUMMY_JSON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(eg);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_GROUP_ID, enrollmentGroup_getGroupId(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getAttestationMechanism(eg) == TEST_ATTESTATION_MECHANISM);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_IOTHUB_HOSTNAME, enrollmentGroup_getIotHubHostName(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getInitialTwin(eg) == TEST_INITIAL_TWIN);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_ETAG, enrollmentGroup_getEtag(eg));
    ASSERT_IS_TRUE(enrollmentGroup_getProvisioningStatus(eg) == PROVISIONING_STATUS_ENABLED);
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_CREATED_TIME, enrollmentGroup_getCreatedDateTime(eg));
    ASSERT_ARE_EQUAL(char_ptr, DUMMY_UPDATED_TIME, enrollmentGroup_getUpdatedDateTime(eg));

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(enrollmentGroup_deserializeFromJson_max_error)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    umock_c_reset_all_calls();

    enrollmentGroup_deserializeFromJson_expected_calls(true);
    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 7, 8, 9, 11, 13, 14, 16, 18 };
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
        sprintf(tmp_msg, "enrollmentGroup_deserializeFromJson_max_error failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_deserializeFromJson(DUMMY_JSON);

        //assert
        ASSERT_IS_NULL(eg, tmp_msg);
    }

    //cleanup
}

END_TEST_SUITE(provisioning_sc_enrollment_ut);
