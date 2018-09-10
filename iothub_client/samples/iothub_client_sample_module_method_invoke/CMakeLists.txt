#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_client_sample_module_method_invoke_c_files
    iothub_client_sample_module_method_invoke.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_executable(iothub_client_sample_module_method_invoke ${iothub_client_sample_module_method_invoke_c_files})

target_link_libraries(iothub_client_sample_module_method_invoke
    iothub_client_mqtt_transport
    iothub_client_amqp_transport
    uamqp
    iothub_client
)