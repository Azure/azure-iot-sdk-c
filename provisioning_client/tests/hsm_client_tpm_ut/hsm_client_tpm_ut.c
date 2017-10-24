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
#include "umocktypes_bool.h"
#include "umock_c_negative_tests.h"
#include "azure_c_shared_utility/macro_utils.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/urlencode.h"

#include "azure_utpm_c/tpm_codec.h"
#include "azure_utpm_c/Marshal_fp.h"
#undef ENABLE_MOCKS

#include "hsm_client_tpm.h"
#include "hsm_client_tpm_abstract.h"

#define ENABLE_MOCKS
#include "azure_utpm_c/TpmTypes.h"
#undef ENABLE_MOCKS

static const char* TEST_STRING_VALUE = "Test_String_Value";
static const unsigned char TEST_IMPORT_KEY[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10 };

static const char* TEST_RSA_KEY = "1234567890";
static unsigned char TEST_BUFFER[128];

static uint16_t g_rsa_size;

#define TEST_BUFFER_SIZE    128
#define TEST_KEY_SIZE       10

static void my_STRING_delete(STRING_HANDLE h)
{
    my_gballoc_free((void*)h);
}

static TPM_RC my_TSS_CreatePrimary(TSS_DEVICE *tpm, TSS_SESSION *sess, TPM_HANDLE hierarchy, TPM2B_PUBLIC *inPub, TPM_HANDLE *outHandle, TPM2B_PUBLIC *outPub)
{
    (void)tpm;
    (void)sess;
    (void)hierarchy;
    (void)inPub;
    (void)outHandle;

    (*outPub).publicArea.unique.rsa.t.size = g_rsa_size;
    return TPM_RC_SUCCESS;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

/*static BUFFER_HANDLE my_Base64_Decoder(const char* source)
{
    (void)source;
    return (BUFFER_HANDLE)my_gballoc_malloc(1);
}*/

STRING_HANDLE my_Base64_Encode_Bytes(const unsigned char* source, size_t size)
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

BEGIN_TEST_SUITE(hsm_client_tpm_ut)

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
        result = umocktypes_bool_register_types();
        ASSERT_ARE_EQUAL(int, 0, result);

        REGISTER_UMOCK_ALIAS_TYPE(XDA_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPM_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(UINT, unsigned int);
        REGISTER_UMOCK_ALIAS_TYPE(UINT32, unsigned int);
        REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
        REGISTER_UMOCK_ALIAS_TYPE(TPM_PT, unsigned int);

        REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SECURE_DEVICE_TYPE, int);
        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(OBJECT_ATTR, int);
        REGISTER_UMOCK_ALIAS_TYPE(TPM_SE, int);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_DH_OBJECT, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_ALG_HASH, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPMA_SESSION, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_DH_ENTITY, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_DH_CONTEXT, void*);
        REGISTER_UMOCK_ALIAS_TYPE(INT32, int);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_RH_PROVISION, void*);
        REGISTER_UMOCK_ALIAS_TYPE(TPMI_DH_PERSISTENT, void*);

        REGISTER_GLOBAL_MOCK_RETURN(TSS_CreatePwAuthSession, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TSS_CreatePwAuthSession, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(Initialize_TPM_Codec, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Initialize_TPM_Codec, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_ReadPublic, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_ReadPublic, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_HOOK(TSS_CreatePrimary, my_TSS_CreatePrimary);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TSS_CreatePrimary, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TSS_Create, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TSS_Create, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TSS_GetTpmProperty, 1028);

        REGISTER_GLOBAL_MOCK_RETURN(TSS_StartAuthSession, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TSS_StartAuthSession, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TSS_PolicySecret, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TSS_PolicySecret, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_ActivateCredential, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_ActivateCredential, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_Import, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_Import, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_Load, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_Load, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_EvictControl, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_EvictControl, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_FlushContext, TPM_RC_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_FlushContext, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(TPM2_ReadPublic, TPM_RC_HANDLE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(TPM2_ReadPublic, TPM_RC_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(SignData, TEST_BUFFER_SIZE);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(SignData, 0);

        //REGISTER_GLOBAL_MOCK_HOOK(Base64_Decoder, my_Base64_Decoder);
        //REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Decoder, NULL);
        //REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, my_BUFFER_create);
        //REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);

        //REGISTER_GLOBAL_MOCK_RETURN(BUFFER_u_char, TEST_BUFFER);
        //REGISTER_GLOBAL_MOCK_RETURN(BUFFER_length, TEST_BUFFER_SIZE);

        REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING_VALUE);
        REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

        REGISTER_GLOBAL_MOCK_HOOK(Base64_Encode_Bytes, my_Base64_Encode_Bytes);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Base64_Encode_Bytes, NULL);
        
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);

        for (size_t index = 0; index < 10; index++)
        {
            TEST_BUFFER[index] = (unsigned char)(index+1);
        }
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
        g_rsa_size = TEST_KEY_SIZE;
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

    static void setup_hsm_client_tpm_create_mock()
    {
        OBJECT_ATTR tmp = FixedTPM;
        TPM_HANDLE tmp_tpm_handle = 0;
        TPMI_RH_PROVISION tpm_rh_prov = 0;
        TPMI_DH_OBJECT tpm_obj = 0;
        TPMI_DH_PERSISTENT tpm_persistent = 0;
        TPMI_DH_CONTEXT tmp_ctx = 0;
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(TSS_CreatePwAuthSession(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Initialize_TPM_Codec(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(ToTpmaObject(tmp))
            .IgnoreArgument_attrs();
        STRICT_EXPECTED_CALL(TPM2_ReadPublic(IGNORED_PTR_ARG, tpm_obj, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_objectHandle(); // 4
        STRICT_EXPECTED_CALL(TSS_CreatePrimary(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_tpm_handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_hierarchy();
        STRICT_EXPECTED_CALL(TPM2_EvictControl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tpm_rh_prov, tpm_obj, tpm_persistent))
            .IgnoreArgument_auth()
            .IgnoreArgument_objectHandle()
            .IgnoreArgument_persistentHandle();
        STRICT_EXPECTED_CALL(TPM2_FlushContext(IGNORED_PTR_ARG, tmp_ctx))
            .IgnoreArgument_flushHandle();
        EXPECTED_CALL(ToTpmaObject(tmp))
            .IgnoreArgument_attrs(); // 8
        STRICT_EXPECTED_CALL(TPM2_ReadPublic(IGNORED_PTR_ARG, tpm_obj, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_objectHandle();
        STRICT_EXPECTED_CALL(TSS_CreatePrimary(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_tpm_handle, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_hierarchy();
        STRICT_EXPECTED_CALL(TPM2_EvictControl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tpm_rh_prov, tpm_obj, tpm_persistent))
            .IgnoreArgument_auth()
            .IgnoreArgument_objectHandle()
            .IgnoreArgument_persistentHandle();
        STRICT_EXPECTED_CALL(TPM2_FlushContext(IGNORED_PTR_ARG, tmp_ctx))
            .IgnoreArgument_flushHandle();
    }

    static void setup_hsm_client_tpm_import_key_mock()
    {
        OBJECT_ATTR tmp = FixedTPM;
        TPM_SE tmp_se = 0;
        TPMA_SESSION tmp_session = { 0 };
        TPMI_DH_OBJECT tmp_dh = { 0 };
        TPMI_RH_PROVISION tmp_prov = { 0 };
        TPMI_DH_CONTEXT tmp_dh_ctx = { 0 };
        TPMI_DH_PERSISTENT tmp_dh_per = { 0 };

        STRICT_EXPECTED_CALL(TSS_StartAuthSession(IGNORED_PTR_ARG, tmp_se, IGNORED_NUM_ARG, tmp_session, IGNORED_PTR_ARG))
            .IgnoreArgument_sessAttrs()
            .IgnoreArgument_sessionType();
        STRICT_EXPECTED_CALL(TSS_PolicySecret(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 4

        STRICT_EXPECTED_CALL(TPM2B_ID_OBJECT_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));     // 2
        STRICT_EXPECTED_CALL(TPM2B_ENCRYPTED_SECRET_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_PRIVATE_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_ENCRYPTED_SECRET_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_PUBLIC_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, TRUE));
        STRICT_EXPECTED_CALL(UINT16_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));       // 7

        STRICT_EXPECTED_CALL(TPM2_ActivateCredential(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2_Import(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_dh, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_parentHandle();

        STRICT_EXPECTED_CALL(ToTpmaObject(tmp)).IgnoreArgument_attrs(); // 10
        STRICT_EXPECTED_CALL(TSS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_dh, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(TPM2_Load(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_dh, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument_parentHandle();     // 12
        STRICT_EXPECTED_CALL(TPM2_EvictControl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_prov, tmp_dh, tmp_dh_per))
            .IgnoreArgument_auth()
            .IgnoreArgument_objectHandle()
            .IgnoreArgument_persistentHandle();
        STRICT_EXPECTED_CALL(TPM2_EvictControl(IGNORED_PTR_ARG, IGNORED_PTR_ARG, tmp_prov, tmp_dh, tmp_dh_per))
            .IgnoreArgument_auth()
            .IgnoreArgument_objectHandle()
            .IgnoreArgument_persistentHandle();
        STRICT_EXPECTED_CALL(TPM2_FlushContext(IGNORED_PTR_ARG, tmp_dh_ctx)) // 15
            .IgnoreArgument_flushHandle();
    }

    static void setup_hsm_client_tpm_get_storage_key_mocks()
    {
        STRICT_EXPECTED_CALL(TPM2B_PUBLIC_Marshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    }

    static void setup_hsm_client_tpm_sign_data_mocks()
    {
        STRICT_EXPECTED_CALL(SignData(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    }

    static void setup_hsm_client_tpm_get_endorsement_key_mocks()
    {
        STRICT_EXPECTED_CALL(TPM2B_PUBLIC_Marshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, NULL));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_002: [ On success hsm_client_tpm_create shall allocate a new instance of the secure device tpm interface. ] */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_031: [ secure_dev_tpm_create shall get a handle to the Endorsement Key and Storage Root Key. ] */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_030: [ secure_dev_tpm_create shall call into the tpm_codec to initialize a TSS session. ] */
    TEST_FUNCTION(hsm_client_tpm_create_succeed)
    {
        //arrange
        setup_hsm_client_tpm_create_mock();

        //act
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_001: [ If any failure is encountered hsm_client_tpm_create shall return NULL ] */
    TEST_FUNCTION(hsm_client_tpm_create_fail)
    {
        //arrange
        setup_hsm_client_tpm_create_mock();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 3, 8 };

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
            sprintf(tmp_msg, "secure_device_riot_create failure in test %zu/%zu", index, count);

            //act
            HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();

            //assert
            ASSERT_IS_NULL_WITH_MSG(sec_handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_004: [ hsm_client_tpm_destroy shall free the SEC_DEVICE_INFO instance. ] */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_006: [ hsm_client_tpm_destroy shall free all resources allocated in this module. ]*/
    TEST_FUNCTION(hsm_client_tpm_destroy_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(Deinit_TPM_Codec(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        //act
        hsm_client_tpm_destroy(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_005: [ if handle is NULL, hsm_client_tpm_destroy shall do nothing. ] */
    TEST_FUNCTION(hsm_client_tpm_destroy_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        hsm_client_tpm_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_007: [ if handle or key are NULL, or key_len is 0 hsm_client_tpm_import_key shall return a non-zero value ] */
    TEST_FUNCTION(hsm_client_tpm_import_key_handle_NULL_fail)
    {
        //arrange

        //act
        int import_res = hsm_client_tpm_import_key(NULL, TEST_IMPORT_KEY, TEST_KEY_SIZE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, import_res);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_007: [ if handle or key are NULL, or key_len is 0 hsm_client_tpm_import_key shall return a non-zero value ] */
    TEST_FUNCTION(hsm_client_tpm_import_key_key_NULL_fail)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        int import_res = hsm_client_tpm_import_key(sec_handle, NULL, TEST_KEY_SIZE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, import_res);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    TEST_FUNCTION(hsm_client_tpm_import_key_fail)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_hsm_client_tpm_import_key_mock();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 2, 3, 4, 5, 6, 7, 10, 13 };

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
            sprintf(tmp_msg, "hsm_client_tpm_import_key failure in test %zu/%zu", index, count);

            //act
            int import_res = hsm_client_tpm_import_key(sec_handle, TEST_IMPORT_KEY, TEST_KEY_SIZE);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, import_res, tmp_msg);
        }
        //cleanup
        hsm_client_tpm_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_hsm_client_tpm_import_key shall establish a tpm session in preparation to inserting the key into the tpm. */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_008: [ On success hsm_client_tpm_import_key shall return zero ] */
    TEST_FUNCTION(hsm_client_tpm_import_key_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        setup_hsm_client_tpm_import_key_mock();

        //act
        int import_res = hsm_client_tpm_import_key(sec_handle, TEST_IMPORT_KEY, TEST_KEY_SIZE);

        //assert
        ASSERT_ARE_EQUAL(int, 0, import_res);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_013: [ If handle is NULL hsm_client_tpm_get_endorsement_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_endorsement_key_handle_NULL_succeed)
    {
        unsigned char* key;
        size_t key_len;

        //act
        int result = hsm_client_tpm_get_endorsement_key(NULL, &key, &key_len);

        //assert
        //ASSERT_IS_NULL(key);
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_027: [ If the ek_public was not initialized hsm_client_tpm_get_endorsement_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_endorsement_key_size_0_fail)
    {
        g_rsa_size = 0;
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        int result = hsm_client_tpm_get_endorsement_key(NULL, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_015: [ If a failure is encountered, hsm_client_tpm_get_endorsement_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_endorsement_key_fail)
    {
        //arrange
        unsigned char* key;
        size_t key_len;

        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_hsm_client_tpm_get_endorsement_key_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0, 2, 4 };

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
            sprintf(tmp_msg, "hsm_client_tpm_get_endorsement_key failure in test %zu/%zu", index, count);

            int result = hsm_client_tpm_get_endorsement_key(NULL, &key, &key_len);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }
        //cleanup
        hsm_client_tpm_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_014: [ hsm_client_tpm_get_endorsement_key shall allocate and return the Endorsement Key. ] */
    TEST_FUNCTION(hsm_client_tpm_get_endorsement_key_succeed)
    {
        //arrange
        unsigned char* key;
        size_t key_len;

        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        setup_hsm_client_tpm_get_endorsement_key_mocks();

        //act
        int result = hsm_client_tpm_get_endorsement_key(sec_handle, &key, &key_len);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(key);
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_016: [ If handle is NULL, hsm_client_tpm_get_storage_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_storage_key_handle_NULL_fail)
    {
        //arrange
        unsigned char* key;
        size_t key_len;

        //act
        int result = hsm_client_tpm_get_storage_key(NULL, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_017: [ If the srk_public value was not initialized, hsm_client_tpm_get_storage_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_storage_key_size_0_fail)
    {
        unsigned char* key;
        size_t key_len;
        g_rsa_size = 0;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        int result = hsm_client_tpm_get_storage_key(NULL, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_019: [ If any failure is encountered, hsm_client_tpm_get_storage_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_get_storage_key_fail)
    {
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_hsm_client_tpm_get_storage_key_mocks();

        umock_c_negative_tests_snapshot();

        size_t calls_cannot_fail[] = { 0, 2, 4 };

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
            sprintf(tmp_msg, "hsm_client_tpm_get_storage_key failure in test %zu/%zu", index, count);

            int result = hsm_client_tpm_get_storage_key(NULL, &key, &key_len);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_018: [ hsm_client_tpm_get_storage_key shall allocate and return the Storage Root Key. ] */
    TEST_FUNCTION(hsm_client_tpm_get_storage_key_succeed)
    {
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        setup_hsm_client_tpm_get_storage_key_mocks();

        //act
        int result = hsm_client_tpm_get_storage_key(sec_handle, &key, &key_len);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(key);
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_020: [ If handle or data is NULL or data_len is 0, hsm_client_tpm_sign_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_sign_data_handle_fail)
    {
        unsigned char* key;
        size_t key_len;

        //arrange

        //act
        int result = hsm_client_tpm_sign_data(NULL, TEST_BUFFER, TEST_BUFFER_SIZE, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_020: [ If handle or data is NULL or data_len is 0, hsm_client_tpm_sign_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_sign_data_data_NULL_fail)
    {
        //arrange
        unsigned char* key;
        size_t key_len;

        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        int result = hsm_client_tpm_sign_data(NULL, TEST_BUFFER, TEST_BUFFER_SIZE, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_020: [ If handle or data is NULL or data_len is 0, hsm_client_tpm_sign_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_sign_data_size_0_fail)
    {
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        int result = hsm_client_tpm_sign_data(NULL, TEST_BUFFER, TEST_BUFFER_SIZE, &key, &key_len);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_023: [ If an error is encountered hsm_client_tpm_sign_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_sign_data_fail)
    {
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        setup_hsm_client_tpm_sign_data_mocks();

        umock_c_negative_tests_snapshot();

        size_t count = umock_c_negative_tests_call_count();
        for (size_t index = 0; index < count; index++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            char tmp_msg[64];
            sprintf(tmp_msg, "hsm_client_tpm_sign_data failure in test %zu/%zu", index, count);

            //act
            int result = hsm_client_tpm_sign_data(NULL, TEST_BUFFER, TEST_BUFFER_SIZE, &key, &key_len);

            //assert
            ASSERT_ARE_NOT_EQUAL_WITH_MSG(int, 0, result, tmp_msg);
        }

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_021: [ hsm_client_tpm_sign_data shall call into the tpm to hash the supplied data value. ] */
    /* Tests_SRS_SECURE_DEVICE_TPM_07_022: [ If hashing the data was successful, hsm_client_tpm_sign_data shall create a BUFFER_HANDLE with the supplied signed data. ] */
    TEST_FUNCTION(hsm_client_tpm_sign_data_succeed)
    {
        unsigned char* key;
        size_t key_len;

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        setup_hsm_client_tpm_sign_data_mocks();

        //act
        int result = hsm_client_tpm_sign_data(sec_handle, TEST_BUFFER, TEST_BUFFER_SIZE, &key, &key_len);

        //assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(key);
        hsm_client_tpm_destroy(sec_handle);
    }

#if 0
    /* Tests_SRS_SECURE_DEVICE_TPM_07_025: [ If handle or data is NULL or data_len is 0, hsm_client_tpm_decrypt_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_decrypt_data_handle_NULL_fail)
    {
        //arrange

        //act
        BUFFER_HANDLE decrypt_value = hsm_client_tpm_decrypt_data(NULL, TEST_BUFFER, TEST_BUFFER_SIZE);

        //assert
        ASSERT_IS_NULL(decrypt_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_029: [ If an error is encountered secure_dev_tpm_decrypt_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_decrypt_data_data_NULL_fail)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        BUFFER_HANDLE decrypt_value = hsm_client_tpm_decrypt_data(sec_handle, NULL, TEST_BUFFER_SIZE);

        //assert
        ASSERT_IS_NULL(decrypt_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(decrypt_value);
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_025: [ If handle or data is NULL or data_len is 0, hsm_client_tpm_decrypt_data shall return NULL. ] */
    TEST_FUNCTION(hsm_client_tpm_decrypt_data_data_len_0_fail)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        BUFFER_HANDLE decrypt_value = hsm_client_tpm_decrypt_data(sec_handle, TEST_BUFFER, 0);

        //assert
        ASSERT_IS_NULL(decrypt_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(decrypt_value);
        hsm_client_tpm_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_TPM_07_024: [ hsm_client_tpm_decrypt_data shall call into the tpm to decrypt the supplied data value. ] */
    TEST_FUNCTION(hsm_client_tpm_decrypt_data_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        TPM_SE tmp_se = 0;
        TPMA_SESSION tmp_session = { 0 };

        STRICT_EXPECTED_CALL(TSS_StartAuthSession(IGNORED_PTR_ARG, tmp_se, IGNORED_NUM_ARG, tmp_session, IGNORED_PTR_ARG))
            .IgnoreArgument_sessAttrs()
            .IgnoreArgument_sessionType();
        STRICT_EXPECTED_CALL(TSS_PolicySecret(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 4
        STRICT_EXPECTED_CALL(TSS_GetTpmProperty(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(TPM2B_ID_OBJECT_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_ENCRYPTED_SECRET_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_PRIVATE_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_ENCRYPTED_SECRET_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2B_PUBLIC_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
        STRICT_EXPECTED_CALL(UINT16_Unmarshal(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(TPM2_ActivateCredential(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        //act
        BUFFER_HANDLE decrypt_value = hsm_client_tpm_decrypt_data(sec_handle, TEST_BUFFER, TEST_BUFFER_SIZE);

        //assert
        ASSERT_IS_NOT_NULL(decrypt_value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(decrypt_value);
        hsm_client_tpm_destroy(sec_handle);
    }
#endif

    /* Tests_SRS_SECURE_DEVICE_TPM_07_026: [ hsm_client_tpm_interface shall return the SEC_TPM_INTERFACE structure. ] */
    TEST_FUNCTION(hsm_client_tpm_interface_succeed)
    {
        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_tpm_create();
        umock_c_reset_all_calls();

        //act
        const HSM_CLIENT_TPM_INTERFACE* tpm_iface = hsm_client_tpm_interface();

        //assert
        ASSERT_IS_NOT_NULL(tpm_iface);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_tpm_create);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_tpm_destroy);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_get_ek);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_get_srk);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_import_key);
        ASSERT_IS_NOT_NULL(tpm_iface->hsm_client_sign_data);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_tpm_destroy(sec_handle);
    }

    END_TEST_SUITE(hsm_client_tpm_ut)
