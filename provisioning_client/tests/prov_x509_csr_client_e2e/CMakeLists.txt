#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName prov_x509_csr_client_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ../common_prov_e2e/common_prov_e2e.c
)

set(${theseTestsName}_h_files
    ../common_prov_e2e/common_prov_e2e.h
)

if(${use_sample_trusted_cert})
    set(${theseTestsName}_c_files
        ../../../certs/certs.c
    )
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${PROVISIONING_SERVICE_CLIENT_INC_FOLDER})
include_directories(../common_prov_e2e)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../adapters)

file(COPY ../common_prov_e2e/prov_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/provisioning_e2e_tests" VALGRIND_SUPPRESSIONS_FILE prov_valgrind_suppression.supp)

# CSR (Certificate Signing Request) over DPS / IoT Hub is currently only
# supported over MQTT. Guard the test on use_mqtt so it is not built / run
# when only HTTP or AMQP transports are enabled.
if (NOT ${use_mqtt})
    message(STATUS "Skipping ${theseTestsName}: use_mqtt is OFF (CSR is MQTT-only)")
    return()
endif()

add_definitions(-DUSE_MQTT)
set(prov_transport prov_mqtt_transport prov_mqtt_ws_transport)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            iothub_test
            prov_device_client
            prov_device_ll_client
            ${prov_transport}
            iothub_client
            iothub_client_mqtt_transport
            rpcrt4
        )
    endif()

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            iothub_test
            prov_device_client
            prov_device_ll_client
            ${prov_transport}
            iothub_client
            iothub_client_mqtt_transport
            rpcrt4
        )
    endif()
    target_link_libraries(${theseTestsName}_dll provisioning_service_client)
else()

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            prov_device_client
            prov_device_ll_client
            ${prov_transport}
            iothub_client
            iothub_client_mqtt_transport
            iothub_test
        )
    endif()
endif()

target_link_libraries(${theseTestsName}_exe
    provisioning_service_client
    prov_auth_client
    aziotsharedutil
    hsm_security_client
    parson
)

# Override the default single-test registration from build_c_test_artifacts
# with per-function tests so each TEST_FUNCTION appears individually in CTest/Azure DevOps.
if(TARGET ${theseTestsName}_exe)
    # Remove the suite-level test added by build_c_test_artifacts
    set_tests_properties(${theseTestsName} PROPERTIES DISABLED TRUE)

    add_test(NAME dps_x509_csr_full_lifecycle_mqtt
        COMMAND ${theseTestsName}_exe dps_x509_csr_full_lifecycle_mqtt)
    add_test(NAME dps_symkey_csr_full_lifecycle_mqtt
        COMMAND ${theseTestsName}_exe dps_symkey_csr_full_lifecycle_mqtt)
endif()
