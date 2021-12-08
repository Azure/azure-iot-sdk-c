// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"

#include "hsm_client_data.h"
#include "hsm_client_riot.h"

#define MAX_VALIDATION_CODE_LEN         48
static const char* const END_CERT_MARKER = "-----END CERTIFICATE-----";

static char* get_user_input(const char* text_value, int max_len)
{
    int result_len = max_len + 1;
    if (result_len <= 1)
    {
        (void)printf("invalid max_len size\r\n");
        return NULL;
    }

    char* result = malloc(result_len);
    if (result == NULL)
    {
        (void)printf("failed to allocate buffer\r\n");
        return result;
    }
    else
    {
        int index = 0;
        memset(result, 0, result_len);
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

static const char* retrieve_next_cert(const char* cert_list)
{
    const char* result = NULL;
    const char* iterator = cert_list;
    bool after_begin = false;
    size_t end_marker = strlen(END_CERT_MARKER);
    do
    {
        if (after_begin && *iterator == '-')
        {
            if (memcmp(iterator, END_CERT_MARKER, end_marker) == 0)
            {
                iterator += end_marker;
                while (*iterator != '\0')
                {
                    if (*iterator == '\n')
                    {
                        result = iterator+1;
                        break;
                    }
                    iterator++;
                }
                break;
            }
        }
        else if (!after_begin && *iterator == '\n')
        {
            after_begin = true;
        }
        iterator++;
    } while (*iterator != '\0');
    return result;
}

static int alias_certificate_info(HSM_CLIENT_HANDLE hsm_handle)
{
    int result;
    char* certificate;

    if ((certificate = hsm_client_riot_get_certificate(hsm_handle)) == NULL)
    {
        (void)printf("Failure getting root certificate\r\n");
        result = __LINE__;
    }
    else
    {
        // This may contain 2 certificates, but we only need to show the
        // first cert.  Get the 2nd cert
        const char* next_cert = retrieve_next_cert(certificate);
        if (next_cert != NULL)
        {
            // Let's get the length of the first cert
            int init_len = (int)(next_cert-certificate);
            (void)printf("Device certificate:\r\n%.*s\r\n", init_len, certificate);
        }
        else
        {
            (void)printf("Device certificate:\r\n%s\r\n", certificate);
        }
        free(certificate);
        result = 0;
    }
    return result;
}

static int ca_root_certificate_info(HSM_CLIENT_HANDLE hsm_handle)
{
    int result;
    char* leaf_certificate = NULL;
    char* root_cert = NULL;
    char* common_name = NULL;

    if ((root_cert = hsm_client_riot_get_root_cert(hsm_handle)) == NULL)
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
        else if ((leaf_certificate = hsm_client_riot_create_leaf_cert(hsm_handle, common_name)) == NULL)
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

    if (platform_init() == 0 && initialize_hsm_system() == 0)
    {
        HSM_CLIENT_HANDLE hsm_handle;
        if ((hsm_handle = hsm_client_riot_create()) == NULL)
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
                    if (ca_root_certificate_info(hsm_handle) != 0)
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
                    if (alias_certificate_info(hsm_handle) != 0)
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
            hsm_client_riot_destroy(hsm_handle);
        }
        deinitialize_hsm_system();
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