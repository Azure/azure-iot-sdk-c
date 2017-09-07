#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(uhttp_sample_c_files
    uhttp_sample.c
)

set(uhttp_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})

add_executable(uhttp_sample ${uhttp_sample_c_files} ${uhttp_sample_h_files})

target_link_libraries(uhttp_sample dps_client)
linkSharedUtil(uhttp_sample)

