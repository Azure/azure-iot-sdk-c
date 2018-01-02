#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_tpm

compileAsC99()

set(iothub_client_sample_hsm_c_files
    iothub_client_sample_hsm.c
    ../../../certs/certs.c
)

set(iothub_client_sample_hsm_h_files
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

add_executable(iothub_client_sample_hsm ${iothub_client_sample_hsm_c_files} ${iothub_client_sample_hsm_h_files})

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    if (WIN32)
        target_link_libraries(iothub_client_sample_hsm $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

target_link_libraries(iothub_client_sample_hsm
    iothub_client 
    iothub_client_mqtt_transport
    iothub_client_amqp_transport)

link_security_client(iothub_client_sample_hsm)
linkSharedUtil(iothub_client_sample_hsm)
linkUAMQP(iothub_client_sample_hsm)

