#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_registrymanager_sample_c_files
    iothub_registrymanager_sample.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_registrymanager_sample ${iothub_registrymanager_sample_c_files})

target_link_libraries(iothub_registrymanager_sample
    iothub_service_client
)

linkSharedUtil(iothub_registrymanager_sample)
