#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName serializer_int)

include_directories(${SERIALIZER_INC_FOLDER})

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../../deps/parson/parson.c
)

if(MSVC)
    set_source_files_properties(../../../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()


set(${theseTestsName}_h_files
    ../../../deps/parson/parson.h
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests" ADDITIONAL_LIBS serializer aziotsharedutil)