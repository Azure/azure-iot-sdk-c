// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// Core header files for C and IoTHub layer
//
#include <stdio.h>
#include <stdlib.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/xlogging.h>
#include "parson.h"

//
// Header files for interacting with DigitalTwin layer.
//
#include "digitaltwin_device_client.h"
#include "digitaltwin_interface_client.h"
#include "digitaltwin_sample_model_definition.h"

//
// TODO`s: Configure core settings of your Digital Twin sample interface
//

// TODO: Fill in DIGITALTWIN_SAMPLE_INTERFACE_ID_TO_PUBLISH. E.g. 
#define DIGITALTWIN_SAMPLE_INTERFACE_ID_TO_PUBLISH "urn:YOUR_COMPANY_NAME_HERE:EnvironmentalSensor:1"

//
// END TODO section
//

// Path to the interface dtdl on Device, 
// This is a sample json which is used to show how to add/publish its content 
// So that SDK can return it when requested for this interface i.e. the built-in environmentalSensor interface
static const char* dtdl_path = "EnvironmentalSensor.interface.json";

// Digital Twin iterface which we want to add for the device
static const char toPublish_InterfaceName[] = DIGITALTWIN_SAMPLE_INTERFACE_ID_TO_PUBLISH;

// Read file AND store in memory
static char *DigitalTwinSampleModelDefinition_ReadFile(const char *dtdl_file_path)
{
    JSON_Value *dtdlJSONValue = json_parse_file(dtdl_file_path);
    char* result = NULL;
    
    if (dtdlJSONValue != NULL)
    {
        result = json_serialize_to_string(dtdlJSONValue);
        json_value_free(dtdlJSONValue);
    }

    return result;
}

// DigitalTwinSampleModelDefinition_ParseAndPublish will read given file, serialize using parson and publish interface
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleModelDefinition_ParseAndPublish(MODEL_DEFINITION_CLIENT_HANDLE md_handle)
{
    char *addIface = DigitalTwinSampleModelDefinition_ReadFile(dtdl_path);
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;

    if (addIface != NULL)
    {
        if (DIGITALTWIN_CLIENT_OK != (result = DigitalTwin_ModelDefinition_Publish_Interface(toPublish_InterfaceName, addIface, md_handle)))
        {
            LogError("MODEL_DEFINITION_INTERFACE: DigitalTwin_ModelDefinition_Publish_Interface failed, error=<%s>", MU_ENUM_TO_STRING(DIGITALTWIN_CLIENT_RESULT, result));
        }
        else
        {
            result = DIGITALTWIN_CLIENT_OK;
            LogInfo("MODEL_DEFINITION_INTERFACE: Published dtdl for <%s>", toPublish_InterfaceName);
        }
    }
    else
    {
        LogError("MODEL_DEFINITION_INTERFACE: Could not read dtdl file");
    }
    return result;
}

