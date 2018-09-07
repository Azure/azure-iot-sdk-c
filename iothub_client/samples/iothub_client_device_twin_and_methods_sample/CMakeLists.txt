#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_device_twin_and_methods_sample

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without amqp or mqtt support")
endif()

compileAsC99()

set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)

set(iothub_client_device_twin_and_methods_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_client_device_twin_and_methods_sample_c_files ${iothub_client_device_twin_and_methods_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)

add_executable(iothub_client_device_twin_and_methods_sample ${iothub_client_device_twin_and_methods_sample_c_files} ${iothub_client_device_twin_and_methods_sample_h_files})

if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_amqp_transport
        uamqp
    )
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_mqtt_transport
    )
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothub_client_device_twin_and_methods_sample
    aziotsharedutil
    iothub_client
)
