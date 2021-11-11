#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#This is the CMakeLists.txt for testing
cmake_minimum_required (VERSION 3.5)

compileAsC99()

set(symm_key_provision_c_files
    ./symm_key_provision.c
)

set(symm_key_provision_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

add_executable(symm_key_provision ${symm_key_provision_c_files} ${symm_key_provision_h_files})

target_link_libraries(symm_key_provision
    PUBLIC
        prov_auth_client
        hsm_security_client
        aziotsharedutil
)
