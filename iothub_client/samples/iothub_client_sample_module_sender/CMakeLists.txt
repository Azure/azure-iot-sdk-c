#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_module_sender

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothub_client_sample_module_sender being generated without mqtt support")
endif()

compileAsC99()

set(iothub_client_sample_module_sender_c_files
    iothub_client_sample_module_sender.c
)

set(iothub_client_sample_module_sender_h_files
    iothub_client_sample_module_sender.h
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

add_executable(iothub_client_sample_module_sender ${iothub_client_sample_module_sender_c_files} ${iothub_client_sample_module_sender_h_files} ${samples_cert_file})

target_link_libraries(iothub_client_sample_module_sender
    iothub_client_mqtt_transport
    iothub_client
)

