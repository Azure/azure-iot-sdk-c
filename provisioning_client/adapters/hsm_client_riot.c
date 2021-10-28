// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "hsm_client_riot.h"
#include "hsm_client_data.h"

#include "RIoT.h"
#include "RiotCrypt.h"
#include "derenc.h"
#include "x509bldr.h"
#include "DiceSha256.h"

#define RIOT_SIGNER_NAME            "riot-signer-core"
#define RIOT_COMMON_NAME            "riot-device-cert"
#define RIOT_CA_CERT_NAME           "riot-root"

// Note that even though digest lengths are equivalent here, (and on most
// devices this will be the case) there is no requirement that DICE and RIoT
// use the same one-way function/digest length.
#define DICE_DIGEST_LENGTH          RIOT_DIGEST_LENGTH

// Note also that there is no requirement on the UDS length for a device.
// A 256-bit UDS is recommended but this size may vary among devices.
#define DICE_UDS_LENGTH             0x20

// Size, in bytes, returned when the required certificate buffer size is
// requested.  For this emulator the actual size (~552 bytes) is static,
// based on the contents of the x509TBSData struct (the fiels don't vary).
// As x509 data varies so will, obviously, the overall cert length. For now,
// just pick a reasonable minimum buffer size and worry about this later.
#define REASONABLE_MIN_CERT_SIZE    DER_MAX_TBS

// Emulator specific
// Random (i.e., simulated) RIoT Core "measurement"
static uint8_t RAMDOM_DIGEST[DICE_DIGEST_LENGTH] = {
    0xb5, 0x85, 0x94, 0x93, 0x66, 0x1e, 0x2e, 0xae,
    0x96, 0x77, 0xc5, 0x5d, 0x59, 0x0b, 0x92, 0x94,
    0xe0, 0x94, 0xab, 0xaf, 0xd7, 0x40, 0x78, 0x7e,
    0x05, 0x0d, 0xfe, 0x6d, 0x85, 0x90, 0x53, 0xa0 };

static unsigned char firmware_id[RIOT_DIGEST_LENGTH] = {
    0x6B, 0xE9, 0xB1, 0x84, 0xC9, 0x37, 0xC2, 0x8E,
    0x12, 0x2E, 0xEE, 0x51, 0x2B, 0x68, 0xEA, 0x8E,
    0x00, 0xC3, 0xDD, 0x15, 0x9E, 0xA4, 0xE8, 0x5E,
    0x84, 0xCB, 0xA9, 0x66, 0xF4, 0x46, 0xCD, 0x4E };

// The static data fields that make up the x509 "to be signed" region
//static RIOT_X509_TBS_DATA X509_TBS_DATA = { { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E },
//"RIoT Core", "MSR_TEST", "US", "170101000000Z", "370101000000Z", "RIoT Device", "MSR_TEST", "US" };

// The static data fields that make up the Alias Cert "to be signed" region
static RIOT_X509_TBS_DATA X509_ALIAS_TBS_DATA = {
    { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E }, RIOT_SIGNER_NAME, "MSR_TEST", "US",
    "170101000000Z", "370101000000Z", RIOT_COMMON_NAME, "MSR_TEST", "US" };

// The static data fields that make up the DeviceID Cert "to be signed" region
static RIOT_X509_TBS_DATA X509_DEVICE_TBS_DATA = {
    { 0x0E, 0x0D, 0x0C, 0x0B, 0x0A }, RIOT_CA_CERT_NAME, "MSR_TEST", "US",
    "170101000000Z", "370101000000Z", RIOT_SIGNER_NAME, "MSR_TEST", "US" };

// The static data fields that make up the "root signer" Cert
static RIOT_X509_TBS_DATA X509_ROOT_TBS_DATA = {
    { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E }, RIOT_CA_CERT_NAME, "MSR_TEST", "US",
    "170101000000Z", "370101000000Z", RIOT_CA_CERT_NAME, "MSR_TEST", "US" };

