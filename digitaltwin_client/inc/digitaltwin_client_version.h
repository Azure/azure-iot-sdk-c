// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**  
  @file digitaltwin_client_version.h

  @brief  Header for retrieving the Digital Twin SDK's version.
*/


#ifndef DIGITALTWIN_CLIENT_VERSION_H
#define DIGITALTWIN_CLIENT_VERSION_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

/** @brief Pre-processor friendly representation of Digital Twin Client version. */
#define DIGITALTWIN_CLIENT_SDK_VERSION "0.9.0"

#ifdef __cplusplus
extern "C"
{
#endif

/**
  @brief    Returns a pointer to a null terminated string containing the current Digital Twin Client SDK version.

  @remarks  The version returned is of the Digital Twin Client SDK itself.  This is not to be confused with
            the version of the underlying IoTHub Device SDK, which acts as the transport for the Digital Twin.
            Although in general these versions will be incremented at the same time, they technically do not have to be.
 
  @remarks  The caller *MUST NOT* free() this string.  The value returned is a simple static that the Digital Twin SDK itself owns the memory of.
 
  @return  Pointer to a null terminated string containing the current Digital Twin Client SDK version.

  @see     IoTHubClient_GetVersionString
*/

MOCKABLE_FUNCTION(, const char*, DigitalTwin_Client_GetVersionString);

#ifdef __cplusplus
}
#endif

#endif // DIGITALTWIN_CLIENT_VERSION_H
