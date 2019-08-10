#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for version
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName dt_lock_thread_binding_stub_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)


set(${theseTestsName}_c_files
../../src/dt_lock_thread_binding_stub.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/UnitTests")
