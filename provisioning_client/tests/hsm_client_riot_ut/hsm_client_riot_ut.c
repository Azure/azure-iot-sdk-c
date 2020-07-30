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

#if defined _MSC_VER
#pragma warning(disable: 4054) /* MSC incorrectly fires this */
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
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#include "RIoT.h"
#include "RiotCrypt.h"
#include "derenc.h"
#include "x509bldr.h"
#include "DiceSha256.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"

MOCKABLE_FUNCTION(, void, DiceSHA256, const uint8_t*, buf, size_t, bufSize, uint8_t*, digest);
MOCKABLE_FUNCTION(, void, DiceSHA256_2, const uint8_t*, buf1, size_t, bufSize1, const uint8_t*, buf2, size_t, bufSize2, uint8_t*, digest);
MOCKABLE_FUNCTION(, RIOT_STATUS, RiotCrypt_Hash, uint8_t*, hash_result, size_t, resultSize, const void*, data, size_t, dataSize);
MOCKABLE_FUNCTION(, RIOT_STATUS, RiotCrypt_DeriveEccKey, RIOT_ECC_PUBLIC*, publicPart, RIOT_ECC_PRIVATE*, privatePart, const void*, srcData, size_t, srcDataSize, const uint8_t*, label, size_t, labelSize);
MOCKABLE_FUNCTION(, RIOT_STATUS, RiotCrypt_Hash2, uint8_t*, hash_result, size_t, resultSize, const void*, data1, size_t, data1Size, const void*, data2, size_t, data2Size);
MOCKABLE_FUNCTION(, void, DERInitContext, DERBuilderContext*, Context, uint8_t*, Buffer, uint32_t, Length);
MOCKABLE_FUNCTION(, int, X509GetDEREncodedTBS, DERBuilderContext*, Tbs, RIOT_X509_TBS_DATA*, TbsData, RIOT_ECC_PUBLIC*, AliasKeyPub, RIOT_ECC_PUBLIC*, DevIdKeyPub, uint8_t*, Fwid, uint32_t, FwidLen);
MOCKABLE_FUNCTION(, RIOT_STATUS, RiotCrypt_Sign, RIOT_ECC_SIGNATURE*, sig, const void*, data, size_t, dataSize, const RIOT_ECC_PRIVATE*, key);
MOCKABLE_FUNCTION(, int, X509MakeAliasCert, DERBuilderContext*, AliasCert, RIOT_ECC_SIGNATURE*, TbsSig);
MOCKABLE_FUNCTION(, int, X509MakeRootCert, DERBuilderContext*, RootCert, RIOT_ECC_SIGNATURE*, TbsSig);

MOCKABLE_FUNCTION(, int, X509GetDEREcc, DERBuilderContext*, Context, RIOT_ECC_PUBLIC, Pub, RIOT_ECC_PRIVATE, Priv);
MOCKABLE_FUNCTION(, int, DERtoPEM, DERBuilderContext*, Context, uint32_t, Type, char*, PEM, uint32_t*, Length);

MOCKABLE_FUNCTION(, int, X509GetDEREccPub, DERBuilderContext*, Context, RIOT_ECC_PUBLIC, Pub);
MOCKABLE_FUNCTION(, int, X509GetDeviceCertTBS, DERBuilderContext*, Tbs, RIOT_X509_TBS_DATA *, TbsData, RIOT_ECC_PUBLIC*, DevIdKeyPub, uint8_t*, RootKeyPub, uint32_t, RootKeyPubLen);
MOCKABLE_FUNCTION(, int, X509MakeDeviceCert, DERBuilderContext*, DeviceIDCert, RIOT_ECC_SIGNATURE*, TbsSig);
MOCKABLE_FUNCTION(, int, X509GetAliasCertTBS, DERBuilderContext*, Tbs, RIOT_X509_TBS_DATA*, TbsData, RIOT_ECC_PUBLIC*, AliasKeyPub, RIOT_ECC_PUBLIC*, DevIdKeyPub, uint8_t*, Fwid, uint32_t, FwidLen);
MOCKABLE_FUNCTION(, int, X509GetDERCsrTbs, DERBuilderContext*, Context, RIOT_X509_TBS_DATA*, TbsData, RIOT_ECC_PUBLIC*, DeviceIDPub);
MOCKABLE_FUNCTION(, int, X509GetDERCsr, DERBuilderContext*, Context, RIOT_ECC_SIGNATURE*, Signature);
MOCKABLE_FUNCTION(, int, X509GetRootCertTBS, DERBuilderContext*, Tbs, RIOT_X509_TBS_DATA*, TbsData, RIOT_ECC_PUBLIC*, DevIdKeyPub);
MOCKABLE_FUNCTION(, void, mbedtls_mpi_free, mbedtls_mpi*, X);
MOCKABLE_FUNCTION(, void, mbedtls_ecp_point_free, mbedtls_ecp_point*, pt);
MOCKABLE_FUNCTION(, int, mbedtls_mpi_lset, mbedtls_mpi*, X, mbedtls_mpi_sint, z);
MOCKABLE_FUNCTION(, int, mbedtls_mpi_read_binary, mbedtls_mpi*, X, const unsigned char*, buf, size_t, buflen);

