#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubclientcore_ut
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()

set(theseTestsName iothubclientcore_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

set(${theseTestsName}_c_files
    ../../src/iothub_client_core.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
