#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include("${CMAKE_CURRENT_LIST_DIR}/uamqpTargets.cmake")

get_target_property(UAMQP_INCLUDES uamqp INTERFACE_INCLUDE_DIRECTORIES)

set(UAMQP_INCLUDES ${UAMQP_INCLUDES} CACHE INTERNAL "")