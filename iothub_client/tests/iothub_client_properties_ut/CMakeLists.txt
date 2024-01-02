#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for version
cmake_minimum_required(VERSION 3.5)

compileAsC99()
set(theseTestsName iothub_client_properties_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER} ${IOTHUB_TEST_REAL_INC_FOLDER})

set(${theseTestsName}_c_files
  ../../src/iothub_client_properties.c
  ${IOTHUB_TEST_REAL_SRC_FOLDER}/real_parson.c
  ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
)

set(${theseTestsName}_h_files
    ${IOTHUB_TEST_REAL_INC_FOLDER}/real_parson.h
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
