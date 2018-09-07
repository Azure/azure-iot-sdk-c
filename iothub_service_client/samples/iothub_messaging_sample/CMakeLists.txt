#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_messaging_sample_c_files
    iothub_messaging_sample.c
)

IF(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_messaging_sample ${iothub_messaging_sample_c_files})

target_link_libraries(iothub_messaging_sample
    iothub_service_client
)

linkSharedUtil(iothub_messaging_sample)
linkUAMQP(iothub_messaging_sample)
