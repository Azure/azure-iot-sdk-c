// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gb_rand.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "x509_info.h"

#include "RIoT.h"
#include "RiotCrypt.h"
#include "derenc.h"
#include "x509bldr.h"
#include "DiceSha256.h"

typedef struct X509_CERT_INFO_TAG
{
    char device_name[64];

    RIOT_ECC_PUBLIC     device_id_pub;
    RIOT_ECC_PRIVATE    device_id_priv;

    RIOT_ECC_PUBLIC     alias_key_pub;
    RIOT_ECC_PRIVATE    alias_key_priv;

    RIOT_ECC_PUBLIC     ca_root_pub;
    RIOT_ECC_PRIVATE    ca_root_priv;

    uint32_t device_id_length;
    char device_id_public_pem[DER_MAX_PEM];

    uint32_t alias_key_length;
    char alias_priv_key_pem[DER_MAX_PEM];

    uint32_t alias_cert_length;
    char alias_cert_pem[DER_MAX_PEM];

    uint32_t device_signed_length;
    char device_signed_pem[DER_MAX_PEM];

    uint32_t root_ca_length;
    char root_ca_pem[DER_MAX_PEM];

    uint32_t root_ca_priv_length;
    char root_ca_priv_pem[DER_MAX_PEM];
} X509_CERT_INFO;

static const char* g_subject_name = NULL;

static const char* const ECCTYPE = "prime256v1";
static const char* const COMMON_NAME = "hsm_cert_example";
static const char* const DEVICE_PREFIX = "hsm-dev-";

#define HSM_SIGNER_NAME            "hsm-signer"
#define HSM_CA_CERT_NAME           "hsm-ca-cert"

#define DICE_UDS_LENGTH             0x20
#define RANDOM_BUFFER_LENGTH        8
#define DEVICE_GUID_SIZE            37

static unsigned char g_uds_seed[DICE_UDS_LENGTH];
static unsigned char firmware_id[RIOT_DIGEST_LENGTH] = { 0 };
static uint8_t g_digest[RIOT_DIGEST_LENGTH] = { 0 };
static uint8_t g_CDI[RIOT_DIGEST_LENGTH] = { 0 };
static char g_device_name[64] = { 0 };

static int g_digest_initialized = 0;

// Free the mbedtls_mpi members of the signature
static void ecc_signature_destroy(RIOT_ECC_SIGNATURE* tbs_sig);

static RIOT_X509_TBS_DATA X509_ALIAS_TBS_DATA = {
    { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E }, HSM_SIGNER_NAME, "AZURE_TEST", "US",
    "170101000000Z", "370101000000Z", "", "MSR_TEST", "US" };

// The static data fields that make up the DeviceID Cert "to be signed" region
static RIOT_X509_TBS_DATA X509_DEVICE_TBS_DATA = {
    { 0x0E, 0x0D, 0x0C, 0x0B, 0x0A }, HSM_CA_CERT_NAME, "AZURE_TEST", "US",
    "170101000000Z", "370101000000Z", HSM_SIGNER_NAME, "AZURE_TEST", "US" };

// The static data fields that make up the "root signer" Cert
static RIOT_X509_TBS_DATA X509_ROOT_TBS_DATA = {
    { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E }, HSM_CA_CERT_NAME, "AZURE_TEST", "US",
    "170101000000Z", "370101000000Z", HSM_CA_CERT_NAME, "AZURE_TEST", "US" };

