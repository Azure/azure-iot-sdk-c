#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName device_auth_emulator_ut)

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
../../src/device_auth_emulator.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_provisioning_tests")
