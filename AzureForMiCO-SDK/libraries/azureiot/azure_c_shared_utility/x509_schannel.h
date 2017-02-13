// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef X509_SCHANNEL_H
#define X509_SCHANNEL_H

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#else
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

typedef struct X509_SCHANNEL_HANDLE_DATA_TAG* X509_SCHANNEL_HANDLE;

MOCKABLE_FUNCTION(, X509_SCHANNEL_HANDLE, x509_schannel_create, const char*, x509certificate, const char*, x509privatekey);
MOCKABLE_FUNCTION(, void, x509_schannel_destroy, X509_SCHANNEL_HANDLE, x509_schannel_handle);
MOCKABLE_FUNCTION(, PCCERT_CONTEXT, x509_schannel_get_certificate_context, X509_SCHANNEL_HANDLE, x509_schannel_handle);

#ifdef __cplusplus
}
#endif 

#endif
