// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Stub for csr_gen_openssl.c when building against OpenSSL < 3.0.
// EVP_EC_gen() was added in OpenSSL 3.0; older toolchains (cross-compile
// Docker images with 1.0.2/1.1.1) cannot compile the real implementation.

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L

#include <stddef.h>
#include "azure_c_shared_utility/csr_gen.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "macro_utils/macro_utils.h"

int csr_gen_ec_p256(const char* common_name, char** csr_base64, char** private_key_pem)
{
    (void)common_name;
    (void)csr_base64;
    (void)private_key_pem;
    LogError("csr_gen_ec_p256 requires OpenSSL >= 3.0 (EVP_EC_gen)");
    return MU_FAILURE;
}

#endif
