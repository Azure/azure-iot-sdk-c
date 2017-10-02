#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists for dps_service_client

if(NOT ${use_http})
    message("dps service client needs HTTP support - use_http option was ${use_http}")
    return()
endif()

compileAsC99()

set(dps_service_client_c_files
./src/dps_sc_client.c
)

set(dps_service_client_h_files
./inc/dps_sc_client.h
./inc/dps_sc_enrollment.h
)

include_directories(${SHARED_UTIL_INC_FOLDER})

include_directories(${UHTTP_C_INC_FOLDER})

set(DPS_SERVICE_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "This is the include folder for dps_service_client" FORCE)

include_directories(${DPS_SERVICE_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

if (${build_as_dynamic})
    add_library(dps_service_client SHARED ${dps_service_client_c_files} ${dps_service_client_h_files} ./src/dps_service_client.def)
    linkSharedUtil(dps_service_client)

else()
    add_library(dps_service_client ${dps_service_client_c_files} ${dps_service_client_h_files})
endif()

target_link_libraries(dps_service_client)

if (NOT ${ARCHITECTURE} STREQUAL "ARM")
    if (NOT ${skip_samples})
    add_subdirectory(samples)
    endif()
endif()

if(NOT IN_OPENWRT)
    # Disable tests for OpenWRT
    if(${run_unittests})
        add_subdirectory(tests)
    endif()
endif()