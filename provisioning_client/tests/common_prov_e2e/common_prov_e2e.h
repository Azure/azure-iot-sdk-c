// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef COMMON_PROV_E2E_H
#define COMMON_PROV_E2E_H

#ifdef __cplusplus
extern "C" {
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "azure_prov_client/prov_device_ll_client.h"

typedef enum REGISTRATION_RESULT_TAG
{
    REG_RESULT_BEGIN,
    REG_RESULT_COMPLETE,
    REG_RESULT_FAILED
} REGISTRATION_RESULT;

typedef struct PROV_CLIENT_E2E_INFO_TAG
{
    char* iothub_uri;
    char* device_id;
    REGISTRATION_RESULT reg_result;
} PROV_CLIENT_E2E_INFO;

extern void create_x509_enrollment_device(const char* prov_conn_string, bool use_tracing);
extern void create_tpm_enrollment_device(const char* prov_conn_string, bool use_tracing);
extern void remove_enrollment_device(const char* prov_conn_string);

extern void wait_for_dps_result(PROV_DEVICE_LL_HANDLE handle, PROV_CLIENT_E2E_INFO* prov_info);
extern int construct_device_id(const char* prefix, char** device_name);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_PROV_E2E_H */