#undef ENABLE_MOCKS

#include "hsm_client_riot.h"
#include "hsm_client_data.h"

// Defines for mock aliasing mbedtls_mpi_lset
#if defined(MBEDTLS_HAVE_INT32)
#define MPI_SINT int32_t
#elif defined(MBEDTLS_HAVE_INT64)
#define MPI_SINT int64_t
#endif

static const char* TEST_STRING_VALUE = "Test_String_Value";
static const char* TEST_CERTIFICATE_VALUE = "Test_String_ValueTest_String_Value";
static const char* TEST_CN_VALUE = "riot-device-cert";

static int umocktypes_copy_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* dest, const RIOT_ECC_PRIVATE* src)
{
    int result;
    if (src == NULL) 
    {
        dest = NULL;
        result = 0;
    }
    else
    {
        // copy the sign, total number of limbs, and address of first limb
        dest->s = src->s;
        dest->n = src->n;
        dest->p = src->p;
        result = 0;
    }
    return result;
}

static void umocktypes_free_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* value)
{
    (void)value;
}

static char* umocktypes_stringify_RIOT_ECC_PRIVATE(const RIOT_ECC_PRIVATE* value)
{
    char* result;
    if (value == NULL)
    {
        result = (char*)my_gballoc_malloc(5);
        if (result != NULL)
        {
            (void)memcpy(result, "NULL", 5);
        }
    }
    else
    {
        int length = snprintf(NULL, 0, "{ %d, %zd, %p }",
            value->s, value->n, value->p);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            //Add for NULL terminator
            length++;
            result = (char*)my_gballoc_malloc(length);
            if (result != NULL)
            {
                (void)snprintf(result, length, "{ %d, %zd, %p }",
                    value->s, value->n, value->p);
            }
        }
    }
    return result;
}

static int umocktypes_are_equal_RIOT_ECC_PRIVATE(RIOT_ECC_PRIVATE* left, RIOT_ECC_PRIVATE* right)
{
    int result;
    if ((right == NULL) && (left == NULL))
    {
        result = 1;
    }
    else if (((right == NULL) && (left != NULL)) || ((left == NULL) && (right != NULL)))
    {
        result = 0;
    }
    else if ((right->s != left->s) || (right->n != left->n) || (right->p != left->p))
    {
        result = 0;
    }
    else
    {
        result = 1;
    }
    return result;
}

static int umocktypes_copy_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* dest, const RIOT_ECC_PUBLIC* src)
{
    int result;
    if (src == NULL)
    {
        dest = NULL;
        result = 0;
    }
    else
    {
        // X point
        dest->X.s = src->X.s;
        dest->X.n = src->X.n;
        dest->X.p = src->X.p;
        // Y point
        dest->Y.s = src->Y.s;
        dest->Y.n = src->Y.n;
        dest->Y.p = src->Y.p;
        // Z point
        dest->Z.s = src->Z.s;
        dest->Z.n = src->Z.n;
        dest->Z.p = src->Z.p;
        result = 0;
    }
    return result;
}

static void umocktypes_free_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* value)
{
    (void)value;
}

static char* umocktypes_stringify_RIOT_ECC_PUBLIC(const RIOT_ECC_PUBLIC* value)
{
    char* result;
    if (value == NULL)
    {
        result = (char*)my_gballoc_malloc(5);
        if (result != NULL)
        {
            (void)memcpy(result, "NULL", 5);
        }
    }
    else
    {
        int length = snprintf(NULL, 0, "{ %d, %zd, %p, %d, %zd, %p, %d, %zd, %p }",
                    value->X.s, value->X.n, value->X.p, 
                    value->Y.s, value->Y.n, value->Y.p, 
                    value->Z.s, value->Z.n, value->Z.p);
        if (length < 0)
        {
            result = NULL;
        }
        else
        {
            //Add for NULL terminator
            length++;
            result = (char*)my_gballoc_malloc(length);
            if (result != NULL)
            {
                (void)snprintf(result, length, "{ %d, %zd, %p, %d, %zd, %p, %d, %zd, %p }",
                    value->X.s, value->X.n, value->X.p, 
                    value->Y.s, value->Y.n, value->Y.p, 
                    value->Z.s, value->Z.n, value->Z.p);
            }
        }
    }
    return result;
}

