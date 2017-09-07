// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"

#include "dps_hsm_riot.h"

#define MAX_VALIDATION_CODE_LEN         24

typedef struct DICE_ENROLLMENT_INFO_TAG
{
    char* leaf_certificate;
    char* common_name;
} DICE_ENROLLMENT_INFO;

static char* get_user_input()
{
    char* result = malloc(MAX_VALIDATION_CODE_LEN+1);
    if (result == NULL)
    {
        (void)printf("failed to allocate buffer\r\n");
        return result;
    }
    else
    {
        int index = 0;
        memset(result, 0, sizeof(MAX_VALIDATION_CODE_LEN+1) );
        printf("Enter the Validation Code (Press enter when finished): ");
        // trim the leading spaces
        while (1)
        {
            int c = getchar();
            if (c == EOF)
                break;
            if (!isspace(c))
            {
                ungetc(c, stdin);
                break;
            }
        }

        while (1)
        {
            int input = getchar();
            if (isspace(input) || input == EOF)
            {
                break;
            }
            result[index] = (char)input;
            if (index == MAX_VALIDATION_CODE_LEN)
            {
                break;
            }
            index++;
        }

        if (strlen(result) != MAX_VALIDATION_CODE_LEN)
        {
            (void)printf("Invalid validation code specified\r\n");
            free(result);
            result = NULL;
        }
    }
    return result;
}

static int device_certificate_info(DICE_ENROLLMENT_INFO* enrollment_info)
{
    int result;
    DPS_SECURE_DEVICE_HANDLE dps_handle;
    if ((dps_handle = dps_hsm_riot_create()) == NULL)
    {
        (void)printf("Failure getting common name\r\n");
        result = __LINE__;
    }
    else
    {
        if ((enrollment_info->common_name = get_user_input()) == NULL)
        {
            (void)printf("Failure getting common name\r\n");
            result = __LINE__;
        }
        else if ((enrollment_info->leaf_certificate = dps_hsm_riot_create_leaf_cert(dps_handle, enrollment_info->common_name)) == NULL)
        {
            (void)printf("Failure setting leaf certificate\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
        dps_hsm_riot_destroy(dps_handle);
    }
    return result;
}

int main()
{
    int result;
    DICE_ENROLLMENT_INFO enrollment_info;
    memset(&enrollment_info, 0, sizeof(enrollment_info));

    if (platform_init() == 0 && initialize_riot_system() == 0)
    {
        if (device_certificate_info(&enrollment_info) != 0)
        {
            result = __LINE__;
        }
        else
        {
            (void)printf("Leaf Certificate:\r\n%s\r\n", enrollment_info.leaf_certificate);
            result = 0;

            free(enrollment_info.leaf_certificate);
            free(enrollment_info.common_name);
        }
        deinitialize_riot_system();
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