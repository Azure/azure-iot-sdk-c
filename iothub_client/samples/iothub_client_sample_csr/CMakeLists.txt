#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_csr

compileAsC99()

set(iothub_c_files
    iothub_client_sample_csr.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_c_files ${iothub_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)

add_executable(iothub_client_sample_csr ${iothub_c_files})

target_link_libraries(iothub_client_sample_csr
    iothub_client
    iothub_client_mqtt_transport
    aziotsharedutil
    parson
    ${OPENSSL_LIBRARIES}
)

linkMqttLibrary(iothub_client_sample_csr)
