#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

message("azure_iot_sdksConfig.cmake: Line 4: ${CMAKE_CURRENT_LIST_DIR}")
include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_sdksTargets.cmake")

get_target_property(IOTHUB_CLIENT_INCLUDES iothub_client INTERFACE_INCLUDE_DIRECTORIES)

set(IOTHUB_CLIENT_INCLUDES ${IOTHUB_CLIENT_INCLUDES} CACHE INTERNAL "")

include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_sdksFunctions.cmake")