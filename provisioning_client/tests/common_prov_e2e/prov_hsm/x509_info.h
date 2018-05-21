// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef X509_OPENSSL_H
#define X509_OPENSSL_H

    typedef struct CERTIFICATE_INFO_TAG
    {
        const char* country;
        const char* subject_name;
        const char* organization;
    } CERTIFICATE_INFO;

    typedef struct X509_CERT_INFO_TAG* X509_INFO_HANDLE;

    extern void initialize_device();
    extern X509_INFO_HANDLE x509_info_create();
    extern void x509_info_destroy(X509_INFO_HANDLE handle);
    extern const char* x509_info_get_cert(X509_INFO_HANDLE handle);
    extern const char* x509_info_get_key(X509_INFO_HANDLE handle);
    extern const char* x509_info_get_cn(X509_INFO_HANDLE handle);
#endif
