#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for datapublisher_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName datapublisher_ut)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../src/datapublisher.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
)

set(${theseTestsName}_h_files
)

#datapublisher is special and needs some data type definitions from iothub_client
#but shouldn't...
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")