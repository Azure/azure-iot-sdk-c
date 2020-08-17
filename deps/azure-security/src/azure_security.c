// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include "azure_security.h"

IOTHUB_MESSAGE_HANDLE AzureSecurity_GetSecurityInfoMessage()
{
	IOTHUB_MESSAGE_HANDLE security_message_handle = NULL;
    char security_info[256];
    
	// TODO (ASC Team): Implement the data collection as needed.
    if (sprintf(security_info, "<todo: collect and format security info as needed>") <= 0)
    {
        printf("Failed gathering security info\r\n");
    }
    else if ((security_message_handle = IoTHubMessage_CreateFromString(security_info)) == NULL)
	{
		printf("Failed creating security message\r\n");
	}
	else if (IoTHubMessage_SetAsSecurityMessage(security_message_handle) != IOTHUB_MESSAGE_OK)
	{
		printf("Failed setting security message flag\r\n");
		IoTHubMessage_Destroy(security_message_handle);
	}
	
	return security_message_handle;
}
