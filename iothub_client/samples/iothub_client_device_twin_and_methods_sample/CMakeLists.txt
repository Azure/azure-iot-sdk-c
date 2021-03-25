#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#This is CMakeLists.txt for iothub_client_device_twin_and_methods_sample

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without amqp or mqtt support")
endif()

compileAsC99()

# Compiler Defintions
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()
if(${use_amqp})
    add_definitions(-DUSE_AMQP)
endif()
if(${use_mqtt})
    add_definitions(-DUSE_MQTT)
endif()

# Sample executable
set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)
if(${use_sample_trusted_cert})
    set(iothub_client_device_twin_and_methods_sample_c_files ${iothub_client_device_twin_and_methods_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()
set(iothub_client_device_twin_and_methods_sample_h_files
)

add_executable(iothub_client_device_twin_and_methods_sample ${iothub_client_device_twin_and_methods_sample_c_files} ${iothub_client_device_twin_and_methods_sample_h_files})
include_directories(.)

# C-utility and iothub_client -- Compiled and Included at a higher level
target_link_libraries(iothub_client_device_twin_and_methods_sample
    aziotsharedutil
    iothub_client
)

# MQTT Library
if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_mqtt_transport
    )
endif()

# AMQP Library
if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_amqp_transport
        uamqp
    )
endif()

# Parson Library
if(TARGET NOT ${parson})
    add_library(parson
        ./deps/parson/parson.c
        ./deps/parson/parson.h
    )
endif()

if (MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()

target_link_libraries(iothub_client_device_twin_and_methods_sample parson)
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)