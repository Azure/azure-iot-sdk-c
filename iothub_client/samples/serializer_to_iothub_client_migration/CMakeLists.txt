#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for serializer_to_iothub_client_migration

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "serializer_to_iothub_client_migration being generated without amqp or mqtt support")
endif()

compileAsC99()

set(serializer_to_iothub_client_migration_c_files
    serializer_to_iothub_client_migration.c
)

set(serializer_to_iothub_client_migration_h_files
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
    set(serializer_to_iothub_client_migration_c_files ${serializer_to_iothub_client_migration_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(serializer_to_iothub_client_migration ${serializer_to_iothub_client_migration_c_files} ${serializer_to_iothub_client_migration_h_files})

if(${use_amqp}) 
    target_link_libraries(serializer_to_iothub_client_migration  
        iothub_client_amqp_transport
        uamqp
    )
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt}) 
    target_link_libraries(serializer_to_iothub_client_migration  
        iothub_client_mqtt_transport
    )
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(serializer_to_iothub_client_migration  
    aziotsharedutil
    iothub_client 
)
