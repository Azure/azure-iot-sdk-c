#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubmoduleclient_ll_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()

set(theseTestsName iothubmoduleclient_ll_ut)

generate_cppunittest_wrapper(${theseTestsName})

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

set(${theseTestsName}_c_files
    ../../src/iothub_module_client_ll.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
