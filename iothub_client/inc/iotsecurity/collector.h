// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef COLLECTOR_H
#define COLLECTOR_H

#ifdef __cplusplus
extern "C"
{
#else
#endif

typedef enum _CollectorResult {
    COLLECTOR_OK,
    COLLECTOR_EXCEPTION,
    COLLECTOR_PARSE_EXCEPTION,
    COLLECTOR_MEMORY_EXCEPTION,
    COLLECTOR_NOT_SUPPORTED_EXCEPTION
} CollectorResult;

#ifdef __cplusplus
}
#endif

#endif /* COLLECTOR_H */
