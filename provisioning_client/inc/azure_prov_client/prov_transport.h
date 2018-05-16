// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_TRANSPORT_H
#define PROV_TRANSPORT_H

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/buffer_.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif /* __cplusplus */

    struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG;
    typedef struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG PROV_DEVICE_TRANSPORT_PROVIDER;

    typedef void* PROV_DEVICE_TRANSPORT_HANDLE;

#define TRANSPORT_HSM_TYPE_VALUES   \
    TRANSPORT_HSM_TYPE_TPM,         \
    TRANSPORT_HSM_TYPE_X509

    DEFINE_ENUM(TRANSPORT_HSM_TYPE, TRANSPORT_HSM_TYPE_VALUES);

#define PROV_DEVICE_TRANSPORT_RESULT_VALUES     \
    PROV_DEVICE_TRANSPORT_RESULT_OK,            \
    PROV_DEVICE_TRANSPORT_RESULT_UNAUTHORIZED,  \
    PROV_DEVICE_TRANSPORT_RESULT_ERROR

    DEFINE_ENUM(PROV_DEVICE_TRANSPORT_RESULT, PROV_DEVICE_TRANSPORT_RESULT_VALUES);

#define PROV_DEVICE_TRANSPORT_STATUS_VALUES     \
    PROV_DEVICE_TRANSPORT_STATUS_CONNECTED,     \
    PROV_DEVICE_TRANSPORT_STATUS_AUTHENTICATED, \
    PROV_DEVICE_TRANSPORT_STATUS_UNASSIGNED,    \
    PROV_DEVICE_TRANSPORT_STATUS_ASSIGNING,     \
    PROV_DEVICE_TRANSPORT_STATUS_ASSIGNED,      \
    PROV_DEVICE_TRANSPORT_STATUS_BLACKLISTED,   \
    PROV_DEVICE_TRANSPORT_STATUS_ERROR

    DEFINE_ENUM(PROV_DEVICE_TRANSPORT_STATUS, PROV_DEVICE_TRANSPORT_STATUS_VALUES);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_TRANSPORT
