#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_device_twin

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_sample_device_twin being generated without mqtt support")
endif()

compileAsC99()

set(iothub_client_sample_device_twin_c_files
    iothub_client_sample_device_twin.c
)

set(iothub_client_sample_device_twin_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_client_sample_device_twin ${iothub_client_sample_device_twin_c_files} ${iothub_client_sample_device_twin_h_files})

if(${use_amqp}) 
    target_link_libraries(iothub_client_sample_device_twin  
        iothub_client_amqp_transport
        uamqp
    )
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt}) 
    target_link_libraries(iothub_client_sample_device_twin  
        iothub_client_mqtt_transport
    )
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothub_client_sample_device_twin  
    aziotsharedutil
    iothub_client 
)
