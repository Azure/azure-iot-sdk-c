#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubclient_edge_ut
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()

set(theseTestsName iothubclient_edge_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})
include_directories(../../../deps/parson/)

set(${theseTestsName}_c_files
    ../../src/iothub_client_edge.c
)

set(${theseTestsName}_h_files
    ../../../deps/parson/parson.h
)


build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")