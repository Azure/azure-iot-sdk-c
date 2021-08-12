#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for datamarshaller_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName datamarshaller_ut)

generate_cppunittest_wrapper(${theseTestsName})

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER} ${IOTHUB_TEST_REAL_INC_FOLDER})

set(${theseTestsName}_c_files
    ${IOTHUB_TEST_REAL_SRC_FOLDER}/real_parson.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
    ../../src/datamarshaller.c
${LOCK_C_FILE}
)

set(${theseTestsName}_h_files
    ${IOTHUB_TEST_REAL_INC_FOLDER}/real_parson.h
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.h
)

if(MSVC)
    set_source_files_properties(${IOTHUB_TEST_REAL_SRC_FOLDER}/real_parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()


build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")