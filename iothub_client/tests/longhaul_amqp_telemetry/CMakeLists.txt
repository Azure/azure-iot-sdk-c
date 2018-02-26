#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for longhaul_amqp_telemetry

compileAsC99()

set(PROJECT_NAME "longhaul_amqp_telemetry")

if(NOT ${use_amqp})
    message(FATAL_ERROR "longhaul_amqp_telemetry being generated without uamqp support")
endif()

set(project_c_files
    ${PROJECT_NAME}.c
)

set(project_h_files
    ${PROJECT_NAME}.h
)

build_c_test_longhaul_test(${PROJECT_NAME} ${project_c_files} ${project_h_files})

target_link_libraries(${PROJECT_NAME} iothub_client_amqp_transport)

linkSharedUtil(${PROJECT_NAME})
linkUAMQP(${PROJECT_NAME})
