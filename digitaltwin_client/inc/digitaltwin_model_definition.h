// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef DIGITALTWIN_MODEL_DEFINITION_H
#define DIGITALTWIN_MODEL_DEFINITION_H

#include <stdio.h>
#include <stdlib.h>
#include "umock_c/umock_c_prod.h"
#include "digitaltwin_client_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MODEL_DEFINITION_CLIENT_TAG * MODEL_DEFINITION_CLIENT_HANDLE;

/**
    * @brief   DigitalTwin_ModelDefinition_Create creates MODEL DEFINITION CLIENT handle 
    *
    * @return  DIGITALTWIN_CLIENT_RESULT
*/

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_ModelDefinition_Create, MODEL_DEFINITION_CLIENT_HANDLE *, handle, DIGITALTWIN_INTERFACE_CLIENT_HANDLE *, interfaceHandle);

/**
    * @brief   DigitalTwin_ModelDefinition_Destroy creates MODEL DEFINITION CLIENT handle
    *
    * @return  MODEL_DEFINITION_CLIENT_HANDLE
*/

MOCKABLE_FUNCTION(, void, DigitalTwin_ModelDefinition_Destroy, MODEL_DEFINITION_CLIENT_HANDLE, handle);

/**
    * @brief   DigitalTwin_ModelDefinition_Publish_Interface publishes a given interface with the corresponding dtdl
    *
    * @return  DIGITALTWIN_CLIENT_RESULT for Success or Failure
*/

MOCKABLE_FUNCTION(, DIGITALTWIN_CLIENT_RESULT, DigitalTwin_ModelDefinition_Publish_Interface, const char *, interfaceId, char *, data, MODEL_DEFINITION_CLIENT_HANDLE, mdHandle);

/**
    * @brief   DigitalTwin_ModelDefinition_Get_Data gets definition langauge json stored in memory
    *
    * @return  char * to dtdl content or NULL if content not found.
*/

MOCKABLE_FUNCTION(, const char *, DigitalTwin_ModelDefinition_Get_Data, const char *, interfaceId, MODEL_DEFINITION_CLIENT_HANDLE, mdHandle);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_MODEL_DEFINITION_H
