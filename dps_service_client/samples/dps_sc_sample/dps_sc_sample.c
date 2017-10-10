// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "dps_sc_client.h"

int main()
{
    int result;
    result = 0;

    const char* connectionString = "[IoTHub Connection String]";
    const char* deviceId = "[Device Id]";
    const char* registrationId = "[Registration Id]";

    ENROLLMENT enrollment;
    enrollment.device_id = deviceId;
    enrollment.registration_id = registrationId;

    char* json;
    json = enrollment_toJson(&enrollment);
    printf("%s\n", json);

    DPS_SC_HANDLE dps_sc = dps_sc_create_from_connection_string(connectionString);
    dps_sc_create_or_update_enrollment(dps_sc, registrationId, &enrollment);

    return result;
}
