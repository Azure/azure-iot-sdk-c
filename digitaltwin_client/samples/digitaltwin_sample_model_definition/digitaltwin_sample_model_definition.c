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

//
// Header files for interacting with DigitalTwin layer.
//
#include "digitaltwin_device_client.h"
#include "digitaltwin_interface_client.h"
#include "digitaltwin_sample_model_definition.h"

// Path to the interface dtdl on Device, 
// This is a sample json which is used to show how to add/publish its content 
// So that SDK can return it when requested for this interface i.e. environmentalSensor
static const char* dtdl_path = "EnvironmentalSensor.interface.json";

// Digital Twin iterface which we want to add for the device
static const char toPublish_InterfaceName[] = "urn:<YOUR_COMPANY_NAME_HERE>:EnvironmentalSensor:1";

// Read file AND store in memory
static char *DigitalTwinSampleModelDefinition_ReadFile(const char *dtdl_file_path)
{
    /* Pointer to the file */
    FILE *dtdlfp = NULL;
    char* result;

    if (NULL == (dtdlfp = fopen(dtdl_file_path, "r")))
    {
        LogError("Error could not open file for reading %s", dtdl_file_path);
        result = NULL;
    }
    else if (0 != fseek(dtdlfp, 0, SEEK_END))
    {
        LogError("fseek on file %s fails, errno=%d", dtdl_file_path, errno);
        result = NULL;
    }
    else
    {
        long filesize = ftell(dtdlfp);
        if (0 > filesize)
        {
            LogError("ftell fails reading %s, errno=%d",  dtdl_file_path, errno);
            result = NULL;
        }
        else if (0 == filesize)
        {
            LogError("file %s is 0 bytes, which is not valid dtdl json",  dtdl_file_path);
            result = NULL;
        }
        else
        {
            rewind(dtdlfp);

            if (NULL == (result = calloc(1, ((size_t)filesize) + 1)))
            {
                 LogError("Cannot allocate %lu bytes", (unsigned long)filesize);
            }
            else if ((0 == fread(result, 1, filesize, dtdlfp)) || (0 != ferror(dtdlfp)))
            {
                LogError("fread failed on file %s, errno=%d", dtdl_file_path, errno);
                free(result);
                result = NULL;
            }
        }
    }    

    if (NULL != dtdlfp)
    {
        (void)fclose(dtdlfp);
    }

    return result;
}

// DigitalTwinSampleModelDefinition_ParseAndPublish will read given file, serialize using parson and publish interface
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleModelDefinition_ParseAndPublish(MODEL_DEFINITION_CLIENT_HANDLE md_handle)
{
    char *addIface = DigitalTwinSampleModelDefinition_ReadFile(dtdl_path);
    DIGITALTWIN_CLIENT_RESULT result = DIGITALTWIN_CLIENT_ERROR;

    if (NULL != addIface)
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

