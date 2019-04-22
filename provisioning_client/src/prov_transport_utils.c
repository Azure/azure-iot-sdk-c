// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "azure_prov_client/prov_client_const.h"

// Snippit from RFC 7231 https://tools.ietf.org/html/rfc7231
// The value of this field can be either an HTTP - date or a number of
// seconds to delay after the response is received.

// Retry - After = HTTP - date / delay - seconds

// A delay - seconds value is a non - negative decimal integer, representing
// time in seconds.

// delay - seconds = 1 * DIGIT
// Two examples of its use are

// Retry-After: Fri, 31 Dec 1999 23 : 59 : 59 GMT
// Retry-After : 120
uint32_t parse_retry_after_value(const char* retry_after)
{
    uint32_t result = PROV_GET_THROTTLE_TIME;
    if (retry_after != NULL)
    {
        // Is the retry after a number
        if (retry_after[0] >= 0x30 || retry_after[0] <= 0x39)
        {
            result = atol(retry_after);
        }
        // Will need to parse the retry after for date information
    }
    return result;
}
