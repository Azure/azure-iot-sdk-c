# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

########## SDK Testing Dependencies ##########
# Test dependencies (macro-utils-c, umock-c, ctest, testrunnerswitcher, c-logging)
# are added as first-level submodules and loaded by the top-level CMakeLists.txt
# before this file is included.
if (${use_installed_dependencies})
    if (NOT azure_macro_utils_cFOUND AND NOT macro_utils_cFOUND)
        find_package(macro_utils_c CONFIG QUIET)
        if (NOT macro_utils_c_FOUND)
            find_package(azure_macro_utils_c REQUIRED CONFIG)
        endif()
    endif ()
    if (NOT umock_cFOUND)
        find_package(umock_c REQUIRED CONFIG)
    endif ()
endif()

########## Parson ##########
if(NOT ${use_installed_dependencies})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/parson)
else()
    find_package(parson REQUIRED CONFIG)
endif()
