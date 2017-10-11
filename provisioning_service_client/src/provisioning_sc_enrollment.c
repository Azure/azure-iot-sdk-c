// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/xlogging.h"

#include "provisioning_sc_enrollment.h"
#include "parson.h"

static void* ENROLLMENT_JSON_DEFUALT_VALUE_NULL = NULL;
static const char* ENROLLMENT_JSON_KEY_REG_ID = "registrationId";
static const char* ENROLLMENT_JSON_KEY_DEVICE_ID = "deviceId";
//registration status?
//attestation?
static const char* ENROLLMENT_JSON_KEY_HOSTNAME = "iotHubHostName";
//twin state?
static const char* ENROLLMENT_JSON_KEY_ETAG = "etag";
static const char* ENROLLMENT_JSON_KEY_GENERATION_ID = "generationId";
//provisioning status?
static const char* ENROLLMENT_JSON_KEY_CREATED_TIME = "createdDateTimeUtc";
static const char* ENROLLMENT_JSON_KEY_UPDATED_TIME = "lastUpdatedDateTimeUtc";


char* individualEnrollment_toJson(const INDIVIDUAL_ENROLLMENT* enrollment)
{
    char * result = NULL;
    
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;
    JSON_Status jsonStatus;

    if (enrollment == NULL)
    {
        LogError("enrollment is NULL");
        result = NULL;
    }
    else if ((root_value = json_value_init_object()) == NULL)
    {
        LogError("json_value_init_object failed");
        result = NULL;
    }
    else if ((root_object = json_value_get_object(root_value)) == NULL)
    {
        LogError("json_value_get_object failed");
        result = NULL;
    }
    else if (json_object_set_string(root_object, ENROLLMENT_JSON_KEY_REG_ID, enrollment->registration_id) != JSONSuccess)
    {
        LogError("Failed to set registrationId in JSON string");
        result = NULL;
    }
    else if (json_object_set_string(root_object, ENROLLMENT_JSON_KEY_DEVICE_ID, enrollment->device_id) != JSONSuccess)
    {
        LogError("Failed to set deviceId in JSON String");
    }
    //add more fields

    else
    {

        if ((result = json_serialize_to_string(root_value)) == NULL)
        {
            LogError("json_serialize_to_string_failed");
            result = NULL;
        }

    }

    if ((jsonStatus = json_object_clear(root_object)) != JSONSuccess)
    {
        LogError("json_object_clear failed");
        result = NULL;
    }
    if (root_value != NULL)
    {
        json_value_free(root_value);
    }

    return result;
}
