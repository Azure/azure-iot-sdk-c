#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for dps_sc_sample

compileAsC99()

set(dps_sc_sample_c_files
    dps_sc_sample.c
)

set (dps_sc_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(${DPS_SERVICE_CLIENT_INC_FOLDER})

add_executable(dps_sc_sample ${dps_sc_sample_c_files} ${dps_sc_sample_h_files})

target_link_libraries(dps_sc_sample
    dps_service_client)

linkSharedUtil(dps_sc_sample)
linkUHTTP(dps_sc_sample)