#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for prov_sc_query_sample

compileAsC99()

set(prov_sc_query_sample_c_files
    prov_sc_query_sample.c
)

set (prov_sc_query_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(${PROVISIONING_SERVICE_CLIENT_INC_FOLDER})

add_executable(prov_sc_query_sample ${prov_sc_query_sample_c_files} ${prov_sc_query_sample_h_files})

target_link_libraries(prov_sc_query_sample
    provisioning_service_client)

linkSharedUtil(prov_sc_query_sample)
linkUHTTP(prov_sc_query_sample)