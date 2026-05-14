#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if("${CMAKE_VERSION}" VERSION_GREATER 3.0.2)
    include("${CMAKE_CURRENT_LIST_DIR}/macro_utils_cTargets.cmake")

    get_target_property(MACRO_UTILS_C_INCLUDES macro_utils_c INTERFACE_INCLUDE_DIRECTORIES)

    set(MACRO_UTILS_C_INCLUDES ${MACRO_UTILS_C_INCLUDES} CACHE INTERNAL "")
else()
    message(STATUS "This version of CMake does not support interface targets. Update CMake")
endif()