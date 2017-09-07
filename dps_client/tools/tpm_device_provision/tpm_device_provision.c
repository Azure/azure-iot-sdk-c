// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/base64.h"

#include "azure_hub_modules/dps_sec_client.h"
#include "azure_hub_modules/secure_device_factory.h"

typedef struct DPS_REGISTRATION_INFO_TAG
{
    BUFFER_HANDLE endorsement_key;
    char* registration_id;
} DPS_REGISTRATION_INFO;

static int gather_registration_info(DPS_REGISTRATION_INFO* reg_info)
{
    int result;

    DPS_SEC_HANDLE security_handle = dps_sec_create();
    if (security_handle == NULL)
    {
        (void)printf("failed creating security device handle\r\n");
        result = __LINE__;
    }
    else
    {
        if ((reg_info->endorsement_key = dps_sec_get_endorsement_key(security_handle)) == NULL)
        {
            (void)printf("failed getting endorsement key from device\r\n");
            result = __LINE__;
        }
        else if ((reg_info->registration_id = dps_sec_get_registration_id(security_handle)) == NULL)
        {
            (void)printf("failed getting endorsement key from device\r\n");
            BUFFER_delete(reg_info->endorsement_key);
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        dps_sec_destroy(security_handle);
    }
    return result;
}

int main()
{
    int result;
    DPS_REGISTRATION_INFO reg_info;
    memset(&reg_info, 0, sizeof(reg_info));

    (void)printf("Gathering the registration information...\r\n");
    if (platform_init() == 0 && dps_secure_device_init() == 0)
    {
        if (gather_registration_info(&reg_info) != 0)
        {
            result = __LINE__;
        }
        else
        {
            STRING_HANDLE encoded_ek;
            if ((encoded_ek = Base64_Encoder(reg_info.endorsement_key)) == NULL)
            {
                (void)printf("Failure base64 encoding ek");
                result = __LINE__;
            }
            else
            {
                (void)printf("\r\nRegistration Id:\r\n%s\r\n", reg_info.registration_id);
                (void)printf("\r\nEndorsement Key:\r\n%s\r\n", STRING_c_str(encoded_ek));
                STRING_delete(encoded_ek);
                result = 0;
            }
            BUFFER_delete(reg_info.endorsement_key);
            free(reg_info.registration_id);
        }
        dps_secure_device_deinit();
        platform_deinit();
    }
    else
    {
        (void)printf("Failed calling platform_init\r\n");
        result = __LINE__;
    }

    (void)printf("\r\nPress any key to continue:\r\n");
    (void)getchar();
    return result;
}