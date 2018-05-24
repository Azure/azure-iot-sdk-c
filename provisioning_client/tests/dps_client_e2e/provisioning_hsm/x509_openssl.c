// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "x509_info.h"

#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

typedef struct X509_OPENSSL_INFO_TAG
{
    bool use_ecc_type;
    char device_name[32];
    X509* x509_cert;
} X509_OPENSSL_INFO;


#define X509_VERSION            3
#define SERIAL_NUMBER           2244225
#define BIT_LENGTH              512
#define RANDOM_BUFFER_LENGTH    8

EVP_PKEY* g_evp_key = NULL;
bool g_use_ecc_type = true;
static const char* g_subject_name = NULL;

static const char* const ECCTYPE = "prime256v1";
static const char* const COMMON_NAME = "hsm_cert_example";
static const char* const DEVICE_PREFIX = "hsm_dev_";
static const char* const PRIVATE_KEY_CERT_FILE = "prov_hsm_pk.info";
static const char* const CERTIFICATE_INFO_FILE = "prov_hsm_cert.info";

static int save_key_file()
{
    int result;
    //FILE* key_file = fopen(PRIVATE_KEY_CERT_FILE, "wb");
    BIO* key_file = BIO_new_file(PRIVATE_KEY_CERT_FILE, "w");
    if (key_file == NULL)
    {
        (void)printf("Failure opening private key cert\r\n");
        result = __LINE__;
    }
    else
    {
        if (!PEM_write_bio_PrivateKey(key_file, g_evp_key, NULL, NULL, 0, NULL, NULL))
        {
            (void)printf("Failure PEM_read_PrivateKey\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        BIO_free_all(key_file);
    }
    return result;
}

static int load_key_file()
{
    int result;
    BIO* key_file = BIO_new_file(PRIVATE_KEY_CERT_FILE, "r");
    if (key_file == NULL)
    {
        result = __LINE__;
    }
    else
    {
        g_evp_key = PEM_read_bio_PrivateKey(key_file, NULL, NULL, NULL);
        if (g_evp_key == NULL)
        {
            (void)printf("Failure PEM_read_PrivateKey\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        BIO_free_all(key_file);
    }
    return result;
}

static int save_certificate_file(X509_OPENSSL_INFO* hsm_impl)
{
    int result;
    BIO* cert_file = BIO_new_file(CERTIFICATE_INFO_FILE, "w");
    if (cert_file == NULL)
    {
        (void)printf("Failure opening private key cert\r\n");
        result = __LINE__;
    }
    else
    {
        if (!PEM_write_bio_X509(cert_file, hsm_impl->x509_cert))
        {
            (void)printf("Failure PEM_read_PrivateKey\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        BIO_free_all(cert_file);
    }
    return result;
}

static int load_certificate_file(X509_OPENSSL_INFO* hsm_impl)
{
    int result;
    BIO* cert_file = BIO_new_file(CERTIFICATE_INFO_FILE, "r");
    if (cert_file == NULL)
    {
        result = __LINE__;
    }
    else
    {
        hsm_impl->x509_cert = PEM_read_bio_X509(cert_file, NULL, NULL, NULL);
        if (hsm_impl->x509_cert == NULL)
        {
            (void)printf("Failure PEM_read_PrivateKey\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        BIO_free_all(cert_file);
    }
    return result;
}

static int generate_subject_name(X509_OPENSSL_INFO* hsm_impl)
{
    int result;
    unsigned char rand_buff[RANDOM_BUFFER_LENGTH];
    if (RAND_bytes(rand_buff, RANDOM_BUFFER_LENGTH) != 1)
    {
        result = __LINE__;
    }
    else
    {
        strcpy(hsm_impl->device_name, DEVICE_PREFIX);
        char* insert_pos = hsm_impl->device_name + strlen(hsm_impl->device_name);
        for (size_t index = 0; index < RANDOM_BUFFER_LENGTH; index++)
        {
            int pos = sprintf(insert_pos, "%x", rand_buff[index]);
            insert_pos += pos;
        }
        result = 0;
    }
    return result;
}

static int create_key_pair()
{
    int result;
    if (load_key_file() != 0)
    {
        // Can't load private key
        if (g_use_ecc_type)
        {
            EC_KEY* ecc_key;
            int ecc_group = OBJ_txt2nid(ECCTYPE);

            if ((ecc_key = EC_KEY_new_by_curve_name(ecc_group)) == NULL)
            {
                (void)printf("Failure getting curve name\r\n");
                result = __LINE__;
            }
            else
            {
                EC_KEY_set_asn1_flag(ecc_key, OPENSSL_EC_NAMED_CURVE);
                if (!EC_KEY_generate_key(ecc_key))
                {
                    (void)printf("Error generating ECC key\r\n");
                    result = __LINE__;
                }
                else if ((g_evp_key = EVP_PKEY_new()) == NULL)
                {
                    (void)printf("Failed creating PKey\r\n");
                    result = __LINE__;
                }
                else if (!EVP_PKEY_assign_EC_KEY(g_evp_key, ecc_key))
                {
                    (void)printf("Error assigning ECC key to EVP_PKEY structure.\r\n");
                    EVP_PKEY_free(g_evp_key);
                    g_evp_key = NULL;
                    result = __LINE__;
                }
                else
                {
                    ecc_key = EVP_PKEY_get1_EC_KEY(g_evp_key);
                    const EC_GROUP* ecgrp = EC_KEY_get0_group(ecc_key);
                    /* ---------------------------------------------------------- *
                    * Here we print the key length, and extract the curve type.  *
                    * ---------------------------------------------------------- */
                    (void)printf("ECC Key size: %d bit\n", EVP_PKEY_bits(g_evp_key));
                    (void)printf("ECC Key type: %s\n", OBJ_nid2sn(EC_GROUP_get_curve_name(ecgrp)));

                    if (save_key_file() != 0)
                    {
                        (void)printf("Fail saving key file.\r\n");
                        EVP_PKEY_free(g_evp_key);
                        g_evp_key = NULL;
                        result = __LINE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                EC_KEY_free(ecc_key);
            }
        }
        else
        {
            RSA* rsa;
            if ((g_evp_key = EVP_PKEY_new()) == NULL)
            {
                (void)printf("Failure create new EVP key.\r\n");
                result = __LINE__;
            }
            else
            {
                if ((rsa = RSA_generate_key(BIT_LENGTH, RSA_F4, NULL, NULL)) == NULL)
                {
                    (void)printf("Failure create new EVP key.\r\n");
                    EVP_PKEY_free(g_evp_key);
                    g_evp_key = NULL;
                    result = __LINE__;
                }
                else if (!EVP_PKEY_assign_RSA(g_evp_key, rsa))
                {
                    (void)printf("Failure create new EVP key.\r\n");
                    EVP_PKEY_free(g_evp_key);
                    g_evp_key = NULL;
                    result = __LINE__;
                }
                else if (save_key_file() != 0)
                {
                    (void)printf("Failure saving key file.\r\n");
                    EVP_PKEY_free(g_evp_key);
                    g_evp_key = NULL;
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

static int generate_certificate(X509_OPENSSL_INFO* hsm_impl, const CERTIFICATE_INFO* cert_info, int days_valid)
{
    int result;
    if (load_certificate_file(hsm_impl) != 0)
    {
        if ((hsm_impl->x509_cert = X509_new()) == NULL)
        {
            (void)printf("Failure creating the x509 cert\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
            if (!X509_set_version(hsm_impl->x509_cert, X509_VERSION))
            {
                result = __LINE__;
            }
            else if (!ASN1_INTEGER_set(X509_get_serialNumber(hsm_impl->x509_cert), SERIAL_NUMBER))
            {
                result = __LINE__;
            }
            else if (!X509_gmtime_adj(X509_get_notBefore(hsm_impl->x509_cert), 0))
            {
                (void)printf("Failure setting not before time\r\n");
                result = __LINE__;
            }
            else if (!X509_gmtime_adj(X509_get_notAfter(hsm_impl->x509_cert), days_valid * 86400))
            {
                (void)printf("Failure setting not after time\r\n");
                result = __LINE__;
            }
            else if (!X509_set_pubkey(hsm_impl->x509_cert, g_evp_key))
            {
                (void)printf("Failure setting public key\r\n");
                result = __LINE__;
            }
            else
            {
                X509_NAME* name = X509_get_subject_name(hsm_impl->x509_cert);
                if (name == NULL)
                {
                    (void)printf("Failure get subject name\r\n");
                    result = __LINE__;
                }
                else if (X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)cert_info->organization, -1, -1, 0) != 1)
                {
                    (void)printf("Failure X509_NAME_add_entry_by_txt 0\r\n");
                    result = __LINE__;
                }
                else if (X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)cert_info->country, -1, -1, 0) != 1)
                {
                    (void)printf("Failure X509_NAME_add_entry_by_txt C\r\n");
                    result = __LINE__;
                }
                else if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cert_info->subject_name, -1, -1, 0) != 1)
                {
                    (void)printf("Failure X509_NAME_add_entry_by_txt CN\r\n");
                    result = __LINE__;
                }
                else if (!X509_set_issuer_name(hsm_impl->x509_cert, name))
                {
                    (void)printf("Failure issuer name\r\n");
                    result = __LINE__;
                }
                else if (!X509_sign(hsm_impl->x509_cert, g_evp_key, EVP_sha256()))
                {
                    (void)printf("Failure signing x509\r\n");
                    result = __LINE__;
                }
                else if (save_certificate_file(hsm_impl) != 0)
                {
                    (void)printf("Failure saving x509 certificate\r\n");
                    result = __LINE__;
                }
                else
                {
                    result = 0;
                }
            }

            if (result != 0)
            {
                X509_free(hsm_impl->x509_cert);
                hsm_impl->x509_cert = NULL;
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

X509_INFO_HANDLE x509_info_create()
{
    X509_OPENSSL_INFO* result;
    /* Codes_SRS_HSM_CLIENT_TPM_07_002: [ On success hsm_client_tpm_create shall allocate a new instance of the secure device tpm interface. ] */
    result = malloc(sizeof(X509_OPENSSL_INFO));
    if (result == NULL)
    {
        /* Codes_SRS_HSM_CLIENT_TPM_07_001: [ If any failure is encountered hsm_client_tpm_create shall return NULL ] */
        printf("Failure: malloc X509_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(X509_OPENSSL_INFO));
        // Initialize openssl
        OpenSSL_add_all_algorithms();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();

        if (g_evp_key == NULL && create_key_pair() != 0)
        {
            (void)printf("Failure creating key pair\r\n");
            result = NULL;
        }
        else
        {
            result->use_ecc_type = g_use_ecc_type;

            generate_subject_name(result);
            CERTIFICATE_INFO cert_info;
            cert_info.organization = "microsoft";
            cert_info.subject_name = result->device_name;
            cert_info.country = "US";

            if (generate_certificate(result, &cert_info, 30) != 0)
            {
                (void)printf("Failure generating certificate\r\n");
                free(result);
                result = NULL;
            }
            else
            {
                // Generate a random subject name
                result = 0;
            }

        }

    }
    return result;
}

void x509_info_destroy(X509_INFO_HANDLE handle)
{
    if (handle != NULL)
    {
        if (g_subject_name != NULL)
        {
        }
        if (g_evp_key != NULL)
        {
            EVP_PKEY_free(g_evp_key);
        }
        X509_free(handle->x509_cert);
        free(handle);
    }
}

#if 0
int generate_certificate(X509_INFO_HANDLE handle, CERTIFICATE_INFO* cert_info)
{
    int result;
    BUF_MEM* bio_ptr;
    BIO* mem_bio = BIO_new(BIO_s_mem());
    if (mem_bio == NULL)
    {
        (void)printf("Failure creating bio\r\n");
        result = __LINE__;
    }
    else if (!PEM_write_bio_X509(mem_bio, handle->x509_cert))
    {
        (void)printf("Failure writing cert to bio\r\n");
        result = __LINE__;
    }
    else
    {
        BIO_get_mem_ptr(mem_bio, &bio_ptr);
        /*if ((result = (char*)malloc(bio_ptr->length + 1)) == NULL)
        {
            (void)printf("Failure allocating certificate\r\n");
            result = __LINE__;
        }
        else
        {
            //memset(result, 0, bio_ptr->length + 1);
            //memcpy(result, bio_ptr->data, bio_ptr->length);
            result = 0;
        }*/

        BUF_MEM* bio_ptr;
        BIO* mem_bio = BIO_new(BIO_s_mem());
        if (mem_bio == NULL)
        {
            (void)printf("Failure creating bio\r\n");
            result = NULL;
        }
        else if (!PEM_write_bio_PrivateKey(mem_bio, g_evp_key, NULL, NULL, 0, 0, NULL))
        {
            (void)printf("Failure writing cert to bio\r\n");
            result = NULL;
        }
        else
        {
            /*BIO_get_mem_ptr(mem_bio, &bio_ptr);
            if ((result = (char*)malloc(bio_ptr->length)) == NULL)
            {
                (void)printf("Failure allocating certificate\r\n");
                result = NULL;
            }
            else
            {
                memset(result, 0, bio_ptr->length);
                memcpy(result, bio_ptr->data, bio_ptr->length - 1);
            }
            BIO_free_all(mem_bio);*/
        }

        //BIO_free_all(mem_bio);
    }
    return result;
}
#endif
const char* x509_info_get_cert(X509_INFO_HANDLE handle)
{
    const char* result = NULL;
    return result;
}

const char* x509_info_get_key(X509_INFO_HANDLE handle)
{
    const char* result = NULL;
    return result;
}

const char* x509_info_get_common_name(X509_INFO_HANDLE handle)
{
    const char* result = NULL;
    return result;
}
