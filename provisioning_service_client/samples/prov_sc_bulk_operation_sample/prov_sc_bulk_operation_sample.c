// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>

#include "azure_c_shared_utility/platform.h"

#include "../../../certs/certs.h"

#include "prov_service_client/provisioning_service_client.h"

static bool g_use_trace = true;

#ifdef USE_OPENSSL
static bool g_use_certificate = true;
#else
static bool g_use_certificate = false;
#endif //USE_OPENSSL

int main()
{
    int result = 0;

    const char* connectionString = "[Connection String]";
    const char* endorsementKey = "[Endorsement Key]";
    const char* registrationId1 = "[Registration Id #1]";
    const char* registrationId2 = "[Registration Id #2]";

    /* ---This function must be called before anything else so that sockets work--- */
    platform_init();

    /* ---Create a handle for accessing the Provisioning Service--- */
    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc = prov_sc_create_from_connection_string(connectionString);

    /* ---Optionally set connection options---*/
    if (g_use_trace)
    {
        prov_sc_set_trace(prov_sc, TRACING_STATUS_ON);
    }
    if (g_use_certificate)
    {
        prov_sc_set_certificate(prov_sc, certificates);
    }

    /* ---Build the array of individual enrollments to run bulk operations on--- */
    ATTESTATION_MECHANISM_HANDLE am1 = attestationMechanism_createWithTpm(endorsementKey, NULL);
    ATTESTATION_MECHANISM_HANDLE am2 = attestationMechanism_createWithTpm(endorsementKey, NULL);
    INDIVIDUAL_ENROLLMENT_HANDLE ie1 = individualEnrollment_create(registrationId1, am1);
    INDIVIDUAL_ENROLLMENT_HANDLE ie2 = individualEnrollment_create(registrationId2, am2);
    INDIVIDUAL_ENROLLMENT_HANDLE ie_list[2];
    ie_list[0] = ie1;
    ie_list[1] = ie2;

    /* ---Build the bulk operation structure for creation--- */
    PROVISIONING_BULK_OPERATION bulkop = { 0 };
    bulkop.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.enrollments.ie = ie_list;
    bulkop.num_enrollments = 2;

    /* ---Define a pointer that can be filled with results--- */
    PROVISIONING_BULK_OPERATION_RESULT* bulkres;

    /* ---Run the bulk operation to create enrollments--- */
    prov_sc_run_individual_enrollment_bulk_operation(prov_sc, &bulkop, &bulkres);

    /* ---Free memory allocated in bulkres---*/
    bulkOperationResult_free(bulkres);

    /* ---Now adjust the bulk operation structure for deletion */
    bulkop.mode = BULK_OP_DELETE;

    /* ---Run the bulk operation to delete the enrollments--- */
    prov_sc_run_individual_enrollment_bulk_operation(prov_sc, &bulkop, &bulkres);

    /* ---Free memory allocated in bulkres--- */
    bulkOperationResult_free(bulkres);

    /* ---Destroy the enrollment related handles--- */
    individualEnrollment_destroy(ie1);
    individualEnrollment_destroy(ie2);

    /* ---Destroy the provisioning service client handle--- */
    prov_sc_destroy(prov_sc);
    platform_deinit();

    return result;
}
