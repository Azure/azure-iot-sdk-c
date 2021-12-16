# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

########## SDK Testing Dependencies ##########
if (NOT ${use_installed_dependencies})

    if (NOT TARGET azure_macro_utils_c AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/azure-macro-utils-c/CMakeLists.txt")
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/azure-macro-utils-c)
    endif()

    if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
        if (NOT TARGET testrunnerswitcher)
            add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/azure-c-testrunnerswitcher)
        endif()
        if (NOT TARGET ctest)
            # Get the repo if it's not there
            add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/azure-ctest)
        endif()
        enable_testing()
    endif()


    if (NOT TARGET umock_c)
        # Get the repo if it's not there
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