#define DER_ECC_KEY_MAX     0x80
#define DER_ECC_PUB_MAX     0x60

#if !defined(RIOTSECP256R1)
#error "Must define RIOTSECP256R1 - NIST 256 Curve is only supported"
#endif

static int g_digest_initialized = 0;
static uint8_t g_digest[DICE_DIGEST_LENGTH] = { 0 };
static unsigned char g_uds_seed[DICE_UDS_LENGTH] = {
    0x54, 0x10, 0x5D, 0x2E, 0xCD, 0x07, 0xF9, 0x01,
    0x99, 0xB3, 0x95, 0xC7, 0x42, 0x61, 0xA0, 0x8C,
    0xFF, 0x27, 0x1A, 0x0D, 0xF6, 0x6F, 0x1F, 0xE0,
    0x00, 0x34, 0xBB, 0x11, 0xF7, 0x98, 0x9A, 0x12 };
static uint8_t g_CDI[DICE_DIGEST_LENGTH] = {
    0x91, 0x75, 0xDB, 0xEE, 0x90, 0xC4, 0xE1, 0xE3,
    0x74, 0x47, 0x2C, 0x8A, 0x55, 0x3F, 0xD2, 0xB8,
    0xE9, 0x79, 0xEE, 0xF1, 0x62, 0xF8, 0x64, 0xDA,
    0x50, 0x69, 0x4B, 0x3E, 0x5A, 0x1E, 0x3A, 0x6E };

typedef enum CERTIFICATE_SIGNING_TYPE_TAG
{
    type_self_sign,
    type_riot_csr,
    type_root_signed
} CERTIFICATE_SIGNING_TYPE;

typedef struct HSM_CLIENT_X509_INFO_TAG
{
    // In Riot these are call the device Id pub and pri
    RIOT_ECC_PUBLIC     device_id_pub;
    RIOT_ECC_PRIVATE    device_id_priv;

    RIOT_ECC_PUBLIC     alias_key_pub;
    RIOT_ECC_PRIVATE    alias_key_priv;

    RIOT_ECC_PUBLIC     ca_root_pub;
    RIOT_ECC_PRIVATE    ca_root_priv;

    char* certificate_common_name;

    uint32_t device_id_length;
    char device_id_public_pem[DER_MAX_PEM];

    uint32_t device_signed_length;
    char device_signed_pem[DER_MAX_PEM];

    uint32_t alias_key_length;
    char alias_priv_key_pem[DER_MAX_PEM];

    uint32_t alias_cert_length;
    char alias_cert_pem[DER_MAX_PEM];

    uint32_t root_ca_length;
    char root_ca_pem[DER_MAX_PEM];

    uint32_t root_ca_priv_length;
    char root_ca_priv_pem[DER_MAX_PEM];

} HSM_CLIENT_X509_INFO;

static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    hsm_client_riot_create,
    hsm_client_riot_destroy,
    hsm_client_riot_get_certificate,
    hsm_client_riot_get_alias_key,
    hsm_client_riot_get_common_name
};

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

// The "root" signing key. This is intended for DEVELOPMENT PURPOSES ONLY.
// This key is used to sign the DeviceID certificate, the certificiate for
// this "root" key represents the "trusted" CA for the developer-mode
// server(s). Again, this is for DEVELOPMENT PURPOSES ONLY and (obviously)
// provides no meaningful security whatsoever, NEVER use this in production.
static void get_riot_root_dev_key(HSM_CLIENT_X509_INFO* x509_info)
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

