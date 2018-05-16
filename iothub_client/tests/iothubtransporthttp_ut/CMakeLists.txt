#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubtransporthttp_ut
cmake_minimum_required(VERSION 2.8.11)

if(NOT ${use_http})
    message(FATAL_ERROR "iothubtransporthttp_ut being generated without HTTP support")
endif()

compileAsC11()

set(theseTestsName iothubtransporthttp_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

set(${theseTestsName}_c_files
    ../../src/iothubtransporthttp.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_buffer.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
    real_doublylinkedlist.c
)

set(${theseTestsName}_h_files
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
