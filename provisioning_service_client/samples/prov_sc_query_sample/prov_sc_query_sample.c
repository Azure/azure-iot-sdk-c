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

void print_ie_results(PROVISIONING_QUERY_RESPONSE* query_resp)
{
    for (int i = 0; i < (int)query_resp->response_arr_size; i++)
    {
        INDIVIDUAL_ENROLLMENT_HANDLE ie = query_resp->response_arr.ie[i]; //access the ie (Individual Enrollment) portion of the response array
        printf("Individual Enrollment Registration ID: %s\n", individualEnrollment_getRegistrationId(ie));
    }
}

int main()
{
    int result = 0;

    const char* connectionString = "[Connection String]";

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

    /* ---Set the Query Specification details for enrollments--- */
    PROVISIONING_QUERY_SPECIFICATION query_spec = { 0 };
    query_spec.page_size = 5;       //can specify NO_MAX_PAGE if you would prefer all results at once
    query_spec.query_string = "*";
    query_spec.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    PROVISIONING_QUERY_RESPONSE* query_resp; //will be filled in by the query function
    char* cont_token = NULL; //will be updated by the query function

    /* --- Run the Query until there are no more results---*/
    prov_sc_query_individual_enrollment(prov_sc, &query_spec, &cont_token, &query_resp);
    print_ie_results(query_resp);
    queryResponse_free(query_resp);
    while (cont_token != NULL)     //while there are more results (i.e. the cont token is not NULL), keep querying
    {
        prov_sc_query_individual_enrollment(prov_sc, &query_spec, &cont_token, &query_resp);
        print_ie_results(query_resp);
        queryResponse_free(query_resp);     //if you are reusing the same query_resp structure, must free it in between calls
    }

    /* ---Cleanup memory---*/
    free(cont_token);
    prov_sc_destroy(prov_sc);
    platform_deinit();

    return result;
}
