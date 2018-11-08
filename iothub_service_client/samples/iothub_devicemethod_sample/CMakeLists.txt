#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_devicemethod_sample_c_files
    iothub_devicemethod_sample.c
)

set(iothub_devicemethod_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_devicemethod_sample ${iothub_devicemethod_sample_c_files} ${iothub_devicemethod_sample_h_files})

target_link_libraries(iothub_devicemethod_sample
    iothub_service_client
)

linkSharedUtil(iothub_devicemethod_sample)
