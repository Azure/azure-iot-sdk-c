#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for version
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName dt_model_definition_ut)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER} ${IOTHUB_TEST_REAL_INC_FOLDER})

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
${SHARED_UTIL_REAL_TEST_FOLDER}/real_map.c
${IOTHUB_TEST_REAL_SRC_FOLDER}/real_parson.c
../../src/dt_model_definition.c
)

set(${theseTestsName}_h_files
${SHARED_UTIL_REAL_TEST_FOLDER}/real_map.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
