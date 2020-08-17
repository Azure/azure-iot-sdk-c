#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

project(azure_security)

if(not(linux))
	message(fatal_error "azure security is intended for linux platforms only")
else()
    set(cmake_cxx_flags "${cmake_cxx_flags} -werror")
    set(cmake_c_flags "${cmake_c_flags} -werror")
endif()

set(AZURE_SECURITY_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using Azure Security lib" FORCE)

set(azure_security_c_files
    ./src/azure_security.c
)

set(azure_security_h_files
    ./inc/azure_security.h
)

include_directories(
	${AZURE_SECURITY_INC_FOLDER}
	${IOTHUB_CLIENT_INC_FOLDER}
)

add_library(azure_security ${azure_security_c_files} ${azure_security_h_files})