static int umocktypes_are_equal_RIOT_ECC_PUBLIC(RIOT_ECC_PUBLIC* left, RIOT_ECC_PUBLIC* right)
{
    int result;
    if ((right == NULL) && (left == NULL))
    {
        result = 1;
    }
    else if (((right == NULL) && (left != NULL)) || ((left == NULL) && (right != NULL)))
    {
        result = 0;
    }
    else if ((right->X.s != left->X.s) || (right->X.n != left->X.n) || (right->X.p != left->X.p))
    {
        // X point
        result = 0;
    }
    else if ((right->Y.s != left->Y.s) || (right->Y.n != left->Y.n) || (right->Y.p != left->Y.p))
    {
        // Y point
        result = 0;
    }
    else if ((right->Z.s != left->Z.s) || (right->Z.n != left->Z.n) || (right->Z.p != left->Z.p))
    {
        // Z point
        result = 0;
    }
    else
    {
        result = 1;
    }
    return result;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)my_gballoc_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static void my_DiceSHA256(const uint8_t *buf, size_t bufSize, uint8_t *digest)
{
    (void)buf;
    unsigned char counter = 0x01;
    for (size_t index = 0; index < bufSize; index++)
    {
        digest[0] = counter++;
    }
}

static void my_DiceSHA256_2(const uint8_t *buf1, size_t bufSize1, const uint8_t *buf2, size_t bufSize2, uint8_t *digest)
{
    (void)buf1;
    (void)bufSize1;
    (void)buf2;
    unsigned char counter = 0x01;
    for (size_t index = 0; index < bufSize2; index++)
    {
        digest[0] = counter++;
    }
}

static int my_DERtoPEM(DERBuilderContext* Context, uint32_t Type, char* PEM, uint32_t* Length)
{
    (void)Context;
    (void)Type;

    strcpy(PEM, TEST_STRING_VALUE);
    *Length = (uint32_t)strlen(PEM);
    return 0;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void mbedtls_error_on_free()
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error attempting to double-free :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, UMOCK_C_NULL_ARGUMENT));
    ASSERT_FAIL(temp_str);
}

static void my_mbedtls_mpi_free(mbedtls_mpi* X)
{
    if (X == NULL)
    {
        mbedtls_error_on_free();
        return;
    }
    if (X->p != NULL)
    {
        memset(X->p, 0, X->n);
        free(X->p);
    }

    X->s = 1;
    X->n = 0;
    X->p = NULL;
}

