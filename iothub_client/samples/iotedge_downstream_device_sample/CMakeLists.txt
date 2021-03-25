#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iotedge_downstream_device_sample being generated without protocol support")
endif()

compileAsC99()

# Sample executable
set(iotedge_downstream_c_files
    iotedge_downstream_device_sample.c
)
set(iotedge_downstream_h_files
)

add_executable(iotedge_downstream_device_sample
    ${iotedge_downstream_c_files}
    ${iotedge_downstream_h_files}
)

include_directories(.)

target_link_libraries(iotedge_downstream_device_sample
    aziotsharedutil
    iothub_client
)

# MQTT Library
if(${use_mqtt})
    target_link_libraries(iotedge_downstream_device_sample
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
    )
endif()

# AMQP Library
if(${use_amqp})
    target_link_libraries(iotedge_downstream_device_sample
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
    )
endif()

# HTTP Library
if(${use_http})
    target_link_libraries(iotedge_downstream_device_sample
        iothub_client_http_transport
    )
endif()
