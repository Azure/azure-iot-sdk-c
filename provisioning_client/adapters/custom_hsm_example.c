// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "custom_hsm_abstract.h"
#include "hsm_client_data.h"

typedef struct CUSTOM_HSM_INFO_TAG
{
    int info;
} CUSTOM_HSM_INFO;

static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_get_certificate,
    custom_hsm_get_alias_key,
    custom_hsm_get_common_name
};

static const HSM_CLIENT_TPM_INTERFACE tpm_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_import_key,
    custom_hsm_get_endorsement_key,
    custom_hsm_get_storage_root_key,
    custom_hsm_sign_key
};

HSM_CLIENT_HANDLE custom_hsm_create()
{
    CUSTOM_HSM_INFO* result;
    result = malloc(sizeof(CUSTOM_HSM_INFO));
    if (result == NULL)
    {
        (void)printf("Failure: malloc CUSTOM_HSM_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(CUSTOM_HSM_INFO));
        result = 0;
    }
    return (HSM_CLIENT_HANDLE)result;
}

void custom_hsm_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        CUSTOM_HSM_INFO* hsm_impl = (CUSTOM_HSM_INFO*)handle;
        free(hsm_impl);
    }
}

int hsm_client_x509_init()
{
    // Add any code needed to initialize the x509 module
    return 0;
}

void hsm_client_x509_deinit()
{
}

int hsm_client_tpm_init()
{
    // Add any code needed to initialize the TPM module
    return 0;
}

void hsm_client_tpm_deinit()
{
}

// Return the X509 certificate in PEM format
char* custom_hsm_get_certificate(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        const char* cert = "certificate";
        if (cert == NULL)
        {
            (void)printf("Failure retrieving cert");
            result = NULL;
        }
        else
        {
            size_t length = strlen(cert);
            result = malloc(length + 1);
            if (result == NULL)
            {
                (void)printf("Failure allocating certifiicate");
            }
            else
            {
                strcpy(result, cert);
            }
        }
    }
    return result;
}

// Return Private Key of the Certification
char* custom_hsm_get_alias_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        const char* private_key = "alias_key";
        if (private_key == NULL)
        {
            (void)printf("Failure retrieving private key");
            result = NULL;
        }
        else
        {
            size_t length = strlen(private_key);
            result = malloc(length + 1);
            if (result == NULL)
            {
                (void)printf("Failure allocating private key");
            }
            else
            {
                strcpy(result, private_key);
            }
        }
    }
    return result;
}

// Return allocated common name on the x509 certificate
char* custom_hsm_get_common_name(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        const char* common_name = "Common_name"
        if (common_name == NULL)
        {
            (void)printf("Failure retrieving common name");
            result = NULL;
        }
        else
        {
            size_t length = strlen(common_name);
            result = malloc(length + 1);
            if (result == NULL)
            {
                (void)printf("Failure allocating common name");
            }
            else
            {
                strcpy(result, common_name);
            }
        }
    }
    return result;
}

// TPM Custom Information handling
// Allocates the endorsement key using as key and the length as key_len
int custom_hsm_get_endorsement_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    int result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

// Allocates the Storage Root key using as key and the length as key_len
int custom_hsm_get_storage_root_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == NULL)
    {
        (void)printf("Invalid handle value specified");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

// Decrypt and Stores the encrypted key
int custom_hsm_activate_id_key(HSM_CLIENT_HANDLE handle, const unsigned char* key, size_t key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == 0)
    {
        (void)printf("Invalid argument specified handle: %p, key: %p, key_len: %d", handle, key, key_len);
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

// Hashes value specified in data with the key stored in slot 1 and returns the result in signed_value
int custom_hsm_sign_with_identity(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len)
{
    int result;
    if (handle == NULL || data == NULL || data_len == 0 || signed_value == NULL || signed_len == NULL)
    {
        (void)printf("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_get_certificate,
    custom_hsm_get_alias_key,
    custom_hsm_get_common_name
};

static const HSM_CLIENT_TPM_INTERFACE tpm_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_activate_id_key,
    custom_hsm_get_endorsement_key,
    custom_hsm_get_storage_root_key,
    custom_hsm_sign_with_identity
};

const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface()
{
    return &tpm_interface;
}

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface()
{
    return &x509_interface;
}
