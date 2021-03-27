#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothub_client_sample_module_sender being generated without MQTT support")
endif()

compileAsC99()

# Sample Executable
set(iothub_client_sample_module_sender_c_files
    iothub_client_sample_module_sender.c
)

add_executable(iothub_client_sample_module_sender
    ${iothub_client_sample_module_sender_c_files}
)

target_link_libraries(iothub_client_sample_module_sender
    iothub_client
)
