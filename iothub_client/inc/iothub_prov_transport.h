// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_PROV_TRANSPORT_H
#define IOTHUB_PROV_TRANSPORT_H

#include "internal/iothub_transport_ll_private.h"
#ifdef USE_PROV_MODULE
#include "azure_prov_client/prov_security_factory.h"
#endif

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C"
{
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

    #define PROVISIONING_INFO_VERSION       1;

    struct IOTHUB_PROV_TRANSPORT_PROVIDER_TAG;
    typedef struct IOTHUB_PROV_TRANSPORT_PROVIDER_TAG IOTHUB_PROV_TRANSPORT_PROVIDER;

    typedef const IOTHUB_PROV_TRANSPORT_PROVIDER*(*IOTHUB_PROV_CLIENT_TRANSPORT_PROVIDER)(void);

    typedef struct PROVISIONING_AUTH_INFO_TAG
    {
        unsigned int version;
        const char* provisioning_uri;
        const char* id_scope;
#ifdef USE_PROV_MODULE
        SECURE_DEVICE_TYPE attestation_type;
#endif
        IOTHUB_PROV_CLIENT_TRANSPORT_PROVIDER transport;
        const char* registration_id;
        const char* symmetric_key;
    } PROVISIONING_AUTH_INFO;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // IOTHUB_PROV_TRANSPORT_H
