#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName prov_x509_client_openssl_engine_e2e)

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

file(COPY ../common_prov_e2e/prov_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_e2e_tests" VALGRIND_SUPPRESSIONS_FILE prov_valgrind_suppression.supp)

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

if(LINUX AND ${use_openssl} AND ${hsm_type_x509})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_OPENSSL_ENGINE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTEST_OPENSSL_ENGINE")

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            prov_device_ll_client
            ${prov_transport}
            iothub_test
        )
    endif()

    target_link_libraries(${theseTestsName}_exe
        provisioning_service_client
        prov_auth_client
        aziotsharedutil
        hsm_security_client
    )
else()
    message(FATAL_ERROR "prov_x509_client_openssl_engine_e2e can only run on Linux with OpenSSL as the TLS stack.")
endif()
