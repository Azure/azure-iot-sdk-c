// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/uuid.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/hmacsha256.h"
#include "azure_c_shared_utility/azure_base64.h"

typedef struct REGISTRATION_INFO_TAG
{
    BUFFER_HANDLE endorsement_key;
    char* registration_id;
} REGISTRATION_INFO;

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
        if (code_len > (size_t)max_len)
        {
            (void)printf("Invalid validation code length specified\r\n");
            free(result);
            result = NULL;
        }
        (void)printf("\r\n");
    }
    return result;
}

static int construct_individual_enrollment(void)
{
    int result = 0;

    UUID_T uuid;
    if (UUID_generate(&uuid) == 0)
    {
        STRING_HANDLE device_key = Azure_Base64_Encode_Bytes(uuid, 16);
        if (device_key == NULL)
        {
            (void)printf("Failure generating key info\r\n");
            result = __LINE__;
        }
        else
        {
            (void)printf("Symmetric Key: %s\r\n", STRING_c_str(device_key));
            STRING_delete(device_key);
            result = 0;
        }
    }
    else
    {
        (void)printf("Failure generating key id\r\n");
        result = __LINE__;
    }
    return result;
}

static int construct_group_enrollment(void)
{
    int result;
    char* group_key = NULL;
    char* device_name = NULL;
    if ((group_key = get_user_input("Enter symmetric group key: ", 256)) == NULL)
    {
        (void)printf("Failure with symmetric group key\r\n");
        result = __LINE__;
    }
    else if ((device_name = get_user_input("Enter device name: ", 128)) == NULL)
    {
        (void)printf("Failure with device name key\r\n");
        free(group_key);
        result = __LINE__;
    }
    else
    {
        BUFFER_HANDLE decode_key;
        BUFFER_HANDLE hash;
        if ((decode_key = Azure_Base64_Decode(group_key)) == NULL)
        {
            (void)printf("Failure decoding group key\r\n");
            result = __LINE__;
        }
        else if ((hash = BUFFER_new()) == NULL)
        {
            (void)printf("Failure decoding group key\r\n");
            result = __LINE__;
            BUFFER_delete(decode_key);
        }
        else
        {
            STRING_HANDLE device_key = NULL;
            if ((HMACSHA256_ComputeHash(BUFFER_u_char(decode_key), BUFFER_length(decode_key), (const unsigned char*)device_name, strlen(device_name), hash)) != HMACSHA256_OK)
            {
                (void)printf("Failure decoding group key\r\n");
                result = __LINE__;
            }
            else
            {
                device_key = Azure_Base64_Encode(hash);
                (void)printf("Symmetric Key: %s\r\n", STRING_c_str(device_key));
                STRING_delete(device_key);
                result = 0;
            }
            BUFFER_delete(hash);
            BUFFER_delete(decode_key);
        }
        free(group_key);
        free(device_name);
    }
    return result;
}

int main()
{
    int result;
    REGISTRATION_INFO reg_info;
    memset(&reg_info, 0, sizeof(reg_info));

    (void)printf("Gathering the registration information...\r\n");
    char* enroll_type = get_user_input("Would you like to do Individual (i) or Group (g) enrollments: ", 1);
    if (enroll_type != NULL)
    {
        if (enroll_type[0] == 'g' || enroll_type[0] == 'G')
        {
            // Group enrollment
            result = construct_group_enrollment();
        }
        else
        {
            // Individual enrollment
            result = construct_individual_enrollment();
        }
        free(enroll_type);
    }
    else
    {
        (void)printf("Invalid option specified\r\n");
        result = __LINE__;
    }
    (void)printf("\r\nPress any key to continue:\r\n");
    (void)getchar();
    return result;
}