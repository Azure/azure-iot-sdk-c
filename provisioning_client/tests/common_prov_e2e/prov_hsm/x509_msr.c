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

#if !defined(RIOTSECP256R1)
#error "Must define RIOTSECP256R1 - NIST 256 Curve is only supported"
#endif

// Free the mbedtls_mpi members of the signature
static void ecc_signature_destroy(RIOT_ECC_SIGNATURE* tbs_sig)
{
    mbedtls_mpi_free(&tbs_sig->r);
    mbedtls_mpi_free(&tbs_sig->s);
}

static void x509_cert_free(RIOT_ECC_PUBLIC* pub, RIOT_ECC_PRIVATE* priv)
{
    mbedtls_ecp_point_free(pub);
    mbedtls_mpi_free(priv);
}

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

// The "root" signing key. This is intended for DEVELOPMENT PURPOSES ONLY.
// This key is used to sign the DeviceID certificate, the certificiate for
// this "root" key represents the "trusted" CA for the developer-mode
// server(s). Again, this is for DEVELOPMENT PURPOSES ONLY and (obviously)
// provides no meaningful security whatsoever, NEVER use this in production.
static void get_riot_root_dev_key(X509_CERT_INFO* x509_info) 
{
    // CA_Root_Pub X coordinates random bits
    uint8_t rootX[RIOT_COORDMAX] = {
        0xeb, 0x9c, 0xfc, 0xc8, 0x49, 0x94, 0xd3, 0x50, 0xa7, 0x1f, 0x9d, 0xc5,
        0x09, 0x3d, 0xd2, 0xfe, 0xb9, 0x48, 0x97, 0xf4, 0x95, 0xa5, 0x5d, 0xec,
        0xc9, 0x0f, 0x52, 0xa1, 0x26, 0x5a, 0xab, 0x69 };
    // CA_Root_Pub Y coordinates random bits
    uint8_t rootY[RIOT_COORDMAX] = {
        0x7d, 0xce, 0xb1, 0x62, 0x39, 0xf8, 0x3c, 0xd5, 0x9a, 0xad, 0x9e, 0x05,
        0xb1, 0x4f, 0x70, 0xa2, 0xfa, 0xd4, 0xfb, 0x04, 0xe5, 0x37, 0xd2, 0x63,
        0x9a, 0x46, 0x9e, 0xfd, 0xb0, 0x5b, 0x1e, 0xdf };
    // CA_Root_Priv random bits
    uint8_t rootD[RIOT_COORDMAX] = {
        0xe3, 0xe7, 0xc7, 0x13, 0x57, 0x3f, 0xd9, 0xc8, 0xb8, 0xe1, 0xea, 0xf4,
        0x53, 0xf1, 0x56, 0x15, 0x02, 0xf0, 0x71, 0xc0, 0x53, 0x49, 0xc8, 0xda,
        0xe6, 0x26, 0xa9, 0x0b, 0x17, 0x88, 0xe5, 0x70 };

    // Simulator only: We need to populate the root key.
    // The following memset's are unnecessary in a simulated environment
    // in the wild it's good to stay in habit of clearing potential 
    // sensitive data.
    mbedtls_mpi_read_binary(&x509_info->ca_root_pub.X, rootX, RIOT_COORDMAX);
    memset(rootX, 0, sizeof(rootX));
    mbedtls_mpi_read_binary(&x509_info->ca_root_pub.Y, rootY, RIOT_COORDMAX);
    memset(rootY, 0, sizeof(rootY));
    mbedtls_mpi_lset(&x509_info->ca_root_pub.Z, 1);
    mbedtls_mpi_read_binary(&x509_info->ca_root_priv, rootD, RIOT_COORDMAX);
    memset(rootD, 0, sizeof(rootD));
}

static int generate_root_ca_info(X509_CERT_INFO* x509_info, RIOT_ECC_SIGNATURE* tbs_sig)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };
    DERBuilderContext der_pri_ctx = { 0 };
    RIOT_STATUS status;

    // Build the TBS (to be signed) region of CA_Root Certificate
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    DERInitContext(&der_pri_ctx, der_buffer, DER_MAX_TBS);

    // Generate the CA_Root using the development key
    get_riot_root_dev_key(x509_info);

    if (X509GetRootCertTBS(&der_ctx, &X509_ROOT_TBS_DATA, &x509_info->ca_root_pub) != 0)
    {
        LogError("Failure: X509GetRootCertTBS");
        result = MU_FAILURE;
    }
    // Sign the CA_Root Certificate's TBS region
    else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &x509_info->ca_root_priv)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if (X509MakeRootCert(&der_ctx, tbs_sig) != 0)
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
    return result;
}

static int produce_device_cert(X509_CERT_INFO* x509_info, RIOT_ECC_SIGNATURE* tbs_sig)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };
    RIOT_STATUS status;

    // Build the TBS (to be signed) region of DeviceID Certificate
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDeviceCertTBS(&der_ctx, &X509_DEVICE_TBS_DATA, &x509_info->device_id_pub, (uint8_t*)&x509_info->ca_root_pub, sizeof(x509_info->ca_root_pub)) != 0)
    {
        LogError("Failure: X509GetDeviceCertTBS");
        result = MU_FAILURE;
    }
    // Sign the DeviceID Certificate's TBS region
    else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &x509_info->device_id_priv)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if (X509MakeDeviceCert(&der_ctx, tbs_sig) != 0)
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
            RIOT_ECC_SIGNATURE tbs_sig = { 0 };

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
            else if (generate_root_ca_info(x509_info, &tbs_sig) != 0)
            {
                LogError("Failure: producing root ca.");
                result = MU_FAILURE;
            }
            else if (produce_device_cert(x509_info, &tbs_sig) != 0)
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

static void generate_subject_name(void)
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

static void generate_keys(void)
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

void initialize_device(void)
{
    generate_keys();
    generate_subject_name();
}

X509_INFO_HANDLE x509_info_create(void)
{
    X509_CERT_INFO* result;
    result = malloc(sizeof(X509_CERT_INFO));
    if (result == NULL)
    {
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
        x509_cert_free(&handle->ca_root_pub, &handle->ca_root_priv);
        x509_cert_free(&handle->device_id_pub, &handle->device_id_priv);
        x509_cert_free(&handle->alias_key_pub, &handle->alias_key_priv);

        free(handle);
    }
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