static void my_mbedtls_ecp_point_free(mbedtls_ecp_point* pt)
{
    if (pt == NULL)
    {
        mbedtls_error_on_free();
        return;
    }

    my_mbedtls_mpi_free(&(pt->X));
    my_mbedtls_mpi_free(&(pt->Y));
    my_mbedtls_mpi_free(&(pt->Z));
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(hsm_client_riot_ut)

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

        REGISTER_UMOCK_ALIAS_TYPE(HSM_CLIENT_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(mbedtls_mpi_sint, MPI_SINT);
        REGISTER_TYPE(RIOT_ECC_PUBLIC, RIOT_ECC_PUBLIC);
        REGISTER_TYPE(RIOT_ECC_PRIVATE, RIOT_ECC_PRIVATE);

        REGISTER_GLOBAL_MOCK_RETURN(RiotCrypt_Hash, RIOT_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RiotCrypt_Hash, RIOT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(RiotCrypt_DeriveEccKey, RIOT_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RiotCrypt_DeriveEccKey, RIOT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(RiotCrypt_Hash2, RIOT_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RiotCrypt_Hash2, RIOT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetDEREncodedTBS, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetDEREncodedTBS, 1);
        REGISTER_GLOBAL_MOCK_RETURN(RiotCrypt_Sign, RIOT_SUCCESS);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(RiotCrypt_Sign, RIOT_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(X509MakeAliasCert, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509MakeAliasCert, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetDEREcc, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetDEREcc, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetDEREccPub, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetDEREccPub, 1);
        REGISTER_GLOBAL_MOCK_HOOK(DERtoPEM, my_DERtoPEM);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(DERtoPEM, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetAliasCertTBS, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetAliasCertTBS, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509MakeDeviceCert, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509MakeDeviceCert, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509MakeRootCert, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509MakeRootCert, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetDERCsr, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetDERCsr, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetDeviceCertTBS, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetDeviceCertTBS, 1);
        REGISTER_GLOBAL_MOCK_RETURN(X509GetRootCertTBS, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(X509GetRootCertTBS, 1);

        REGISTER_GLOBAL_MOCK_HOOK(mbedtls_mpi_free, my_mbedtls_mpi_free);
        REGISTER_GLOBAL_MOCK_HOOK(mbedtls_ecp_point_free, my_mbedtls_ecp_point_free);

        REGISTER_GLOBAL_MOCK_HOOK(DiceSHA256, my_DiceSHA256);
        REGISTER_GLOBAL_MOCK_HOOK(DiceSHA256_2, my_DiceSHA256_2);

        REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
        REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
        REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, __LINE__);
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

    static void hsm_client_riot_create_leaf_cert_mock(void)
    {
        // Expected calls preceeded by a commented number are members of calls_cannot_fail[] array
        // These calls are skipped in negative/fail testing
        STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 0
        STRICT_EXPECTED_CALL(X509GetAliasCertTBS(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_Sign(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(X509MakeDeviceCert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, CERT_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG)); // 6
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG)); // 7
    }

    static void hsm_client_riot_create_mock(bool device_signed)
    {
        RIOT_ECC_PUBLIC pub = { 0 };
        RIOT_ECC_PRIVATE pri = { 0 };

        // Expected calls preceeded by a commented number are members of calls_cannot_fail[] array
        // These calls are skipped in negative/fail testing
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_Hash(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_DeriveEccKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_Hash2(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_DeriveEccKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 5
        STRICT_EXPECTED_CALL(X509GetDEREccPub(IGNORED_PTR_ARG, pub))
            .IgnoreArgument_Pub();
        STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, PUBLICKEY_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 8
        STRICT_EXPECTED_CALL(X509GetDEREcc(IGNORED_PTR_ARG, pub, pri))
            .IgnoreArgument_Pub()
            .IgnoreArgument_Priv();
        STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, ECC_PRIVATEKEY_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 11
        STRICT_EXPECTED_CALL(X509GetAliasCertTBS(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_Sign(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(X509MakeAliasCert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, CERT_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        if (device_signed)
        {
            STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 16
            STRICT_EXPECTED_CALL(X509GetDeviceCertTBS(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(RiotCrypt_Sign(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(X509MakeDeviceCert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, CERT_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        }
        else
        {
            STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 17
            STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 18
            // Generate the CA_Root using the development key
            STRICT_EXPECTED_CALL(mbedtls_mpi_read_binary(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 19
            STRICT_EXPECTED_CALL(mbedtls_mpi_read_binary(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 20
            STRICT_EXPECTED_CALL(mbedtls_mpi_lset(IGNORED_PTR_ARG, IGNORED_NUM_ARG));                         // 21
            STRICT_EXPECTED_CALL(mbedtls_mpi_read_binary(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 22

            STRICT_EXPECTED_CALL(X509GetRootCertTBS(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(RiotCrypt_Sign(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));

            STRICT_EXPECTED_CALL(X509MakeRootCert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
            STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, CERT_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
            STRICT_EXPECTED_CALL(X509GetDEREcc(IGNORED_PTR_ARG, pub, pri))
                .IgnoreArgument_Pub()
                .IgnoreArgument_Priv();
            STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, ECC_PRIVATEKEY_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        }

        // Produce root-signed device cert
        STRICT_EXPECTED_CALL(DERInitContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG)); // 28
        STRICT_EXPECTED_CALL(X509GetDeviceCertTBS(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
        STRICT_EXPECTED_CALL(RiotCrypt_Sign(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(X509MakeDeviceCert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(DERtoPEM(IGNORED_PTR_ARG, CERT_TYPE, IGNORED_PTR_ARG, IGNORED_NUM_ARG));

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG)); // 34
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG)); // 35
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_001: [ On success hsm_client_riot_create shall allocate a new instance of the device auth interface. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_002: [ hsm_client_riot_create shall call into the RIot code to sign the RIoT certificate. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_003: [ hsm_client_riot_create shall cache the device id public value from the RIoT module. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_004: [ hsm_client_riot_create shall cache the alias key pair value from the RIoT module. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_005: [ hsm_client_riot_create shall create the Signer regions of the alias key certificate. ]*/
    TEST_FUNCTION(hsm_client_riot_create_succeed)
    {
        hsm_client_x509_init();
        umock_c_reset_all_calls();

        //arrange
        hsm_client_riot_create_mock(false);

        //act
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();

        //assert
        ASSERT_IS_NOT_NULL(sec_handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_006: [ If any failure is encountered hsm_client_riot_create shall return NULL ] */
    TEST_FUNCTION(hsm_client_riot_create_fail)
    {
        hsm_client_x509_init();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        //arrange
        hsm_client_riot_create_mock(false);

        umock_c_negative_tests_snapshot();

        // List of calls that we are not testing failures on: [ hsm_client_riot_create_mock ]
        size_t calls_cannot_fail[] = { 5, 8, 11, 16, 17, 18, 19, 20, 21, 22, 28, 34, 35 };

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
            sprintf(tmp_msg, "hsm_client_riot_create failure in test %zu/%zu", index, count);

            //act
            HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();

            //assert
            ASSERT_IS_NULL(sec_handle, tmp_msg);
        }

        //cleanup
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_008: [ hsm_client_riot_destroy shall free the HSM_CLIENT_HANDLE instance. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_009: [ hsm_client_riot_destroy shall free all resources allocated in this module. ] */
    TEST_FUNCTION(hsm_client_riot_destroy_succeed)
    {
        hsm_client_x509_init();
        umock_c_reset_all_calls();

        //arrange
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mbedtls_ecp_point_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mbedtls_ecp_point_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(mbedtls_ecp_point_free(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(mbedtls_mpi_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(gballoc_free(sec_handle));

        //act
        hsm_client_riot_destroy(sec_handle);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_007: [ if handle is NULL, hsm_client_riot_destroy shall do nothing. ] */
    TEST_FUNCTION(hsm_client_riot_destroy_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        hsm_client_riot_destroy(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_010: [ if handle is NULL, hsm_client_riot_get_certificate shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_get_certificate_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        char* value = hsm_client_riot_get_certificate(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_013: [ If any failure is encountered hsm_client_riot_get_certificate shall return NULL ] */
    TEST_FUNCTION(hsm_client_riot_get_certificate_malloc_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        char* value = hsm_client_riot_get_certificate(sec_handle);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_011: [ hsm_client_riot_get_certificate shall allocate a char* to return the riot certificate. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_012: [ On success hsm_client_riot_get_certificate shall return the riot certificate. ] */
    TEST_FUNCTION(hsm_client_riot_get_certificate_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        char* value = hsm_client_riot_get_certificate(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_CERTIFICATE_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_014: [ if handle is NULL, hsm_client_riot_get_alias_key shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_get_alias_key_handle_NULL_succeed)
    {
        //arrange

        //act
        char* value = hsm_client_riot_get_alias_key(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_017: [ If any failure is encountered hsm_client_riot_get_alias_key shall return NULL ] */
    TEST_FUNCTION(hsm_client_riot_get_alias_key_malloc_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        char* value = hsm_client_riot_get_alias_key(sec_handle);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_015: [ hsm_client_riot_get_alias_key shall allocate a char* to return the alias certificate. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_016: [ On success hsm_client_riot_get_alias_key shall return the alias certificate. ] */
    TEST_FUNCTION(hsm_client_riot_get_alias_key_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        char* value = hsm_client_riot_get_alias_key(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_022: [ if handle is NULL, hsm_client_riot_get_signer_cert shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_get_signer_cert_handle_NULL_succeed)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        char* value = hsm_client_riot_get_signer_cert(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_025: [ If any failure is encountered hsm_client_riot_get_signer_cert shall return NULL ] */
    TEST_FUNCTION(hsm_client_riot_get_signer_cert_malloc_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG)).SetReturn(NULL);

        //act
        char* value = hsm_client_riot_get_signer_cert(sec_handle);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_024: [ On success hsm_client_riot_get_signer_cert shall return the signer certificate. ] */
    /* Tests_SRS_SECURE_DEVICE_RIOT_07_023: [ hsm_client_riot_get_signer_cert shall allocate a char* to return the signer certificate. ] */
    TEST_FUNCTION(hsm_client_riot_get_signer_cert_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        char* value = hsm_client_riot_get_signer_cert(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }

    TEST_FUNCTION(hsm_client_riot_get_root_cert_handle_NULL_fail)
    {
        //arrange
        umock_c_reset_all_calls();

        //act
        char* value = hsm_client_riot_get_root_cert(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    TEST_FUNCTION(hsm_client_riot_get_root_cert_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

        //act
        char* value = hsm_client_riot_get_root_cert(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_STRING_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }


    /* Tests_SRS_SECURE_DEVICE_RIOT_07_026: [ if handle is NULL, hsm_client_riot_get_common_name shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_get_common_name_handle_NULL_succeed)
    {
        //arrange

        //act
        char* value = hsm_client_riot_get_common_name(NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_028: [ If any failure is encountered hsm_client_riot_get_signer_cert shall return NULL ] */
    TEST_FUNCTION(hsm_client_riot_get_common_name_mallocAndStrcpy_s_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(1);

        //act
        char* value = hsm_client_riot_get_common_name(sec_handle);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_027: [ hsm_client_riot_get_common_name shall allocate a char* to return the certificate common name. ] */
    TEST_FUNCTION(hsm_client_riot_get_common_name_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

        //act
        char* value = hsm_client_riot_get_common_name(sec_handle);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, TEST_CN_VALUE, value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_HSM_RIOT_07_030: [ If handle or common_name is NULL, hsm_client_riot_create_leaf_cert shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_create_leaf_cert_handle_NULL_fail)
    {
        //arrange

        //act
        char* value = hsm_client_riot_create_leaf_cert(NULL, TEST_CN_VALUE);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
    }

    /* Tests_SRS_DPS_HSM_RIOT_07_030: [ If handle or common_name is NULL, hsm_client_riot_create_leaf_cert shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_create_leaf_cert_common_name_NULL_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        //act
        char* value = hsm_client_riot_create_leaf_cert(sec_handle, NULL);

        //assert
        ASSERT_IS_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_HSM_RIOT_07_031: [ If successful hsm_client_riot_create_leaf_cert shall return a leaf cert with the CN of common_name. ] */
    TEST_FUNCTION(hsm_client_riot_create_leaf_cert_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        hsm_client_riot_create_leaf_cert_mock();

        //act
        char* value = hsm_client_riot_create_leaf_cert(sec_handle, TEST_CN_VALUE);

        //assert
        ASSERT_IS_NOT_NULL(value);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        my_gballoc_free(value);
        hsm_client_riot_destroy(sec_handle);
    }

    /* Tests_SRS_DPS_HSM_RIOT_07_032: [ If hsm_client_riot_create_leaf_cert encounters an error it shall return NULL. ] */
    TEST_FUNCTION(hsm_client_riot_create_leaf_cert_fail)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        int negativeTestsInitResult = umock_c_negative_tests_init();
        ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

        hsm_client_riot_create_leaf_cert_mock();

        umock_c_negative_tests_snapshot();

        // List of calls that we are not testing failures on: [ hsm_client_riot_create_leaf_cert_mock ]
        size_t calls_cannot_fail[] = { 0, 6, 7 };

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
            sprintf(tmp_msg, "hsm_client_riot_create_leaf_cert failure in test %zu/%zu", index, count);

            char* value = hsm_client_riot_create_leaf_cert(sec_handle, TEST_CN_VALUE);

            //assert
            ASSERT_IS_NULL(value, tmp_msg);
        }

        //cleanup
        hsm_client_riot_destroy(sec_handle);
        umock_c_negative_tests_deinit();
    }

    /* Tests_SRS_SECURE_DEVICE_RIOT_07_029: [ hsm_client_riot_interface shall return the SEC_RIOT_INTERFACE structure. ] */
    TEST_FUNCTION(hsm_client_riot_interface_succeed)
    {
        //arrange
        hsm_client_x509_init();
        HSM_CLIENT_HANDLE sec_handle = hsm_client_riot_create();
        umock_c_reset_all_calls();

        //act
        const HSM_CLIENT_X509_INTERFACE* riot_iface = hsm_client_x509_interface();

        //assert
        ASSERT_IS_NOT_NULL(riot_iface);
        ASSERT_IS_NOT_NULL(riot_iface->hsm_client_x509_create);
        ASSERT_IS_NOT_NULL(riot_iface->hsm_client_x509_destroy);
        ASSERT_IS_NOT_NULL(riot_iface->hsm_client_get_cert);
        ASSERT_IS_NOT_NULL(riot_iface->hsm_client_get_key);
        ASSERT_IS_NOT_NULL(riot_iface->hsm_client_get_common_name);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        hsm_client_riot_destroy(sec_handle);
    }

    END_TEST_SUITE(hsm_client_riot_ut)
