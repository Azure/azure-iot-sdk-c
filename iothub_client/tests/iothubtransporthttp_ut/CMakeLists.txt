#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubtransporthttp_ut
cmake_minimum_required (VERSION 3.5)

if(NOT ${use_http})
    message(FATAL_ERROR "iothubtransporthttp_ut being generated without HTTP support")
endif()

compileAsC99()

set(theseTestsName iothubtransporthttp_ut)

generate_cppunittest_wrapper(${theseTestsName})

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

set(${theseTestsName}_c_files
    ../../src/iothubtransporthttp.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_buffer.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_doublylinkedlist.c
)

set(${theseTestsName}_h_files
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
