#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

include(CMakeFindDependencyMacro)

# aziotsharedutil's exported targets reference macro_utils_c via INTERFACE_LINK_LIBRARIES,
# so consumers must be able to resolve it before the targets file is included.
find_dependency(macro_utils_c)

if(UNIX)
    if(${use_http})
        find_dependency(CURL)
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/azure_c_shared_utilityTargets.cmake")

get_target_property(AZURE_C_SHARED_UTILITY_INCLUDES aziotsharedutil INTERFACE_INCLUDE_DIRECTORIES)

set(AZURE_C_SHARED_UTILITY_INCLUDES ${AZURE_C_SHARED_UTILITY_INCLUDES} CACHE INTERNAL "")

include("${CMAKE_CURRENT_LIST_DIR}/azure_c_shared_utilityFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_build_rules.cmake")
