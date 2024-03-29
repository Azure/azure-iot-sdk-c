#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for codefirst_cpp_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName codefirst_cpp_ut)

set(${theseTestsName}_test_files
${theseTestsName}.cpp
)

include_directories(${SHARED_UTIL_REAL_TEST_FOLDER})

set(${theseTestsName}_c_files
    ../../src/codefirst.c
    ./c_bool_size.c
    ${SHARED_UTIL_SRC_FOLDER}/gballoc.c
    ${SHARED_UTIL_SRC_FOLDER}/crt_abstractions.c
    ${LOCK_C_FILE}
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_vector.c
    ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.c
)

set(${theseTestsName}_h_files
    ./c_bool_size.h
        ${SHARED_UTIL_REAL_TEST_FOLDER}/real_strings.h
)

#CPP compiler on windows likes to complain about unused local function removed (C4505)
#C compiler doesn't like to complain about the same thing
if(WIN32)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505")
elseif(LINUX OR MACOSX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
