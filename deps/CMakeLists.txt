# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

########## SDK Testing Dependencies ##########
# Test dependencies (macro-utils-c, umock-c, ctest, testrunnerswitcher, c-logging)
# are added as first-level submodules and loaded by the top-level CMakeLists.txt
# before this file is included.
if (${use_installed_dependencies})
    # New macro_utils_c and umock_c install their CMake configs into
    # <prefix>/cmake/ which is non-standard. Tell find_package to look there.
    set(_sdk_cmake_config_path "${CMAKE_INSTALL_PREFIX}/cmake")
    if (NOT azure_macro_utils_cFOUND AND NOT macro_utils_cFOUND)
        find_package(macro_utils_c CONFIG QUIET PATHS "${_sdk_cmake_config_path}")
        if (NOT macro_utils_c_FOUND)
            find_package(azure_macro_utils_c CONFIG QUIET PATHS "${_sdk_cmake_config_path}")
            if (NOT azure_macro_utils_c_FOUND)
                find_package(azure_macro_utils_c REQUIRED CONFIG)
            endif()
        endif()
    endif ()
    if (NOT umock_cFOUND)
        find_package(umock_c CONFIG QUIET PATHS "${_sdk_cmake_config_path}")
        if (NOT umock_c_FOUND)
            # umock_c may not install a Config.cmake; just verify headers exist.
            find_path(_umock_c_inc "umock_c/umock_c_prod.h" PATHS "${CMAKE_INSTALL_PREFIX}/include")
            if (_umock_c_inc)
                set(UMOCK_C_INC_FOLDER "${_umock_c_inc}" CACHE INTERNAL "" FORCE)
            else()
                find_package(umock_c REQUIRED CONFIG)
            endif()
        endif()
    endif ()
endif()

########## Parson ##########
if(NOT ${use_installed_dependencies})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/parson)
else()
    find_package(parson REQUIRED CONFIG)
endif()
