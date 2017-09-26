// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SASL_TPM_ANONYMOUS_H
#define SASL_TPM_ANONYMOUS_H

#include "azure_uamqp_c/sasl_mechanism.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_hub_modules/dps_transport.h"

    typedef char*(*TPM_CHALLENGE_CALLBACK)(BUFFER_HANDLE data_handle, void* user_ctx);

    typedef struct SASL_TPM_CONFIG_INFO_TAG
    {
        BUFFER_HANDLE endorsement_key;
        BUFFER_HANDLE storage_root_key;
        const char* registration_id;
        const char* dps_hostname;
        TPM_CHALLENGE_CALLBACK challenge_cb;
        void* user_ctx;
    } SASL_TPM_CONFIG_INFO;

    MOCKABLE_FUNCTION(, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, dps_sasltpm_get_interface);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SASL_TPM_ANONYMOUS_H */
