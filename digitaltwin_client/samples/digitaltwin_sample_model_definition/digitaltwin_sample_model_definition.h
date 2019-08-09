// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Header file for sample for model definition interface.

#ifndef DIGITALTWIN_SAMPLE_MODEL_DEFINITION_H
#define DIGITALTWIN_SAMPLE_MODEL_DEFINITION_H

#include "digitaltwin_interface_client.h"
#include "digitaltwin_model_definition.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Stores an interface definition language for given interface.
DIGITALTWIN_CLIENT_RESULT DigitalTwinSampleModelDefinition_ParseAndPublish(MODEL_DEFINITION_CLIENT_HANDLE);

// Closes down resources associated with model definition interface.
void DigitalTwinSampleModelDefinition_Close(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif


#endif // DIGITALTWIN_SAMPLE_MODEL_DEFINITION_H
