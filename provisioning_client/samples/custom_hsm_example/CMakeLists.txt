#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

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

# Ensure this is built with c99
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set_target_properties(custom_hsm_example PROPERTIES COMPILE_FLAGS "--std=c99")
    endif()
else()
    set_target_properties(custom_hsm_example PROPERTIES C_STANDARD 99)
    set_target_properties(custom_hsm_example PROPERTIES CXX_STANDARD 11)
endif()
