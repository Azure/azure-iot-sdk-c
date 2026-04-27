#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(UNIX)
    if(${use_http})
        include(CMakeFindDependencyMacro)
        find_dependency(CURL)
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/azure_c_shared_utilityTargets.cmake")

get_target_property(AZURE_C_SHARED_UTILITY_INCLUDES aziotsharedutil INTERFACE_INCLUDE_DIRECTORIES)

set(AZURE_C_SHARED_UTILITY_INCLUDES ${AZURE_C_SHARED_UTILITY_INCLUDES} CACHE INTERNAL "")

include("${CMAKE_CURRENT_LIST_DIR}/azure_c_shared_utilityFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_build_rules.cmake")
