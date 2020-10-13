#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubclient_fuzz_mqtt

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iothubclient_fuzz_mqtt being generated without protocol support")
endif()

compileAsC99()

set(iothub_c_files
    iothubclient_fuzz.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(.)

add_executable(iothubclient_fuzz_mqtt ${iothub_c_files})

if(${use_http})
    target_link_libraries(iothubclient_fuzz_mqtt iothub_client_http_transport)
    add_definitions(-DUSE_HTTP)
endif()

if(${use_amqp})
    target_link_libraries(iothubclient_fuzz_mqtt iothub_client_amqp_transport iothub_client_amqp_ws_transport)
    linkUAMQP(iothubclient_fuzz_mqtt)
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothubclient_fuzz_mqtt iothub_client_mqtt_transport iothub_client_mqtt_ws_transport)
    linkMqttLibrary(iothubclient_fuzz_mqtt)
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothubclient_fuzz_mqtt iothub_client)



