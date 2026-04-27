# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

########## SDK Testing Dependencies ##########
if (NOT ${use_installed_dependencies})

    if ((NOT TARGET c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/c-build-tools/CMakeLists.txt))
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c-build-tools)
        set_default_build_options()
    endif()

    if ((NOT TARGET macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/macro-utils-c/CMakeLists.txt))
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/macro-utils-c)
    endif()

    if ((NOT TARGET c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/c-logging/CMakeLists.txt))
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c-logging)
        if (NOT TARGET c_logging)
            add_library(c_logging ALIAS c_logging_v2)
        endif()
    endif()

    if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
        if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/c-testrunnerswitcher/CMakeLists.txt))
            add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c-testrunnerswitcher)
        endif()
        if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/ctest/CMakeLists.txt))
            add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ctest)
        endif()
        enable_testing()
    endif()

    if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/umock-c/CMakeLists.txt))
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/umock-c)
    endif()
else()
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
