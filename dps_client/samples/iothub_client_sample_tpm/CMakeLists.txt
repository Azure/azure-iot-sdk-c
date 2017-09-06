#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_tpm

compileAsC99()

set(iothub_client_sample_tpm_c_files
    iothub_client_sample_tpm.c
    ../../../certs/certs.c
)

set(iothub_client_sample_tpm_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})

add_executable(iothub_client_sample_tpm ${iothub_client_sample_tpm_c_files} ${iothub_client_sample_tpm_h_files})

target_link_libraries(iothub_client_sample_tpm
    iothub_client 
    dps_client
    iothub_client_mqtt_transport
    iothub_client_amqp_transport)

linkDRS_device_auth(iothub_client_sample_tpm)
linkSharedUtil(iothub_client_sample_tpm)
linkUAMQP(iothub_client_sample_tpm)

