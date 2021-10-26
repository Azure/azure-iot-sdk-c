// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/gballoc.h"

#include "schemalib.h"
#include "codefirst.h"
#include "schema.h"
#include "datamarshaller.h"
#include "datapublisher.h"
#include <stddef.h>
#include "azure_c_shared_utility/xlogging.h"
#include "iotdevice.h"

#define DEFAULT_CONTAINER_NAME  "Container"

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(SERIALIZER_RESULT, SERIALIZER_RESULT_VALUES);

typedef enum AGENT_STATE_TAG
{
    AGENT_NOT_INITIALIZED,
    AGENT_INITIALIZED
} AGENT_STATE;

static AGENT_STATE g_AgentState = AGENT_NOT_INITIALIZED;

SERIALIZER_RESULT serializer_init(const char* overrideSchemaNamespace)
{
    SERIALIZER_RESULT result;

    if (g_AgentState != AGENT_NOT_INITIALIZED)
    {
        result = SERIALIZER_ALREADY_INIT;
        LogError("(result = %s)", MU_ENUM_TO_STRING(SERIALIZER_RESULT, result));
    }
    else
    {
        if (CodeFirst_Init(overrideSchemaNamespace) != CODEFIRST_OK)
        {
            result = SERIALIZER_CODEFIRST_INIT_FAILED;
            LogError("(result = %s)", MU_ENUM_TO_STRING(SERIALIZER_RESULT, result));
        }
        else
        {
            g_AgentState = AGENT_INITIALIZED;

            result = SERIALIZER_OK;
        }
    }

    return result;
}

void serializer_deinit(void)
{
    if (g_AgentState == AGENT_INITIALIZED)
    {
        CodeFirst_Deinit();
    }

    g_AgentState = AGENT_NOT_INITIALIZED;
}

SERIALIZER_RESULT serializer_setconfig(SERIALIZER_CONFIG which, void* value)
{
    SERIALIZER_RESULT result;

    if (value == NULL)
    {
        result = SERIALIZER_INVALID_ARG;
    }
    else if (which == SerializeDelayedBufferMaxSize)
    {
        DataPublisher_SetMaxBufferSize(*(size_t*)value);
        result = SERIALIZER_OK;
    }
    else
    {
        result = SERIALIZER_INVALID_ARG;
    }

    return result;
}
