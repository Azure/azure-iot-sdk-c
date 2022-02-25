#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(prov_dev_client_sample_c_files
    prov_dev_client_sample.c
)

set(prov_dev_client_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(prov_dev_client_sample_c_files ${prov_dev_client_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

add_executable(prov_dev_client_sample ${prov_dev_client_sample_c_files} ${prov_dev_client_sample_h_files})

target_link_libraries(prov_dev_client_sample
    iothub_client
    prov_auth_client
    hsm_security_client
    prov_device_client
)

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    if (WIN32)
        target_link_libraries(prov_dev_client_sample $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

if (${use_http})
    target_link_libraries(prov_dev_client_sample prov_http_transport aziotsharedutil)
endif()
if (${use_mqtt})
    target_link_libraries(prov_dev_client_sample iothub_client_mqtt_transport prov_mqtt_transport prov_mqtt_ws_transport aziotsharedutil)
endif()
if (${use_amqp})
    target_link_libraries(prov_dev_client_sample iothub_client_amqp_transport prov_amqp_ws_transport prov_amqp_transport aziotsharedutil)
    linkUAMQP(prov_dev_client_sample)
endif()
