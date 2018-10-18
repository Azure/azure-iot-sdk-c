#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

#Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(HSM_INCLUDE_DIR "../../adapters")

include_directories(. ${HSM_INCLUDE_DIR})

set(source_files
    ./custom_hsm_example.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ELSE()
    # Make warning as error
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic -Wno-variadic-macros -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Wno-variadic-macros -fPIC")
ENDIF(WIN32)

add_library(custom_hsm_example ${source_files})
