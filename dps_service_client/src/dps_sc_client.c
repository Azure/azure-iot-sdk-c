// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"

#include "dps_sc_client.h"

#define UNREFERENCED_PARAMETER(x) x

static const char* enrollments_prefix = "/enrollments/";
static const char* enrollment_groups_prefix = "/enrollmentGroups/";
static const char* registrations_prefix = "/registrations/";

//consider substructure representing SharedAccessSignature?
typedef struct DPS_SC_TAG
{
    char* dps_uri;
    char* key_name;
    char* access_key;
    
} DPS_SC;


DPS_SC_HANDLE dps_sc_create_from_connection_string(const char* conn_string)
{
    UNREFERENCED_PARAMETER(conn_string);

    //in implementation, parse these values from the conn_string
    const char* hostname = "hostname";
    const char* key_name = "key_name";
    const char* key = "key";

    DPS_SC* result;
    
    if (hostname == NULL || key_name == NULL || key == NULL)
    {
        LogError("invalid parameter hostname: %p, key_name: %p, key: %p", hostname, key_name, key);
        result = NULL;
    }
    else if ((result = malloc(sizeof(DPS_SC))) == NULL)
    {
        LogError("Allocation of dps service client failed");
    }
    else
    {
        memset(result, 0, sizeof(DPS_SC));
        if (mallocAndStrcpy_s(&result->dps_uri, hostname) != 0)
        {
            LogError("Failure allocating of dps uri");
            free(result);
        }
        else if (mallocAndStrcpy_s(&result->key_name, key_name) != 0)
        {
            LogError("Failure allocating of keyname");
            free(result);
        }
        else if (mallocAndStrcpy_s(&result->access_key, key) != 0)
        {
            LogError("Failure allocating of access key");
            free(result);
        }
    }
    return result;
}


void dps_sc_destroy(DPS_SC_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle->dps_uri);
        free(handle->key_name);
        free(handle->access_key);
        free(handle);
    }
}

int dps_sc_create_or_update_enrollment(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT* enrollment)
{
    //1. establish path
    //2. construct headers
    //3. make PUT call

    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment);
    return 0;
}

int dps_sc_delete_enrollment(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_enrollment(DPS_SC_HANDLE handle, const char* id, ENROLLMENT* enrollment)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment);
    return 0;
}

int dps_sc_delete_device_registration_status(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_device_registration_status(DPS_SC_HANDLE handle, const char* id, DEVICE_REGISTRATION_STATUS* reg_status)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(reg_status);
    return 0;
}

int dps_sc_create_or_update_enrollment_group(DPS_SC_HANDLE handle, const char* id, const ENROLLMENT_GROUP* enrollment_group)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment_group);
    return 0;
}

int dps_sc_delete_enrollment_group(DPS_SC_HANDLE handle, const char* id)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    return 0;
}

int dps_sc_get_enrollment_group(DPS_SC_HANDLE handle, const char* id, ENROLLMENT_GROUP* enrollment_group)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enrollment_group);
    return 0;
}
