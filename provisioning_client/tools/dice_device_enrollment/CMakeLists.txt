#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#This is the CMakeLists.txt for testing
cmake_minimum_required(VERSION 2.8.11)

compileAsC99()

set(dice_device_enrollment_c_files
    ./dice_device_enrollment.c
)

set(dice_device_enrollment_h_files
)


IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})

add_executable(dice_device_enrollment ${dice_device_enrollment_c_files} ${dice_device_enrollment_h_files})

if(${use_openssl})
    if (WIN32)
        target_link_libraries(dice_device_enrollment $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

linkSharedUtil(dice_device_enrollment)
link_security_client(dice_device_enrollment)
target_link_libraries(dice_device_enrollment parson)