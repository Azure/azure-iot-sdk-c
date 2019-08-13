#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for version
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName dt_interface_client_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER} ${IOTHUB_TEST_REAL_INC_FOLDER})

set(${theseTestsName}_c_files
../../src/dt_interface_client.c
../../../serializer/tests/serializer_dt_ut/real_crt_abstractions.c
${IOTHUB_TEST_REAL_SRC_FOLDER}/real_parson.c
${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
