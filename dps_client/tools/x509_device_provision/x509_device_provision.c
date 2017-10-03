// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"

#include "azure_hub_modules/dps_sec_client.h"
#include "azure_hub_modules/secure_device_factory.h"

typedef struct DPS_REGISTRATION_INFO_TAG
{
    char* root_cert;
    char* device_cert;
    char* device_key;
    char* root_key;
    char* alias_cert;
} DPS_REGISTRATION_INFO;

static int gather_registration_info(DPS_REGISTRATION_INFO* reg_info)
{
    int result;

    DPS_SEC_HANDLE security_handle = dps_sec_create();
    if (security_handle == NULL)
    {
        (void)printf("failed secure_device_riot_create");
        result = __LINE__;
    }
    else
    {
        if ((reg_info->device_cert = dps_sec_get_signer_cert(security_handle)) == NULL)
        {
            (void)printf("failed allocating signer_cert");
            result = __LINE__;
        }
        /*else if ((reg_info->device_key = dps_sec_get_signer_key(security_handle)) == NULL)
        {
            (void)printf("failed allocating signer_cert");
            result = __LINE__;
        }*/
        else if ((reg_info->root_cert = dps_sec_get_root_cert(security_handle)) == NULL)
        {
            (void)printf("failed allocating signer_cert");
            result = __LINE__;
        }
        else if ((reg_info->root_key = dps_sec_get_root_key(security_handle)) == NULL)
        {
            (void)printf("failed allocating signer_cert");
            result = __LINE__;
        }
        else if ((reg_info->alias_cert = dps_sec_get_certificate(security_handle)) == NULL)
        {
            (void)printf("failed allocating signer_cert");
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
            (void)printf("\r\nRoot Certificate:\r\n%s\r\n", reg_info.root_cert);
            (void)printf("Root Private Key:\r\n%s\r\n", reg_info.root_key);
            (void)printf("Signer Certificate:\r\n%s\r\n", reg_info.device_cert);
            //(void)printf("Signer key:\r\n%s\r\n", reg_info.device_key);
            (void)printf("Alias Certificate (Device):\r\n%s\r\n", reg_info.alias_cert);
            result = 0;

            free(reg_info.device_cert);
            free(reg_info.device_key);
            free(reg_info.root_key);
            free(reg_info.root_cert);
            free(reg_info.alias_cert);
        }
        dps_secure_device_deinit();
        platform_deinit();
    }
    else
    {
        (void)printf("Failed calling platform_init\r\n");
        result = __LINE__;
    }

    (void)printf("Press any key to continue:\r\n");
    (void)getchar();
    return result;
}