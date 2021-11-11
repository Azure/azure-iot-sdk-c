#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for device_auth_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName hsm_client_riot_ut)

include_directories(../../deps/RIoT/Simulation/DICE)
include_directories(../../deps/RIoT/Simulation/RIoT/Core)
include_directories(../../deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include)

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../adapters/hsm_client_riot.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_device_tests"
    ADDITIONAL_LIBS
        hsm_security_client
)

#Set RIoT ECDH Curve & RIOTBUILD
target_compile_definitions(${theseTestsName}_exe
    PUBLIC
        RIOTBUILD
        RIOTSECP256R1
    )
if(WIN32 AND use_cppunittest)
    target_compile_definitions(${theseTestsName}_dll
        PUBLIC
            RIOTBUILD
            RIOTSECP256R1
    )
    target_compile_definitions(${theseTestsName}_testsonly_lib
        PUBLIC
            RIOTBUILD
            RIOTSECP256R1
    )
endif()

