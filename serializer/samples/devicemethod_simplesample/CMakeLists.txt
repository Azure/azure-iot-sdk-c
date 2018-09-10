#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for devicemethod_simplesample sample

if(NOT ${use_mqtt})
    message(FATAL_ERROR "devicemethod_simplesample being generated without mqtt support")
endif()

compileAsC99()

set(devicemethod_simplesample_c_files
    devicemethod_simplesample.c
)

if(WIN32)
    set(devicemethod_simplesample_c_files ${devicemethod_simplesample_c_files} ./windows/main.c)
else()
    set(devicemethod_simplesample_c_files ${devicemethod_simplesample_c_files} ./linux/main.c)
endif()

set(devicemethod_simplesample_h_files
    devicemethod_simplesample.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(devicemethod_simplesample_c_files ${devicemethod_simplesample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(. ${SHARED_UTIL_INC_FOLDER} ${IOTHUB_CLIENT_INC_FOLDER})
#link_directories(${whatIsBuilding}_dll ${SHARED_UTIL_LIB_DIR})

add_executable(devicemethod_simplesample ${devicemethod_simplesample_c_files} ${devicemethod_simplesample_h_files})

target_link_libraries(devicemethod_simplesample
    iothub_client_mqtt_transport
    serializer
    iothub_client
    )
