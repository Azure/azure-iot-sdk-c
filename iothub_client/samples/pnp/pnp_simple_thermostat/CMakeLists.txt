#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for pnp_simple_thermostat

compileAsC99()

set(pnp_simple_thermostat_c_files
    pnp_simple_thermostat.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(pnp_simple_thermostat_c_files ${pnp_simple_thermostat_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(. ${PROJECT_SOURCE_DIR}/deps/parson)

add_executable(pnp_simple_thermostat ${pnp_simple_thermostat_c_files})

target_link_libraries(pnp_simple_thermostat iothub_client_mqtt_transport iothub_client_mqtt_ws_transport)
linkMqttLibrary(pnp_simple_thermostat)
add_definitions(-DUSE_MQTT)

target_link_libraries(pnp_simple_thermostat iothub_client)
