// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTSECURITY_COLLECTOR_NETWORK_H
#define IOTSECURITY_COLLECTOR_NETWORK_H

#include "collector.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#else
#endif

#define NETID_RESULT_VALUES    \
    NETID_TCP,              \
    NETID_UDP,              \
    NETID_RAW,              \
    NETID_U_DGR,            \
    NETID_U_STR,            \
    NETID_P_RAW,            \
    NETID_P_DGR,            \
    NETID_NL

    MU_DEFINE_ENUM(NETID_RESULT, NETID_RESULT_VALUES)

MOCKABLE_FUNCTION(, CollectorResult, CollectorNetwork, JSON_Object, *root);

#ifdef __cplusplus
}
#endif

#endif /* IOTSECURITY_COLLECTOR_NETWORK_H */