static int generate_root_ca_info(HSM_CLIENT_X509_INFO* riot_info, RIOT_ECC_SIGNATURE* tbs_sig)
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
    get_riot_root_dev_key(riot_info);

    if (X509GetRootCertTBS(&der_ctx, &X509_ROOT_TBS_DATA, &riot_info->ca_root_pub) != 0)
    {
        LogError("Failure: X509GetRootCertTBS");
        result = MU_FAILURE;
    }
    // Sign the CA_Root Certificate's TBS region
    else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &riot_info->ca_root_priv)) != RIOT_SUCCESS)
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
        riot_info->root_ca_priv_length = sizeof(riot_info->root_ca_priv_pem);
        riot_info->root_ca_length = sizeof(riot_info->root_ca_pem);

        if (DERtoPEM(&der_ctx, CERT_TYPE, riot_info->root_ca_pem, &riot_info->root_ca_length) != 0)
        {
            LogError("Failure: DERtoPEM return invalid value.");
            result = MU_FAILURE;
        }
        else if (X509GetDEREcc(&der_pri_ctx, riot_info->ca_root_pub, riot_info->ca_root_priv) != 0)
        {
            LogError("Failure: X509GetDEREcc returned invalid status.");
            result = MU_FAILURE;
        }
        else if (DERtoPEM(&der_pri_ctx, ECC_PRIVATEKEY_TYPE, riot_info->root_ca_priv_pem, &riot_info->root_ca_priv_length) != 0)
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

