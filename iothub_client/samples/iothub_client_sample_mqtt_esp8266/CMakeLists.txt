#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_mqtt_esp8266

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothub_client_sample_mqtt_esp8266 being generated without mqtt support")
endif()

compileAsC99()

set(iothub_client_sample_mqtt_esp8266_c_files
    iothub_client_sample_mqtt_esp8266.c
)

set(iothub_client_sample_mqtt_esp8266_h_files
    iothub_client_sample_mqtt_esp8266.h
)

include_directories(.)

add_executable(iothub_client_sample_mqtt_esp8266 ${iothub_client_sample_mqtt_esp8266_c_files} ${iothub_client_sample_mqtt_esp8266_h_files})

target_link_libraries(iothub_client_sample_mqtt_esp8266
    iothub_client_mqtt_transport
    iothub_client
)

