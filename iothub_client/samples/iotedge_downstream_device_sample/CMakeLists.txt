#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iotedge_downstream_device_sample being generated without protocol support")
endif()

compileAsC99()

# Sample Executable
set(iotedge_downstream_c_files
    iotedge_downstream_device_sample.c
)

add_executable(iotedge_downstream_device_sample
    ${iotedge_downstream_c_files}
)

target_link_libraries(iotedge_downstream_device_sample
    iothub_client
)
