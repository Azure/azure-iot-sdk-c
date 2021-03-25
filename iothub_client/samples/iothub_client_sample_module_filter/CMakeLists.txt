#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothub_client_sample_module_filter being generated without MQTT support")
endif()

compileAsC99()

# Sample Executable
set(iothub_client_sample_module_filter_c_files
    iothub_client_sample_module_filter.c
)

add_executable(iothub_client_sample_module_filter
    ${iothub_client_sample_module_filter_c_files}
)

target_link_libraries(iothub_client_sample_module_filter
    aziotsharedutil
    iothub_client
)

# MQTT Library
target_link_libraries(iothub_client_sample_module_filter
    iothub_client_mqtt_transport
)
