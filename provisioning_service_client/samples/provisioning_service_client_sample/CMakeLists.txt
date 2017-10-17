#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for provisioning_service_client_sample

compileAsC99()

set(provisioning_service_client_sample_c_files
    provisioning_service_client_sample.c
)

set (provisioning_service_client_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(${PROVISIONING_SERVICE_CLIENT_INC_FOLDER})

add_executable(provisioning_service_client_sample ${provisioning_service_client_sample_c_files} ${provisioning_service_client_sample_h_files})

target_link_libraries(provisioning_service_client_sample
    provisioning_service_client)

linkSharedUtil(provisioning_service_client_sample)
linkUHTTP(provisioning_service_client_sample)