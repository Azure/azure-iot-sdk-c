// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "provisioning_service_client.h"

int main()
{
    int result;
    result = 0;

    const char* connectionString = "[IoTHub Connection String]";
    const char* deviceId = "[Device Id]";
    const char* registrationId = "[Registration Id]";

    INDIVIDUAL_ENROLLMENT enrollment;
    enrollment.device_id = deviceId;
    enrollment.registration_id = registrationId;

    char* json;
    json = individualEnrollment_toJson(&enrollment);
    printf("%s\n", json);

    PROVISIONING_SERVICE_CLIENT_HANDLE prov_sc = prov_sc_create_from_connection_string(connectionString);
    prov_sc_create_or_update_individual_enrollment(prov_sc, &enrollment);

    return result;
}
