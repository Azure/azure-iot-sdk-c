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

extern const char* g_prov_conn_string;
extern const char* g_dps_scope_id;
extern const char* g_dps_uri;
extern const char* g_desired_iothub;
extern char* g_dps_x509_cert_individual;
extern char* g_dps_x509_key_individual;
extern char* g_dps_regid_individual;
extern const bool g_enable_tracing;

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

static const char* const DPS_CONNECTION_STRING = "IOT_DPS_CONNECTION_STRING";
static const char* const DPS_GLOBAL_ENDPOINT = "IOT_DPS_GLOBAL_ENDPOINT";
static const char* const DPS_ID_SCOPE = "IOT_DPS_ID_SCOPE";
static const char* const DPS_TPM_SIMULATOR_IP_ADDRESS = "IOT_DPS_TPM_SIMULATOR_IP_ADDRESS";
static const char* const DPS_X509_INDIVIDUAL_CERT_BASE64 = "IOT_DPS_INDIVIDUAL_X509_CERTIFICATE";
static const char* const DPS_X509_INDIVIDUAL_KEY_BASE64 = "IOT_DPS_INDIVIDUAL_X509_KEY";
static const char* const DPS_X509_INDIVIDUAL_REGISTRATION_ID = "IOT_DPS_INDIVIDUAL_REGISTRATION_ID";

#ifdef TEST_OPENSSL_ENGINE
#define OPENSSL_ENGINE_ID "pkcs11"
#define PKCS11_PRIVATE_KEY_URI "pkcs11:object=dps-privkey;type=private?pin-value=1234"
#endif

extern void create_x509_individual_enrollment_device();
extern void create_tpm_enrollment_device();
extern void create_symm_key_enrollment_device();
extern void remove_enrollment_device();

extern void wait_for_dps_result(PROV_DEVICE_LL_HANDLE handle, PROV_CLIENT_E2E_INFO* prov_info);
extern int construct_device_id(const char* prefix, char** device_name);

extern void send_dps_test_registration_with_retry(const char* global_uri, const char* scope_id, PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION protocol, bool use_tracing);
extern char* convert_base64_to_string(const char* base64_cert);

extern const int TEST_PROV_RANDOMIZED_BACK_OFF_SEC;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_PROV_E2E_H */
