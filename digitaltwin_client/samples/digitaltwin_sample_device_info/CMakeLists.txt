#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for digitaltwin_sample_device_info

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

if(LINUX)
	set(digitaltwin_sample_device_info_c_files
		./ops/deviceinfo_ops_common.c
		./ops/deviceinfo_ops_linux.c
		./digitaltwin_sample_device_info.c
	)
else()
	set(digitaltwin_sample_device_info_c_files
		./ops/deviceinfo_ops_common.c
		./ops/deviceinfo_ops_sample.c
		./digitaltwin_sample_device_info.c
	)
endif()

set(digitaltwin_sample_device_info_h_files
    ./digitaltwin_sample_device_info.h
)

set(DIGITALTWIN_SAMPLE_DEVICE_INFO_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "this is what needs to be included if using digitaltwin_client lib" FORCE)

include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${DIGITALTWIN_CLIENT_INC_FOLDER})

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif()

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)


add_library(digitaltwin_sample_device_info
    ${digitaltwin_sample_device_info_c_files}
    ${digitaltwin_sample_device_info_h_files}
)
setSdkTargetBuildProperties(digitaltwin_sample_device_info)

target_link_libraries(digitaltwin_sample_device_info digitaltwin_client)