static int produce_device_cert(HSM_CLIENT_X509_INFO* riot_info, RIOT_ECC_SIGNATURE* tbs_sig, CERTIFICATE_SIGNING_TYPE signing_type)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };
    RIOT_STATUS status;

    if (signing_type == type_self_sign)
    {
        // Build the TBS (to be signed) region of DeviceID Certificate
        DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
        if (X509GetDeviceCertTBS(&der_ctx, &X509_DEVICE_TBS_DATA, &riot_info->device_id_pub, NULL, 0) != 0)
        {
            LogError("Failure: X509GetDeviceCertTBS");
            result = MU_FAILURE;
        }
        // Sign the DeviceID Certificate's TBS region
        else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &riot_info->device_id_priv)) != RIOT_SUCCESS)
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
            result = 0;
        }
    }
    else if (signing_type == type_riot_csr)
    {
        DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
        if (X509GetDERCsrTbs(&der_ctx, &X509_ALIAS_TBS_DATA, &riot_info->device_id_pub) != 0)
        {
            LogError("Failure: X509GetDERCsrTbs");
            result = MU_FAILURE;
        }
        // Sign the Alias Key Certificate's TBS region
        else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &riot_info->device_id_priv)) == RIOT_SUCCESS)
        {
            LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
            result = MU_FAILURE;
        }
        // Create CSR for DeviceID
        else if (X509GetDERCsr(&der_ctx, tbs_sig) != 0)
        {
            LogError("Failure: X509GetDERCsr");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    // Root signed
    else
    {
        // Generating "root"-signed DeviceID certificate
        DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
        if (X509GetDeviceCertTBS(&der_ctx, &X509_DEVICE_TBS_DATA, &riot_info->device_id_pub, (uint8_t*)&riot_info->ca_root_pub, sizeof(riot_info->ca_root_pub)) != 0)
        {
            LogError("Failure: X509GetDeviceCertTBS");
            result = MU_FAILURE;
        }
        // Sign the DeviceID Certificate's TBS region
        else if ((status = RiotCrypt_Sign(tbs_sig, der_ctx.Buffer, der_ctx.Position, &riot_info->ca_root_priv)) != RIOT_SUCCESS)
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
            result = 0;
        }
    }

    if (result == 0)
    {
        riot_info->device_signed_length = sizeof(riot_info->device_signed_pem);
        if (DERtoPEM(&der_ctx, CERT_TYPE, riot_info->device_signed_pem, &riot_info->device_signed_length) != 0)
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

static int produce_alias_key_cert(HSM_CLIENT_X509_INFO* riot_info, DERBuilderContext* cert_ctx)
{
    int result;
    riot_info->alias_cert_length = sizeof(riot_info->alias_cert_pem);
    if (DERtoPEM(cert_ctx, CERT_TYPE, riot_info->alias_cert_pem, &riot_info->alias_cert_length) != 0)
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

static int produce_alias_key_pair(HSM_CLIENT_X509_INFO* riot_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };

    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDEREcc(&der_ctx, riot_info->alias_key_pub, riot_info->alias_key_priv) != 0)
    {
        LogError("Failure: X509GetDEREcc returned invalid status.");
        result = MU_FAILURE;
    }
    else
    {
        riot_info->alias_key_length = sizeof(riot_info->alias_priv_key_pem);
        if (DERtoPEM(&der_ctx, ECC_PRIVATEKEY_TYPE, riot_info->alias_priv_key_pem, &riot_info->alias_key_length) != 0)
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

static int produce_device_id_public(HSM_CLIENT_X509_INFO* riot_info)
{
    int result;
    uint8_t der_buffer[DER_MAX_TBS] = { 0 };
    DERBuilderContext der_ctx = { 0 };

    // Copy DeviceID Public
    DERInitContext(&der_ctx, der_buffer, DER_MAX_TBS);
    if (X509GetDEREccPub(&der_ctx, riot_info->device_id_pub) != 0)
    {
        LogError("Failure: X509GetDEREccPub returned invalid status.");
        result = MU_FAILURE;
    }
    else
    {
        riot_info->device_id_length = sizeof(riot_info->device_id_public_pem);
        if (DERtoPEM(&der_ctx, PUBLICKEY_TYPE, riot_info->device_id_public_pem, &riot_info->device_id_length) != 0)
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

static int process_riot_key_info(HSM_CLIENT_X509_INFO* riot_info)
{
    int result;
    RIOT_STATUS status;

    // Don't use CDI directly
    if (g_digest_initialized == 0)
    {
        LogError("Failure: secure_device_init was not called.");
        result = MU_FAILURE;
    }
    else if (X509_ALIAS_TBS_DATA.SubjectCommon == NULL || strlen(X509_ALIAS_TBS_DATA.SubjectCommon) == 0)
    {
        LogError("Failure: The AX509_ALIAS_TBS_DATA.SubjectCommon is not entered");
        result = MU_FAILURE;
    }
    else if ((status = RiotCrypt_Hash(g_digest, RIOT_DIGEST_LENGTH, g_CDI, DICE_DIGEST_LENGTH)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Hash returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else if ((status = RiotCrypt_DeriveEccKey(&riot_info->device_id_pub, &riot_info->device_id_priv,
        g_digest, DICE_DIGEST_LENGTH, (const uint8_t*)RIOT_LABEL_IDENTITY, lblSize(RIOT_LABEL_IDENTITY))) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_DeriveEccKey returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    // Combine CDI and FWID, result in digest
    else if ((status = RiotCrypt_Hash2(g_digest, DICE_DIGEST_LENGTH, g_digest, DICE_DIGEST_LENGTH, firmware_id, RIOT_DIGEST_LENGTH)) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_Hash2 returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    // Derive Alias key pair from CDI and FWID
    else if ((status = RiotCrypt_DeriveEccKey(&riot_info->alias_key_pub, &riot_info->alias_key_priv,
        g_digest, RIOT_DIGEST_LENGTH, (const uint8_t*)RIOT_LABEL_ALIAS, lblSize(RIOT_LABEL_ALIAS))) != RIOT_SUCCESS)
    {
        LogError("Failure: RiotCrypt_DeriveEccKey returned invalid status %d.", status);
        result = MU_FAILURE;
    }
    else
    {
        if (produce_device_id_public(riot_info) != 0)
        {
            LogError("Failure: produce_device_id_public returned invalid result.");
            result = MU_FAILURE;
        }
        else if (produce_alias_key_pair(riot_info) != 0)
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
            if (X509GetAliasCertTBS(&cert_ctx, &X509_ALIAS_TBS_DATA, &riot_info->alias_key_pub, &riot_info->device_id_pub,
                firmware_id, RIOT_DIGEST_LENGTH) != 0)
            {
                LogError("Failure: X509GetAliasCertTBS returned invalid status.");
                result = MU_FAILURE;
            }
            else if ((status = RiotCrypt_Sign(&tbs_sig, cert_ctx.Buffer, cert_ctx.Position, &riot_info->device_id_priv)) != RIOT_SUCCESS)
            {
                LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
                result = MU_FAILURE;
            }
            else if (X509MakeAliasCert(&cert_ctx, &tbs_sig) != 0)
            {
                LogError("Failure: X509MakeAliasCert returned invalid status.");
                result = MU_FAILURE;
            }
            else if (produce_alias_key_cert(riot_info, &cert_ctx) != 0)
            {
                LogError("Failure: producing alias cert.");
                result = MU_FAILURE;
            }
            else if (generate_root_ca_info(riot_info, &tbs_sig) != 0)
            {
                LogError("Failure: producing root ca.");
                result = MU_FAILURE;
            }
            else if (produce_device_cert(riot_info, &tbs_sig, type_root_signed) != 0)
            {
                LogError("Failure: producing device certificate.");
                result = MU_FAILURE;
            }
            else if (mallocAndStrcpy_s(&riot_info->certificate_common_name, X509_ALIAS_TBS_DATA.SubjectCommon) != 0)
            {
                LogError("Failure: attempting to get common name");
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

int hsm_client_x509_init(void)
{
    // Only initialize one time
    if (g_digest_initialized == 0)
    {
        // Derive DeviceID key pair from CDI
        DiceSHA256(g_uds_seed, DICE_UDS_LENGTH, g_digest);

        // Derive CDI based on UDS and RIoT Core "measurement"
        DiceSHA256_2(g_digest, DICE_DIGEST_LENGTH, RAMDOM_DIGEST, DICE_DIGEST_LENGTH, g_CDI);
        g_digest_initialized = 1;
    }
    return 0;
}

void hsm_client_x509_deinit(void)
{
}

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface(void)
{
    return &x509_interface;
}

HSM_CLIENT_HANDLE hsm_client_riot_create(void)
{
    HSM_CLIENT_X509_INFO* result;
    result = malloc(sizeof(HSM_CLIENT_X509_INFO) );
    if (result == NULL)
    {
        LogError("Failure: malloc HSM_CLIENT_X509_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(HSM_CLIENT_X509_INFO));
        if (process_riot_key_info(result) != 0)
        {
            free(result);
            result = NULL;
        }
    }
    return result;
}

void hsm_client_riot_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;
        free(x509_client->certificate_common_name);
        x509_cert_free(&x509_client->ca_root_pub, &x509_client->ca_root_priv);
        x509_cert_free(&x509_client->device_id_pub, &x509_client->device_id_priv);
        x509_cert_free(&x509_client->alias_key_pub, &x509_client->alias_key_priv);

        free(x509_client);
    }
}

char* hsm_client_riot_get_certificate(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        size_t total_len = x509_client->alias_cert_length + x509_client->device_signed_length;

        result = (char*)malloc(total_len + 1);
        if (result == NULL)
        {
            LogError("Failed to allocate cert buffer.");
        }
        else
        {
            size_t offset = 0;
            memset(result, 0, total_len + 1);
            memcpy(result, x509_client->alias_cert_pem, x509_client->alias_cert_length);
            offset += x509_client->alias_cert_length;

            memcpy(result + offset, x509_client->device_signed_pem, x509_client->device_signed_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_alias_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if ((result = (char*)malloc(x509_client->alias_key_length+1)) == NULL)
        {
            LogError("Failure allocating registration id.");
        }
        else
        {
            memset(result, 0, x509_client->alias_key_length+1);
            memcpy(result, x509_client->alias_priv_key_pem, x509_client->alias_key_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_device_cert(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if ((result = (char*)malloc(x509_client->device_id_length+1)) == NULL)
        {
            LogError("Failure allocating registration id.");
        }
        else
        {
            memset(result, 0, x509_client->device_id_length+1);
            memcpy(result, x509_client->device_id_public_pem, x509_client->device_id_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_signer_cert(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if ((result = (char*)malloc(x509_client->device_signed_length + 1)) == NULL)
        {
            LogError("Failure allocating registration id.");
        }
        else
        {
            memset(result, 0, x509_client->device_signed_length + 1);
            memcpy(result, x509_client->device_signed_pem, x509_client->device_signed_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_root_cert(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if ((result = (char*)malloc(x509_client->root_ca_length + 1)) == NULL)
        {
            LogError("Failure allocating registration id.");
        }
        else
        {
            memset(result, 0, x509_client->root_ca_length + 1);
            memcpy(result, x509_client->root_ca_pem, x509_client->root_ca_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_root_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if ((result = (char*)malloc(x509_client->root_ca_priv_length + 1)) == NULL)
        {
            LogError("Failure allocating registration id.");
        }
        else
        {
            memset(result, 0, x509_client->root_ca_priv_length + 1);
            memcpy(result, x509_client->root_ca_priv_pem, x509_client->root_ca_priv_length);
        }
    }
    return result;
}

char* hsm_client_riot_get_common_name(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        LogError("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        HSM_CLIENT_X509_INFO* x509_client = (HSM_CLIENT_X509_INFO*)handle;

        if (mallocAndStrcpy_s(&result, x509_client->certificate_common_name) != 0)
        {
            LogError("Failure allocating common name.");
            result = NULL;
        }
    }
    return result;
}

char* hsm_client_riot_create_leaf_cert(HSM_CLIENT_HANDLE handle, const char* common_name)
{
    char* result;

    // The static data fields that make up the DeviceID Cert "to be signed" region
    RIOT_X509_TBS_DATA LEAF_CERT_TBS_DATA = {
        { 0x5E, 0x4D, 0x3C, 0x2B, 0x1A }, RIOT_CA_CERT_NAME, "MSR_TEST", "US",
        "170101000000Z", "370101000000Z", "", "MSR_TEST", "US" };

    if (handle == NULL || common_name == NULL)
    {
        LogError("invalid parameter specified.");
        result = NULL;
    }
    else
    {
        RIOT_STATUS status;
        uint8_t leaf_buffer[DER_MAX_TBS] = { 0 };
        DERBuilderContext leaf_ctx = { 0 };
        RIOT_ECC_PUBLIC     leaf_id_pub;
        RIOT_ECC_SIGNATURE tbs_sig = { 0 };

        HSM_CLIENT_X509_INFO* riot_info = (HSM_CLIENT_X509_INFO*)handle;

        LEAF_CERT_TBS_DATA.SubjectCommon = common_name;

        DERInitContext(&leaf_ctx, leaf_buffer, DER_MAX_TBS);
        if (X509GetAliasCertTBS(&leaf_ctx, &LEAF_CERT_TBS_DATA, &leaf_id_pub, &riot_info->device_id_pub, firmware_id, RIOT_DIGEST_LENGTH) != 0)
        {
            LogError("Failure: X509GetDeviceCertTBS");
            result = NULL;
        }
        else if ((status = RiotCrypt_Sign(&tbs_sig, leaf_ctx.Buffer, leaf_ctx.Position, &riot_info->ca_root_priv)) != RIOT_SUCCESS)
        {
            LogError("Failure: RiotCrypt_Sign returned invalid status %d.", status);
            result = NULL;
        }
        else if (X509MakeDeviceCert(&leaf_ctx, &tbs_sig) != 0)
        {
            LogError("Failure: X509MakeDeviceCert");
            result = NULL;
        }
        else if ((result = (char*)malloc(DER_MAX_PEM + 1)) == NULL)
        {
            LogError("Failure allocating leaf cert");
        }
        else
        {
            memset(result, 0, DER_MAX_PEM + 1);
            uint32_t leaf_len = DER_MAX_PEM;
            if (DERtoPEM(&leaf_ctx, CERT_TYPE, result, &leaf_len) != 0)
            {
                LogError("Failure: DERtoPEM return invalid value.");
                free(result);
                result = NULL;
            }
        }
        ecc_signature_destroy(&tbs_sig);
    }
    return result;
}