static int generate_root_ca_info(X509_CERT_INFO* x509_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };
    DERBuilderContext der_pri_ctx = { 0 };
    RIOT_STATUS status;
    RIOT_ECC_SIGNATURE tbs_sig;

    // Generating "root"-signed DeviceID certificate
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    DERInitContext(&der_pri_ctx, der_buffer, DER_MAX_TBS);

    if ((status = RiotCrypt_DeriveEccKey(&x509_info->ca_root_pub, &x509_info->ca_root_priv,
        g_digest, RIOT_DIGEST_LENGTH, (const uint8_t*)RIOT_LABEL_ALIAS, lblSize(RIOT_LABEL_ALIAS))) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_DeriveEccKey returned invalid status %d.", status);
        result = MU_FAILURE;
    }

    if (X509GetDeviceCertTBS(&der_ctx, &X509_ROOT_TBS_DATA, &x509_info->ca_root_pub, (uint8_t*)&x509_info->ca_root_pub, sizeof(x509_info->ca_root_pub)) != 0)
    {
        LogError("Failure: X509GetDeviceCertTBS");
        result = MU_FAILURE;
    }
    // Sign the DeviceID Certificate's TBS region
    else if ((status = RiotCrypt_Sign(&tbs_sig, der_ctx.Buffer, der_ctx.Position, &x509_info->ca_root_priv)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if (X509MakeRootCert(&der_ctx, &tbs_sig) != 0)
    {
        LogError("Failure: X509MakeRootCert");
        result = MU_FAILURE;
    }
    else
    {
        x509_info->root_ca_priv_length = sizeof(x509_info->root_ca_priv_pem);
        x509_info->root_ca_length = sizeof(x509_info->root_ca_pem);

        if (DERtoPEM(&der_ctx, CERT_TYPE, x509_info->root_ca_pem, &x509_info->root_ca_length) != 0)
        {
            LogError("Failure: DERtoPEM return invalid value.");
            result = MU_FAILURE;
        }
        else if (X509GetDEREcc(&der_pri_ctx, x509_info->ca_root_pub, x509_info->ca_root_priv) != 0)
        {
            LogError("Failure: X509GetDEREcc returned invalid status.");
            result = MU_FAILURE;
        }
        else if (DERtoPEM(&der_pri_ctx, ECC_PRIVATEKEY_TYPE, x509_info->root_ca_priv_pem, &x509_info->root_ca_priv_length) != 0)
        {
            LogError("Failure: DERtoPEM returned invalid status.");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    ecc_signature_destroy(&tbs_sig);
    return result;
}

static int produce_device_cert(X509_CERT_INFO* x509_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };
    RIOT_STATUS status;
    RIOT_ECC_SIGNATURE tbs_sig;

    // Build the TBS (to be signed) region of DeviceID Certificate
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDeviceCertTBS(&der_ctx, &X509_DEVICE_TBS_DATA, &x509_info->device_id_pub, (uint8_t*)&x509_info->ca_root_pub, sizeof(x509_info->ca_root_pub)) != 0)
    {
        LogError("Failure: X509GetDeviceCertTBS");
        result = MU_FAILURE;
    }
    // Sign the DeviceID Certificate's TBS region
    else if ((status = RiotCrypt_Sign(&tbs_sig, der_ctx.Buffer, der_ctx.Position, &x509_info->device_id_priv)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if (X509MakeDeviceCert(&der_ctx, &tbs_sig) != 0)
    {
        LogError("Failure: X509MakeDeviceCert");
        result = MU_FAILURE;
    }
    else
    {
        x509_info->device_signed_length = sizeof(x509_info->device_signed_pem);
        if (DERtoPEM(&der_ctx, CERT_TYPE, x509_info->device_signed_pem, &x509_info->device_signed_length) != 0)
        {
            LogError("Failure: DERtoPEM return invalid value.");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    ecc_signature_destroy(&tbs_sig);
    return result;
}

static int produce_alias_key_cert(X509_CERT_INFO* x509_info, DERBuilderContext* cert_ctx)
{
    int result;
    x509_info->alias_cert_length = sizeof(x509_info->alias_cert_pem);
    if (DERtoPEM(cert_ctx, CERT_TYPE, x509_info->alias_cert_pem, &x509_info->alias_cert_length) != 0)
    {
        LogError("Failure: DERtoPEM return invalid value.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int produce_device_id_public(X509_CERT_INFO* x509_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };

    // Copy DeviceID Public
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDEREccPub(&der_ctx, x509_info->device_id_pub) != 0)
    {
        LogError("Failure: X509GetDEREccPub returned invalid status.");
        result = MU_FAILURE;
    }
    else
    {
        x509_info->device_id_length = sizeof(x509_info->device_id_public_pem);
        if (DERtoPEM(&der_ctx, PUBLICKEY_TYPE, x509_info->device_id_public_pem, &x509_info->device_id_length) != 0)
        {
            LogError("Failure: DERtoPEM returned invalid status.");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int produce_alias_key_pair(X509_CERT_INFO* x509_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };

    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDEREcc(&der_ctx, x509_info->alias_key_pub, x509_info->alias_key_priv) != 0)
    {
        LogError("Failure: X509GetDEREcc returned invalid status.");
        result = MU_FAILURE;
    }
    else
    {
        x509_info->alias_key_length = sizeof(x509_info->alias_priv_key_pem);
        if (DERtoPEM(&der_ctx, ECC_PRIVATEKEY_TYPE, x509_info->alias_priv_key_pem, &x509_info->alias_key_length) != 0)
        {
            LogError("Failure: DERtoPEM returned invalid status.");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int process_riot_key_info(X509_CERT_INFO* x509_info)
{
    int result;
    RIOT_STATUS status;

    if (g_digest_initialized == 0)
    {
        LogError("Failure: secure_device_init was not called.");
        result = MU_FAILURE;
    }
    else if ((status = RiotCrypt_Hash(g_digest, RIOT_DIGEST_LENGTH, g_CDI, RIOT_DIGEST_LENGTH)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Hash returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if ((status = RiotCrypt_DeriveEccKey(&x509_info->device_id_pub, &x509_info->device_id_priv,
        g_digest, RIOT_DIGEST_LENGTH, (const uint8_t*)RIOT_LABEL_IDENTITY, lblSize(RIOT_LABEL_IDENTITY))) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_DeriveEccKey returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    // Combine CDI and FWID, result in digest
    else if ((status = RiotCrypt_Hash2(g_digest, RIOT_DIGEST_LENGTH, g_digest, RIOT_DIGEST_LENGTH, firmware_id, RIOT_DIGEST_LENGTH)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Hash2 returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    // Derive Alias key pair from CDI and FWID
    else if ((status = RiotCrypt_DeriveEccKey(&x509_info->alias_key_pub, &x509_info->alias_key_priv,
        g_digest, RIOT_DIGEST_LENGTH, (const uint8_t*)RIOT_LABEL_ALIAS, lblSize(RIOT_LABEL_ALIAS))) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_DeriveEccKey returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else
    {
        if (produce_device_id_public(x509_info) != 0)
        {
            LogError("Failure: produce_device_id_public returned invalid result.");
            result = MU_FAILURE;
        }
        else if (produce_alias_key_pair(x509_info) != 0)
        {
            LogError("Failure: produce_alias_key_pair returned invalid result.");
            result = MU_FAILURE;
        }
        else
        {
            DERBuilderContext cert_ctx = { 0 };
            uint8_t cert_buffer[DER_MAX_TBS] = { 0 };
            RIOT_ECC_SIGNATURE tbs_sig;

            // Build the TBS (to be signed) region of Alias Key Certificate
            DERInitContext(&cert_ctx, cert_buffer, DER_MAX_TBS);
            if (X509GetAliasCertTBS(&cert_ctx, &X509_ALIAS_TBS_DATA, &x509_info->alias_key_pub, &x509_info->device_id_pub,
                firmware_id, RIOT_DIGEST_LENGTH) != 0)
            {
                LogError("Failure: X509GetAliasCertTBS returned invalid status.");
                result = MU_FAILURE;
            }
            else if ((status = RiotCrypt_Sign(&tbs_sig, cert_ctx.Buffer, cert_ctx.Position, &x509_info->device_id_priv)) != RIOT_SUCCESS)
            {
                LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
                result = MU_FAILURE;
            }
            else if (X509MakeAliasCert(&cert_ctx, &tbs_sig) != 0)
            {
                LogError("Failure: X509MakeAliasCert returned invalid status.");
                result = MU_FAILURE;
            }
            else if (produce_alias_key_cert(x509_info, &cert_ctx) != 0)
            {
                LogError("Failure: producing alias cert.");
                result = MU_FAILURE;
            }
            else if (generate_root_ca_info(x509_info) != 0)
            {
                LogError("Failure: producing root ca.");
                result = MU_FAILURE;
            }
            else if (produce_device_cert(x509_info) != 0)
            {
                LogError("Failure: producing device certificate.");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            ecc_signature_destroy(&tbs_sig);
        }
    }
    return result;
}

static void generate_subject_name()
{
    char device_guid[DEVICE_GUID_SIZE];
    if (UniqueId_Generate(device_guid, DEVICE_GUID_SIZE) != UNIQUEID_OK)
    {
        unsigned char rand_buff[RANDOM_BUFFER_LENGTH];
        for (size_t index = 0; index < RANDOM_BUFFER_LENGTH; index++)
        {
            rand_buff[index] = (unsigned char)gb_rand();
        }

        strcpy(g_device_name, DEVICE_PREFIX);
        char* insert_pos = g_device_name + strlen(g_device_name);
        for (size_t index = 0; index < RANDOM_BUFFER_LENGTH; index++)
        {
            int pos = sprintf(insert_pos, "%x", rand_buff[index]);
            insert_pos += pos;
        }
    }
    else
    {
        strcpy(g_device_name, DEVICE_PREFIX);
        char* insert_pos = g_device_name + strlen(g_device_name);
        for (size_t index = 0; index < RANDOM_BUFFER_LENGTH; index++)
        {
            int pos = sprintf(insert_pos, "%x", device_guid[index]);
            insert_pos += pos;
        }
    }
}

static void generate_keys()
{
    if (g_digest_initialized == 0)
    {
        uint8_t random_digest[RIOT_DIGEST_LENGTH] = { 0 };

        srand((unsigned int)time(NULL));
        for (size_t index = 0; index < DICE_UDS_LENGTH; index++)
        {
            g_uds_seed[index] = (unsigned char)gb_rand();
        }
        for (size_t index = 0; index < RIOT_DIGEST_LENGTH; index++)
        {
            firmware_id[index] = (unsigned char)gb_rand();
            g_CDI[index] = (unsigned char)gb_rand();
            random_digest[index] = (unsigned char)gb_rand();
        }
        DiceSHA256(g_uds_seed, DICE_UDS_LENGTH, g_digest);
        DiceSHA256_2(g_digest, RIOT_DIGEST_LENGTH, random_digest, RIOT_DIGEST_LENGTH, g_CDI);
        g_digest_initialized = 1;
    }
}

void initialize_device()
{
    generate_keys();
    generate_subject_name();
}

X509_INFO_HANDLE x509_info_create()
{
    X509_CERT_INFO* result;
    /* Codes_SRS_HSM_CLIENT_TPM_07_002: [ On success hsm_client_tpm_create shall allocate a new instance of the secure device tpm interface. ] */
    result = malloc(sizeof(X509_CERT_INFO));
    if (result == NULL)
    {
        /* Codes_SRS_HSM_CLIENT_TPM_07_001: [ If any failure is encountered hsm_client_tpm_create shall return NULL ] */
        printf("Failure: malloc X509_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(X509_CERT_INFO));
        strcpy(result->device_name, g_device_name);
        X509_ALIAS_TBS_DATA.SubjectCommon = result->device_name;

        if (process_riot_key_info(result) != 0)
        {
            free(result);
            result = NULL;
        }

    }
    return result;
}

void x509_info_destroy(X509_INFO_HANDLE handle)
{
    if (handle != NULL)
    {
        mbedtls_ecp_point_free(&handle->ca_root_pub);
        mbedtls_mpi_free(&handle->ca_root_priv);

        mbedtls_ecp_point_free(&handle->device_id_pub);
        mbedtls_mpi_free(&handle->device_id_priv);
        
        mbedtls_ecp_point_free(&handle->alias_key_pub);
        mbedtls_mpi_free(&handle->alias_key_priv);

        free(handle);
    }
}

static void ecc_signature_destroy(RIOT_ECC_SIGNATURE* tbs_sig)
{
    mbedtls_mpi_free(&tbs_sig->r);
    mbedtls_mpi_free(&tbs_sig->s);
}

const char* x509_info_get_cert(X509_INFO_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = handle->alias_cert_pem;
    }
    return result;
}

const char* x509_info_get_key(X509_INFO_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = handle->alias_priv_key_pem;
    }
    return result;
}

const char* x509_info_get_cn(X509_INFO_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        result = handle->device_name;
    }
    return result;
}
