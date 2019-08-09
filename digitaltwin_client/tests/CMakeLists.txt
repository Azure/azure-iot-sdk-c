#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

compileAsC11()

if(${LINUX})
   add_definitions(-DAZIOT_LINUX)
endif()

usePermissiveRulesForSdkSamplesAndTests()

add_unittest_directory(dt_lock_thread_binding_impl_ut)
add_unittest_directory(dt_lock_thread_binding_stub_ut)
add_unittest_directory(dt_client_core_ut)
add_unittest_directory(dt_device_ut)
add_unittest_directory(dt_device_ll_ut)
add_unittest_directory(dt_interface_client_ut)
add_unittest_directory(dt_interface_list_ut)
add_unittest_directory(dt_version_ut)
add_unittest_directory(dt_model_definition_ut)