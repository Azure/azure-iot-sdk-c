#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName methodreturn_ut)

include_directories(${SERIALIZER_INC_FOLDER})

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../src/methodreturn.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
)

set(${theseTestsName}_h_files
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
