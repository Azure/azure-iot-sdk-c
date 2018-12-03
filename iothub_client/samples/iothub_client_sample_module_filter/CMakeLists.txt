#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_module_filter

compileAsC99()

set(iothub_client_sample_module_filter
iothub_client_sample_module_filter.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${set_trusted_cert_in_samples})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
	include_directories(${PROJECT_SOURCE_DIR}/certs)
	set(samples_cert_file ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(iothub_client_sample_module_filter ${iothub_client_sample_module_filter} ${samples_cert_file})

if(${use_http})
    target_link_libraries(iothub_client_sample_module_filter
        iothub_client_http_transport
    )
    linkSharedUtil(iothub_client_sample_module_filter)
    linkHttp(iothub_client_sample_module_filter)
    add_definitions(-DUSE_HTTP)
endif()

if(${use_amqp})
    target_link_libraries(iothub_client_sample_module_filter
    #iothubclient is here only because locking... in gballoc no less.
        iothub_client_amqp_transport
    )
    linkUAMQP(iothub_client_sample_module_filter)
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_client_sample_module_filter
        iothub_client_mqtt_transport
    )
    linkMqttLibrary(iothub_client_sample_module_filter)
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothub_client_sample_module_filter
#iothubclient is here only because locking... in gballoc no less.
    iothub_client
)


