#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for device_auth_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName hsm_client_tpm_ut)

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../adapters/hsm_client_tpm.c
)

set(${theseTestsName}_h_files
)

file(COPY ../common_prov_e2e/prov_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_device_tests"
    ADDITIONAL_LIBS
        hsm_security_client
    VALGRIND_SUPPRESSIONS_FILE 
        prov_valgrind_suppression.supp
)
