// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/envvariable.h"
#include "iothub_module_client_ll.h"
#include "internal/iothub_client_private.h"
#include "azure_prov_client/iothub_security_factory.h"


static const char* ENVIRONMENT_VAR_EDGEHUBCONNECTIONSTRING = "EdgeHubConnectionString";
static const char* ENVIRONMENT_VAR_EDGEAUTHSCHEME = "IOTEDGE_AUTHSCHEME";
static const char* ENVIRONMENT_VAR_EDGEDEVICEID =  "IOTEDGE_DEVICEID";
static const char* ENVIRONMENT_VAR_EDGEMODULEID =  "IOTEDGE_MODULEID";
static const char* ENVIRONMENT_VAR_EDGEHUBHOSTNAME =  "IOTEDGE_IOTHUBHOSTNAME";
static const char* ENVIRONMENT_VAR_EDGEGATEWAYHOST = "IOTEDGE_GATEWAYHOSTNAME";

static const char* SAS_TOKEN_AUTH = "sasToken";

typedef struct EDGE_ENVIRONMENT_VARIABLES_TAG
{
    const char* connection_string;
    const char* auth_scheme;
    const char* device_id;
    const char* iothub_name;
    const char* iothub_suffix;
    const char* gatewayhostname;
    const char* module_id;
    char* iothub_buffer;
} EDGE_ENVIRONMENT_VARIABLES;

static int retrieve_edge_environment_variabes(EDGE_ENVIRONMENT_VARIABLES *edge_environment_variables)
{
    int result;
    const char* edgehubhostname;
    char* edgehubhostname_separator;

    if ((edge_environment_variables->connection_string = environment_get_variable(ENVIRONMENT_VAR_EDGEHUBCONNECTIONSTRING)) != NULL)
    {
        // If a connection string is set, we use it and ignore all other environment variables.
        result = 0;
    }
    else
    {
        if ((edge_environment_variables->auth_scheme = environment_get_variable(ENVIRONMENT_VAR_EDGEAUTHSCHEME)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEAUTHSCHEME);
            result = __FAILURE__;
        }
        else if (strcmp(edge_environment_variables->auth_scheme, SAS_TOKEN_AUTH) != 0)
        {
            LogError("Environment %s was set to %s, but only support for %s", ENVIRONMENT_VAR_EDGEAUTHSCHEME, edge_environment_variables->auth_scheme, SAS_TOKEN_AUTH);
            result = __FAILURE__;
        }
        else if ((edge_environment_variables->device_id = environment_get_variable(ENVIRONMENT_VAR_EDGEDEVICEID)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEDEVICEID);
            result = __FAILURE__;
        }
        else if ((edgehubhostname = environment_get_variable(ENVIRONMENT_VAR_EDGEHUBHOSTNAME)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEHUBHOSTNAME);
            result = __FAILURE__;
        }
        else if ((edge_environment_variables->gatewayhostname = environment_get_variable(ENVIRONMENT_VAR_EDGEGATEWAYHOST)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEGATEWAYHOST);
            result = __FAILURE__;
        }
        else if ((edge_environment_variables->module_id = environment_get_variable(ENVIRONMENT_VAR_EDGEMODULEID)) == NULL)
        {
            LogError("Environment %s not set", ENVIRONMENT_VAR_EDGEMODULEID);
            result = __FAILURE__;
        }
        // Make a copy of just ENVIRONMENT_VAR_EDGEHUBHOSTNAME.  We need to make changes in place (namely inserting a '\0')
        // and can't do this with system environment variable safely.
        else if (mallocAndStrcpy_s(&edge_environment_variables->iothub_buffer, edgehubhostname) != 0)
        {
            LogError("Unable to copy buffer");
            result = __FAILURE__;
        }
        else if ((edgehubhostname_separator = strchr(edge_environment_variables->iothub_buffer, '.')) == NULL)
        {
            LogError("Environment edgehub %s invalid, requires '.' separator", edge_environment_variables->iothub_buffer);
            result = __FAILURE__;
        }
        else if (*(edgehubhostname_separator + 1) == 0)
        {
            LogError("Environment edgehub %s invalid, no content after '.' separator", edge_environment_variables->iothub_buffer);
            result = __FAILURE__;
        }
        else
        {
            edge_environment_variables->iothub_name = edge_environment_variables->iothub_buffer;
            *edgehubhostname_separator = 0;
            edge_environment_variables->iothub_suffix = edgehubhostname_separator + 1;
            result = 0;
        }
    }

    return result;
}

IOTHUB_MODULE_CLIENT_LL_HANDLE IoTHubModuleClient_LL_CreateFromEnvironment(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_MODULE_CLIENT_LL_HANDLE result;
    EDGE_ENVIRONMENT_VARIABLES edge_environment_variables;

    memset(&edge_environment_variables, 0, sizeof(edge_environment_variables));

    if (retrieve_edge_environment_variabes(&edge_environment_variables) != 0)
    {
        LogError("retrieve_edge_environment_variabes failed");
        result = NULL;
    }
    // The presence of a connection string environment variable means we use it, ignoring other settings
    else if (edge_environment_variables.connection_string != NULL)
    {
        result = IoTHubModuleClient_LL_CreateFromConnectionString(edge_environment_variables.connection_string, protocol);
    }
    else if (iothub_security_init(IOTHUB_SECURITY_TYPE_HTTP_EDGE) != 0)
    {
        LogError("iothub_security_init failed");
        result = NULL;
    }
    else
    {
        IOTHUB_CLIENT_CONFIG client_config;

        memset(&client_config, 0, sizeof(client_config));
        client_config.protocol = protocol;
        client_config.deviceId = edge_environment_variables.device_id;
        client_config.iotHubName =  edge_environment_variables.iothub_name;
        client_config.iotHubSuffix = edge_environment_variables.iothub_suffix;
        client_config.protocolGatewayHostName = edge_environment_variables.gatewayhostname;
        result = (IOTHUB_MODULE_CLIENT_LL_HANDLE)IoTHubClientCore_LL_CreateFromEnvironment(&client_config, edge_environment_variables.module_id);
    }

    free(edge_environment_variables.iothub_buffer);
    return result;
}



