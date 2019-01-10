#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_ll_client_x509_sample

compileAsC99()

set(iothub_ll_client_x509_sample_c_files
    iothub_ll_client_x509_sample.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_ll_client_x509_sample_c_files ${iothub_ll_client_x509_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(iothub_ll_client_x509_sample ${iothub_ll_client_x509_sample_c_files})

if(${use_http})
    target_link_libraries(iothub_ll_client_x509_sample
        iothub_client_http_transport
    )
    linkHttp(iothub_ll_client_x509_sample)
    add_definitions(-DUSE_HTTP)
endif()

if(${use_amqp})
    target_link_libraries(iothub_ll_client_x509_sample
        iothub_client_amqp_transport
    )
    linkUAMQP(iothub_ll_client_x509_sample)
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_ll_client_x509_sample iothub_client_mqtt_transport)
    linkMqttLibrary(iothub_ll_client_x509_sample)
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothub_ll_client_x509_sample iothub_client)
linkSharedUtil(iothub_ll_client_x509_sample)


