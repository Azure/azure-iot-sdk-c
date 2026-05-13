# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

########## SDK Testing Dependencies ##########
# Test dependencies (macro-utils-c, umock-c, ctest, testrunnerswitcher, c-logging)
# are now provided by c-utility, which is loaded before this file.
if (${use_installed_dependencies})
    if (NOT azure_macro_utils_cFOUND)
        find_package(azure_macro_utils_c REQUIRED CONFIG)
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
