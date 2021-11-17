#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName serializer_dt_ut)

include_directories(${SERIALIZER_INC_FOLDER})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
)

set(${theseTestsName}_h_files
    ../../../deps/parson/parson.h
    ./real_vector.h
    ./real_crt_abstractions.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")