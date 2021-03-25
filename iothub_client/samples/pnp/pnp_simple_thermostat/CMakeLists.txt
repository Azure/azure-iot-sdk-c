#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "pnp_simple_thermostat being generated without mqtt support")
endif()

compileAsC99()

# Sample executable
set(pnp_simple_thermostat_c_files
    pnp_simple_thermostat.c
)

if(${use_prov_client})
    set(pnp_simple_thermostat_c_files
        ${pnp_simple_thermostat_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/../common/pnp_dps_ll.c
    )
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../common)
endif()

add_executable(pnp_simple_thermostat
    ${pnp_simple_thermostat_c_files}
)

target_link_libraries(pnp_simple_thermostat
    aziotsharedutil
    iothub_client
)

# Certs Library
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    target_link_libraries(pnp_simple_thermostat certs)
endif()

# MQTT Library
target_link_libraries(pnp_simple_thermostat
    iothub_client_mqtt_transport
    iothub_client_mqtt_ws_transport
)

# Provisioning Client Library
if(${use_prov_client})
    target_link_libraries(pnp_simple_thermostat
        prov_device_ll_client
        prov_mqtt_transport
    )
endif()

# Parson Library
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
target_link_libraries(pnp_simple_thermostat parson)