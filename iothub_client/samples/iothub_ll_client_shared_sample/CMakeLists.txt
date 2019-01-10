#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_ll_client_shared_sample

compileAsC99()

if(NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iothub_ll_client_shared_sample being generated without uamqp or http support")
endif()

set(iothub_ll_client_shared_sample_c_files
    iothub_ll_client_shared_sample.c
)

set(iothub_ll_client_shared_sample_h_files
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
    set(iothub_ll_client_shared_sample_c_files ${iothub_ll_client_shared_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(iothub_ll_client_shared_sample ${iothub_ll_client_shared_sample_c_files} ${iothub_ll_client_shared_sample_h_files})

if(${use_http})
    target_link_libraries(iothub_ll_client_shared_sample iothub_client_http_transport)
endif()

if(${use_amqp})
    target_link_libraries(iothub_ll_client_shared_sample
    iothub_client_amqp_transport
    iothub_client_amqp_ws_transport)
    linkUAMQP(iothub_ll_client_shared_sample)
endif()

target_link_libraries(iothub_ll_client_shared_sample
    iothub_client)

linkSharedUtil(iothub_ll_client_shared_sample)

