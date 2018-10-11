#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iotedge_downstream_device_sample being generated without protocol support")
endif()

compileAsC99()

set(iotedge_downstream_c_files
    iotedge_downstream_device_sample.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(.)

add_executable(iotedge_downstream_device_sample ${iotedge_downstream_c_files})
if(${build_as_dynamic})
    target_link_libraries(iotedge_downstream_device_sample iothub_client_dll)
else()
    target_link_libraries(iotedge_downstream_device_sample iothub_client)

    if(${use_http})
        target_link_libraries(iotedge_downstream_device_sample iothub_client_http_transport)
        add_definitions(-DUSE_HTTP)
    endif()

    if(${use_amqp})
        target_link_libraries(iotedge_downstream_device_sample iothub_client_amqp_transport iothub_client_amqp_ws_transport)
        linkUAMQP(iotedge_downstream_device_sample)
        add_definitions(-DUSE_AMQP)
    endif()

    if(${use_mqtt})
        target_link_libraries(iotedge_downstream_device_sample iothub_client_mqtt_transport iothub_client_mqtt_ws_transport)
        linkMqttLibrary(iotedge_downstream_device_sample)
        add_definitions(-DUSE_MQTT)
    endif()
endif()