#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for device_auth_ut
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName hsm_client_riot_ut)

include_directories(../../deps/RIoT/Emulator/DICE)
include_directories(../../deps/RIoT/Emulator/RIoT)
include_directories(../../deps/RIoT/Emulator/RIoT/RIoTCrypt/include)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../adapters/hsm_client_riot.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_device_tests")
