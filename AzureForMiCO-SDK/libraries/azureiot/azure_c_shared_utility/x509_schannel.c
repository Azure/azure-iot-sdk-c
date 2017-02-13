// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"

#ifdef WINCE
#include <wincrypt.h>
#endif
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/x509_schannel.h"
#include "azure_c_shared_utility/xlogging.h"

typedef struct X509_SCHANNEL_HANDLE_DATA_TAG
{
    HCRYPTPROV hProv;
    HCRYPTKEY x509hcryptkey;
    PCCERT_CONTEXT x509certificate_context;
}X509_SCHANNEL_HANDLE_DATA;

X509_SCHANNEL_HANDLE x509_schannel_create(const char* x509certificate, const char* x509privatekey)
{
    X509_SCHANNEL_HANDLE_DATA *result;
    /*this is what happens with the x509 certificate and the x509 private key in this function*/
    /*
    step 1: they are converted to binary form.
    step 1.1: the size of the binary form is computed
    step 1.2: the conversion happens
    step 2: the binary form is decoded
    step 2.1: the decoded form needed size is computed
    step 2.2: the decoded form is actually decoded
    step 3: a crypto provider is created
    step 4: the x509 private key is associated with the crypto provider
    step 5: a certificate context is created
    step 6: the certificate context is linked to the crypto provider
    */

    /*Codes_SRS_X509_SCHANNEL_02_001: [ If x509certificate or x509privatekey are NULL then x509_schannel_create shall fail and return NULL. ]*/
    if (
        (x509certificate == NULL) ||
        (x509privatekey == NULL)
        )
    {
        /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
        LogError("invalid argument const char* x509certificate=%p, const char* x509privatekey=%p", x509certificate, x509privatekey);
        result = NULL;
    }
    else
    {
        result = (X509_SCHANNEL_HANDLE_DATA*)malloc(sizeof(X509_SCHANNEL_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
            /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
            /*return as is*/

        }
        else
        {
            /*Codes_SRS_X509_SCHANNEL_02_002: [ x509_schannel_create shall convert the certificate to binary form by calling CryptStringToBinaryA. ]*/
            DWORD binaryx509certificateSize;
            if (!CryptStringToBinaryA(x509certificate, 0, CRYPT_STRING_ANY, NULL, &binaryx509certificateSize, NULL, NULL))
            {
                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                LogErrorWinHTTPWithGetLastErrorAsString("Failed convert x509 certificate");
                free(result);
                result = NULL;
            }
            else
            {
                unsigned char* binaryx509Certificate = (unsigned char*)malloc(binaryx509certificateSize);
                if (binaryx509Certificate == NULL)
                {
                    /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                    LogError("unable to allocate memory for x509 certificate");
                    free(result);
                    result = NULL;
                }
                else
                {
                    if (!CryptStringToBinaryA(x509certificate, 0, CRYPT_STRING_ANY, binaryx509Certificate, &binaryx509certificateSize, NULL, NULL))
                    {
                        /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                        LogErrorWinHTTPWithGetLastErrorAsString("Failed convert x509 certificate");
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_X509_SCHANNEL_02_003: [ x509_schannel_create shall convert the private key to binary form by calling CryptStringToBinaryA. ]*/
                        /*at this moment x509 certificate is ready to be used in CertCreateCertificateContext*/
                        DWORD binaryx509privatekeySize;
                        if (!CryptStringToBinaryA(x509privatekey, 0, CRYPT_STRING_ANY, NULL, &binaryx509privatekeySize, NULL, NULL))
                        {
                            /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                            LogErrorWinHTTPWithGetLastErrorAsString("Failed convert x509 private key");
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            unsigned char* binaryx509privatekey = (unsigned char*)malloc(binaryx509privatekeySize);
                            if (binaryx509privatekey == NULL)
                            {
                                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                LogError("unable to malloc for binaryx509privatekey");
                                free(result);
                                result = NULL;
                            }
                            else
                            {
                                if (!CryptStringToBinaryA(x509privatekey, 0, CRYPT_STRING_ANY, binaryx509privatekey, &binaryx509privatekeySize, NULL, NULL))
                                {
                                    /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                    LogErrorWinHTTPWithGetLastErrorAsString("Failed convert x509 private key");
                                    free(result);
                                    result = NULL;
                                }
                                else
                                {
                                    /*Codes_SRS_X509_SCHANNEL_02_004: [ x509_schannel_create shall decode the private key by calling CryptDecodeObjectEx. ]*/
                                    DWORD x509privatekeyBlobSize;
                                    if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, binaryx509privatekey, binaryx509privatekeySize, 0, NULL, NULL, &x509privatekeyBlobSize))
                                    {
                                        /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                        LogErrorWinHTTPWithGetLastErrorAsString("Failed to CryptDecodeObjectEx x509 private key");
                                        free(result);
                                        result = NULL;
                                    }
                                    else
                                    {
                                        unsigned char* x509privatekeyBlob = (unsigned char*)malloc(x509privatekeyBlobSize);
                                        if (x509privatekeyBlob == NULL)
                                        {
                                            /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                            LogError("unable to malloc for x509 private key blob");
                                            free(result);
                                            result = NULL;
                                        }
                                        else
                                        {
                                            if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, binaryx509privatekey, binaryx509privatekeySize, 0, NULL, x509privatekeyBlob, &x509privatekeyBlobSize))
                                            {
                                                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                                LogErrorWinHTTPWithGetLastErrorAsString("Failed to CryptDecodeObjectEx x509 private key");
                                                free(result);
                                                result = NULL;
                                            }
                                            else
                                            {
                                                /*Codes_SRS_X509_SCHANNEL_02_005: [ x509_schannel_create shall call CryptAcquireContext. ]*/
                                                /*at this moment, both the private key and the certificate are decoded for further usage*/
                                                if (!CryptAcquireContext(&(result->hProv), NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
                                                {
                                                    /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                                    LogErrorWinHTTPWithGetLastErrorAsString("CryptAcquireContext failed");
                                                    free(result);
                                                    result = NULL;
                                                }
                                                else
                                                {
                                                    /*Codes_SRS_X509_SCHANNEL_02_006: [ x509_schannel_create shall import the private key by calling CryptImportKey. ] */
                                                    if (!CryptImportKey(result->hProv, x509privatekeyBlob, x509privatekeyBlobSize, (HCRYPTKEY)NULL, 0, &(result->x509hcryptkey)))
                                                    {
                                                        /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                                        if (!CryptReleaseContext(result->hProv, 0))
                                                        {
                                                            LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptReleaseContext");
                                                        }
                                                        LogErrorWinHTTPWithGetLastErrorAsString("CryptImportKey for private key failed");
                                                        free(result);
                                                        result = NULL;
                                                    }
                                                    else
                                                    {
                                                        /*Codes_SRS_X509_SCHANNEL_02_007: [ x509_schannel_create shall create a cerficate context by calling CertCreateCertificateContext. ]*/
                                                        result->x509certificate_context = CertCreateCertificateContext(
                                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                            binaryx509Certificate,
                                                            binaryx509certificateSize
                                                        );
                                                        if (result->x509certificate_context == NULL)
                                                        {
                                                            /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                                            LogErrorWinHTTPWithGetLastErrorAsString("unable to CertCreateCertificateContext");
                                                            if (!CryptDestroyKey(result->x509hcryptkey))
                                                            {
                                                                LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptDestroyKey");
                                                            }
                                                            if (!CryptReleaseContext(result->hProv, 0))
                                                            {
                                                                LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptReleaseContext");
                                                            }
                                                            free(result);
                                                            result = NULL;
                                                        }
                                                        else
                                                        {
                                                            /*Codes_SRS_X509_SCHANNEL_02_008: [ x509_schannel_create shall call set the certificate private key by calling CertSetCertificateContextProperty. ]*/
                                                            if (!CertSetCertificateContextProperty(result->x509certificate_context, CERT_KEY_PROV_HANDLE_PROP_ID, 0, (void*)result->hProv))
                                                            {
                                                                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                                                                LogErrorWinHTTPWithGetLastErrorAsString("unable to CertSetCertificateContextProperty");

                                                                if (!CryptDestroyKey(result->x509hcryptkey))
                                                                {
                                                                    LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptDestroyKey");
                                                                }
                                                                if (!CryptReleaseContext(result->hProv, 0))
                                                                {
                                                                    LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptReleaseContext");
                                                                }
                                                                if (!CertFreeCertificateContext(result->x509certificate_context))
                                                                {
                                                                    LogErrorWinHTTPWithGetLastErrorAsString("unable to CertFreeCertificateContext");
                                                                }
                                                                free(result);
                                                                result = NULL;
                                                            }
                                                            else
                                                            {
                                                                /*Codes_SRS_X509_SCHANNEL_02_009: [ If all the operations above succeed, then x509_schannel_create shall succeeds and return a non-NULL X509_SCHANNEL_HANDLE. ]*/
                                                                /*return as is, all is fine*/
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            free(x509privatekeyBlob);
                                        }
                                    }
                                }
                                free(binaryx509privatekey);
                            }
                        }
                    }
                    free(binaryx509Certificate);
                }
            }
        }
    }
    return result;
}

void x509_schannel_destroy(X509_SCHANNEL_HANDLE x509_schannel_handle)
{
    /*Codes_SRS_X509_SCHANNEL_02_011: [ If parameter x509_schannel_handle is NULL then x509_schannel_destroy shall do nothing. ]*/
    if (x509_schannel_handle != NULL)
    {
        /*Codes_SRS_X509_SCHANNEL_02_012: [ Otherwise, x509_schannel_destroy shall free all used resources. ]*/
        X509_SCHANNEL_HANDLE_DATA* x509crypto = (X509_SCHANNEL_HANDLE_DATA*)x509_schannel_handle;

        if (!CryptDestroyKey(x509crypto->x509hcryptkey))
        {
            LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptDestroyKey");
        }

        if (!CryptReleaseContext(x509crypto->hProv, 0))
        {
            LogErrorWinHTTPWithGetLastErrorAsString("unable to CryptReleaseContext");
        }

        if (!CertFreeCertificateContext(x509crypto->x509certificate_context))
        {
            LogErrorWinHTTPWithGetLastErrorAsString("unable to CertFreeCertificateContext");
        }

        free(x509crypto);
    }
}

PCCERT_CONTEXT x509_schannel_get_certificate_context(X509_SCHANNEL_HANDLE x509_schannel_handle)
{
    PCCERT_CONTEXT result;
    if (x509_schannel_handle == NULL)
    {
        /*Codes_SRS_X509_SCHANNEL_02_013: [ If parameter x509_schannel_handle is NULL then x509_schannel_get_certificate_context shall return NULL. ]*/
        result = NULL;
    }
    else
    {
        /*Codes_SRS_X509_SCHANNEL_02_014: [ Otherwise, x509_schannel_get_certificate_context shall return a non-NULL PCCERT_CONTEXT pointer. ]*/
        X509_SCHANNEL_HANDLE_DATA* handleData = (X509_SCHANNEL_HANDLE_DATA*)x509_schannel_handle;
        result = handleData->x509certificate_context;
    }
    return result;
}
