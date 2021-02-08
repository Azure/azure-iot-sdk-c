#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_device_twin_cbor_sample

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_device_twin_cbor_sample being generated without amqp or mqtt support")
endif()

compileAsC99()

add_executable(iothub_client_device_twin_cbor_sample 
    iothub_client_device_twin_cbor_sample.c
    cbor_helper.c
)

find_library(CJSON_LIB
    NAMES
        libcjson.a
)

target_link_libraries(iothub_client_device_twin_cbor_sample
    aziotsharedutil
    iothub_client
    tinycbor
    ${CJSON_LIB}
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    target_include_directories(iothub_client_device_twin_cbor_sample
        PRIVATE
            ${PROJECT_SOURCE_DIR}/certs)
    target_sources(iothub_client_device_twin_cbor_sample
        PRIVATE
            ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
endif()

if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_cbor_sample
        iothub_client_amqp_transport
        uamqp
    )
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_cbor_sample
        iothub_client_mqtt_transport
    )
    add_definitions(-DUSE_MQTT)
endif()
