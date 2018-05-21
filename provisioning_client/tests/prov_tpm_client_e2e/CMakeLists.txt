#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName prov_tpm_client_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ../common_prov_e2e/common_prov_e2e.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_h_files
    ../common_prov_e2e/common_prov_e2e.h
)

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${PROVISIONING_SERVICE_CLIENT_INC_FOLDER})
include_directories(../common_prov_e2e)

file(COPY ../common_prov_e2e/prov_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_e2e_tests" VALGRIND_SUPPRESSIONS_FILE suppressions.supp)

set(prov_transport)
if (${use_http})
    add_definitions(-DUSE_HTTP)
    set(prov_transport ${prov_transport} prov_http_transport)
endif()
if (${use_mqtt})
    add_definitions(-DUSE_MQTT)
    set(prov_transport ${prov_transport} prov_mqtt_transport prov_mqtt_ws_transport)
endif()
if (${use_amqp})
    add_definitions(-DUSE_AMQP)
    set(prov_transport ${prov_transport} prov_amqp_transport prov_amqp_ws_transport)
endif()

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            iothub_test
            prov_device_ll_client
            ${prov_transport}
            rpcrt4
        )
    endif()

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            iothub_test
            prov_device_ll_client
            ${prov_transport}
            rpcrt4
        )
    endif()
    target_link_libraries(${theseTestsName}_dll provisioning_service_client)
else()

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            prov_device_ll_client
            ${prov_transport}
            iothub_test
        )
    endif()
endif()

target_link_libraries(${theseTestsName}_exe provisioning_service_client)

link_security_client(${theseTestsName}_exe)
linkSharedUtil(${theseTestsName}_exe)
