#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubtransportmqtt_ut
cmake_minimum_required (VERSION 3.5)

if(NOT ${use_mqtt}) 
    message(FATAL_ERROR "iothubtransport_mqtt_common_ut being generated without mqtt support")
endif()

compileAsC99()
set(theseTestsName iothubtransport_mqtt_common_ut)

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../src/iothubtransport_mqtt_common.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_doublylinkedlist.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
