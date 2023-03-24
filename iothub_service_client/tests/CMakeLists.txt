#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

usePermissiveRulesForSdkSamplesAndTests()

if(WIN32 AND (MSVC_VERSION GREATER_EQUAL 1700))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
endif()

#this is CMakeLists for service tests folder

add_subdirectory(iothub_deviceconfiguration_ut)
add_subdirectory(iothub_devicemethod_ut)
add_subdirectory(iothub_devicetwin_ut)
add_subdirectory(iothub_msging_ll_ut)
add_subdirectory(iothub_msging_ut)
add_subdirectory(iothub_rm_ut)
add_subdirectory(iothub_sc_version_ut)
add_subdirectory(iothub_srv_client_auth_ut)

if (${run_e2e_tests})
endif()
