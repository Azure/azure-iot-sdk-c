#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

#Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# To be implemented after HSM module is in use
#if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/hsm/CMakeLists.txt")
#    message("running git clone of the c-sdk into ${CMAKE_CURRENT_SOURCE_DIR}/deps/hsm")
#    execute_process (COMMAND git clone https://github.com/Azure/azure-iot-hsm-c.git ${CMAKE_CURRENT_SOURCE_DIR}/deps/hsm)
#endif()

set(IOT_HSM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/hsm/inc)

include_directories(. ${IOT_HSM_INCLUDE_DIR} ${UTPM_INCLUDE_DIR} ../../../adapters)

set(source_c_files
    ./tpm_msr.c
    ./x509_msr.c
    ./symm_key.c
    ./provisioning_hsm.c
)

set(source_h_files
    ./tpm_msr.h
    ./x509_info.h
    ./symm_key.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # Make warning as error
    add_definitions(/WX)
ELSE()
    # Make warning as error
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF(WIN32)

add_library(prov_hsm ${source_c_files} ${source_h_files})
target_link_libraries(prov_hsm msr_riot utpm)

set_target_properties(prov_hsm PROPERTIES FOLDER "tests/azure_prov_e2e_tests")
