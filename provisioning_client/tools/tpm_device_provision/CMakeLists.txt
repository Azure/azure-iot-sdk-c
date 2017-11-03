#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#This is the CMakeLists.txt for testing
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()

set(dev_auth_prov_c_files
    ./tpm_device_provision.c
    ../../../certs/certs.c
)

set(dev_auth_prov_h_files
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

add_executable(tpm_device_provision ${dev_auth_prov_c_files} ${dev_auth_prov_h_files})

linkSharedUtil(tpm_device_provision)
link_security_client(tpm_device_provision)
