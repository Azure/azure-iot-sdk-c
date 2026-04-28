#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# NOTE: Serializer tests are currently disabled at the parent CMakeLists.txt level.
# The serializer relies on MU_C1 and complex DECLARE_MODEL/DECLARE_STRUCT macro expansions
# from the old azure-macro-utils-c that do not exist in the new macro-utils-c.
# Several tests also use the legacy micromock framework instead of umock-c.
# The serializer library itself compiles and links correctly; only its test suite is affected.

cmake_minimum_required (VERSION 3.5)

usePermissiveRulesForSdkSamplesAndTests()

if(LINUX OR MACOSX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

#this is CMakeLists for serializer e2e folder
if(${run_unittests})
add_subdirectory(agentmacros_ut)
add_subdirectory(agenttypesystem_ut)
add_subdirectory(codefirst_cpp_ut)
add_subdirectory(codefirst_ut)
add_subdirectory(codefirst_withstructs_cpp_ut)
add_subdirectory(codefirst_withstructs_ut)
add_subdirectory(commanddecoder_ut)
add_subdirectory(datamarshaller_ut)
add_subdirectory(datapublisher_ut)
add_subdirectory(dataserializer_ut)
add_subdirectory(iotdevice_ut)
add_subdirectory(jsondecoder_ut)
add_subdirectory(jsonencoder_ut)
add_subdirectory(multitree_ut)
add_subdirectory(schema_ut)
add_subdirectory(schemalib_ut)
add_subdirectory(schemalib_without_init_ut)
add_subdirectory(schemaserializer_ut)
add_subdirectory(methodreturn_ut)
add_subdirectory(serializer_int)
add_subdirectory(serializer_dt_int)
add_subdirectory(serializer_dt_ut)
endif()

if(${use_amqp} AND ${use_http} AND ${run_e2e_tests})
    add_subdirectory(serializer_e2e)
endif()
