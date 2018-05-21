// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "umock_c.h"
#endif

#include "testrunnerswitcher.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/internal/prov_auth_client.h"

#include "prov_service_client/provisioning_service_client.h"
#include "prov_service_client/provisioning_sc_enrollment.h"

#include "../../../certs/certs.h"

#include "common_prov_e2e.h"

#define MAX_CLOUD_TRAVEL_TIME       60.0
#define DEVICE_GUID_SIZE            37

void wait_for_dps_result(PROV_DEVICE_LL_HANDLE handle, PROV_CLIENT_E2E_INFO* prov_info)
{
    time_t begin_operation;
    time_t now_time;

    begin_operation = time(NULL);
    do
    {
        Prov_Device_LL_DoWork(handle);
        ThreadAPI_Sleep(1);
    } while ((prov_info->reg_result == REG_RESULT_BEGIN) &&
        ((now_time = time(NULL)),
        (difftime(now_time, begin_operation) < MAX_CLOUD_TRAVEL_TIME)));
}

int construct_device_id(const char* prefix, char** device_name)
{
    int result;
    char deviceGuid[DEVICE_GUID_SIZE];
    if (UniqueId_Generate(deviceGuid, DEVICE_GUID_SIZE) != UNIQUEID_OK)
    {
        LogError("Unable to generate unique Id.\r\n");
        result = __FAILURE__;
    }
    else
    {
        size_t len = strlen(prefix) + DEVICE_GUID_SIZE;
        *device_name = (char*)malloc(len + 1);
        if (*device_name == NULL)
        {
            LogError("Failure allocating device ID.\r\n");
            result = __FAILURE__;
        }
        else
        {
            if (sprintf_s(*device_name, len + 1, prefix, deviceGuid) <= 0)
            {
                LogError("Failure constructing device ID.\r\n");
                free(*device_name);
                result = __FAILURE__;
            }
            else
            {
                LogInfo("Created Device %s.", *device_name);
                result = 0;
            }
        }
    }
    return result;
}

void create_tpm_enrollment_device(const char* prov_conn_string, bool use_tracing)
{
    INDIVIDUAL_ENROLLMENT_HANDLE indiv_enrollment = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(prov_conn_string);
    ASSERT_IS_NOT_NULL_WITH_MSG(prov_sc_handle, "Failure creating provisioning service client");

    if (use_tracing)
    {
        prov_sc_set_trace(prov_sc_handle, TRACING_STATUS_ON);
    }

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure prov_auth_get_common_name");

    if (prov_sc_get_individual_enrollment(prov_sc_handle, registration_id, &indiv_enrollment) != 0)
    {
        BUFFER_HANDLE ek_handle = prov_auth_get_endorsement_key(auth_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(ek_handle, "Failure prov_auth_get_endorsement_key");

        STRING_HANDLE ek_value = Base64_Encoder(ek_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(ek_value, "Failure Base64_Encoder Endorsement key");

        ATTESTATION_MECHANISM_HANDLE attest_handle = attestationMechanism_createWithTpm(STRING_c_str(ek_value), NULL);
        ASSERT_IS_NOT_NULL_WITH_MSG(attest_handle, "Failure attestationMechanism_createWithTpm");

        indiv_enrollment = individualEnrollment_create(registration_id, attest_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(indiv_enrollment, "Failure hsm_client_riot_get_certificate ");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, prov_sc_create_or_update_individual_enrollment(prov_sc_handle, &indiv_enrollment), "Failure prov_sc_create_or_update_individual_enrollment");

        BUFFER_delete(ek_handle);
        STRING_delete(ek_value);
    }
    free(registration_id);
    individualEnrollment_destroy(indiv_enrollment);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}

void create_x509_enrollment_device(const char* prov_conn_string, bool use_tracing)
{
    INDIVIDUAL_ENROLLMENT_HANDLE indiv_enrollment = NULL;

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(prov_conn_string);
    ASSERT_IS_NOT_NULL_WITH_MSG(prov_sc_handle, "Failure creating provisioning service client");

    if (use_tracing)
    {
        prov_sc_set_trace(prov_sc_handle, TRACING_STATUS_ON);
    }

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure prov_auth_get_common_name");

    if (prov_sc_get_individual_enrollment(prov_sc_handle, registration_id, &indiv_enrollment) != 0)
    {
        char* x509_cert = prov_auth_get_certificate(auth_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(x509_cert, "Failure prov_auth_get_certificate");

        ATTESTATION_MECHANISM_HANDLE attest_handle = attestationMechanism_createWithX509ClientCert(x509_cert, NULL);
        ASSERT_IS_NOT_NULL_WITH_MSG(attest_handle, "Failure hsm_client_riot_get_certificate ");

        indiv_enrollment = individualEnrollment_create(registration_id, attest_handle);
        ASSERT_IS_NOT_NULL_WITH_MSG(indiv_enrollment, "Failure hsm_client_riot_get_certificate ");

        ASSERT_ARE_EQUAL_WITH_MSG(int, 0, prov_sc_create_or_update_individual_enrollment(prov_sc_handle, &indiv_enrollment), "Failure prov_sc_create_or_update_individual_enrollment");

        free(x509_cert);
    }
    free(registration_id);
    individualEnrollment_destroy(indiv_enrollment);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}

void remove_enrollment_device(const char* prov_conn_string)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc_handle = prov_sc_create_from_connection_string(prov_conn_string);
    ASSERT_IS_NOT_NULL_WITH_MSG(prov_sc_handle, "Failure creating provisioning service client");

    PROV_AUTH_HANDLE auth_handle = prov_auth_create();
    ASSERT_IS_NOT_NULL_WITH_MSG(auth_handle, "Failure creating auth client");

    char* registration_id = prov_auth_get_registration_id(auth_handle);
    ASSERT_IS_NOT_NULL_WITH_MSG(registration_id, "Failure retrieving registration Id");

    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, prov_sc_delete_individual_enrollment_by_param(prov_sc_handle, registration_id, NULL), "Failure deleting enrollment");

    free(registration_id);
    prov_auth_destroy(auth_handle);
    prov_sc_destroy(prov_sc_handle);
}
