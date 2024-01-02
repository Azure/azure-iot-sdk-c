#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_client_sample_hsm_c_files
    iothub_client_sample_hsm.c
)

set(iothub_client_sample_hsm_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_client_sample_hsm_c_files ${iothub_client_sample_hsm_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

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

if (${use_amqp})
    linkUAMQP(iothub_client_sample_hsm)
endif()

# prov_auth_client must come after the transport selection
target_link_libraries(iothub_client_sample_hsm
    $<$<BOOL:${use_mqtt}>:iothub_client_mqtt_transport>
    $<$<BOOL:${use_amqp}>:iothub_client_amqp_transport>
    iothub_client
    prov_auth_client
    aziotsharedutil
    hsm_security_client
)
