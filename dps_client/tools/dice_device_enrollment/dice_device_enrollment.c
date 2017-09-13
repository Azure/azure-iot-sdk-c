// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"

#include "dps_hsm_riot.h"

#define MAX_VALIDATION_CODE_LEN         48

static char* get_user_input(const char* text_value, int max_len)
{
    char* result = malloc(max_len + 1);
    if (result == NULL)
    {
        (void)printf("failed to allocate buffer\r\n");
        return result;
    }
    else
    {
        int index = 0;
        memset(result, 0, max_len + 1);
        printf("%s", text_value);
        // trim the leading spaces
        while (1)
        {
            int c = getchar();
            if (c == EOF || c == 0xA)
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
            if (isspace(input) || input == EOF || input == 0xA)
            {
                break;
            }
            result[index++] = (char)input;
            if (index == max_len)
            {
                // Will need to clear out the remaining buffer
                while (input != EOF && input != 0xA)
                {
                    input = getchar();
                }
                break;
            }
        }

        size_t code_len = strlen(result);
        if (code_len != (size_t)max_len)
        {
            (void)printf("Invalid validation code length specified\r\n");
            free(result);
            result = NULL;
        }
        (void)printf("\r\n");
    }
    return result;
}

static int alias_certificate_info(DPS_SECURE_DEVICE_HANDLE dps_handle)
{
    int result;
    char* certificate;

    if ((certificate = dps_hsm_riot_get_certificate(dps_handle)) == NULL)
    {
        (void)printf("Failure getting root certificate\r\n");
        result = __LINE__;
    }
    else
    {
        (void)printf("Device certificate:\r\n%s\r\n", certificate);
        free(certificate);
        result = 0;
    }
    return result;
}

static int ca_root_certificate_info(DPS_SECURE_DEVICE_HANDLE dps_handle)
{
    int result;
    char* leaf_certificate = NULL;
    char* root_cert = NULL;
    char* common_name = NULL;

    if ((root_cert = dps_hsm_riot_get_root_cert(dps_handle)) == NULL)
    {
        (void)printf("Failure getting root certificate\r\n");
        result = __LINE__;
    }
    else
    {
        (void)printf("root cert:\r\n%s\r\n", root_cert);

        if ((common_name = get_user_input("Enter the Validation Code (Press enter when finished): ", MAX_VALIDATION_CODE_LEN)) == NULL)
        {
            (void)printf("Failure with validation code\r\n");
            result = __LINE__;
        }
        else if ((leaf_certificate = dps_hsm_riot_create_leaf_cert(dps_handle, common_name)) == NULL)
        {
            (void)printf("Failure setting leaf certificate\r\n");
            result = __LINE__;
        }
        else
        {
            (void)printf("Leaf Certificate:\r\n%s\r\n", leaf_certificate);
            result = 0;
        }
        free(root_cert);
        free(leaf_certificate);
        free(common_name);
    }

    return result;
}

int main()
{
    int result;
    DPS_SECURE_DEVICE_HANDLE dps_handle;

    if (platform_init() == 0 && initialize_riot_system() == 0)
    {
        if ((dps_handle = dps_hsm_riot_create()) == NULL)
        {
            (void)printf("Failure getting common name\r\n");
            result = __LINE__;
        }
        else
        {
            char* enroll_type = get_user_input("Would you like to do Individual (i) or Group (g) enrollments: ", 1);
            if (enroll_type != NULL)
            {
                if (enroll_type[0] == 'g' || enroll_type[0] == 'G')
                {
                    if (ca_root_certificate_info(dps_handle) != 0)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else
                {
                    if (alias_certificate_info(dps_handle) != 0)
                    {
                        result = __LINE__;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                free(enroll_type);
            }
            else
            {
                (void)printf("Invalid option specified\r\n");
                result = __LINE__;
            }
            dps_hsm_riot_destroy(dps_handle);
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