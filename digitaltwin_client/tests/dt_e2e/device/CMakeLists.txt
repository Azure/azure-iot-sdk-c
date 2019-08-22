#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for dt_e2e

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName dt_e2e)

set(${theseTestsName}_test_files
    dt_e2e_commands.c
    dt_e2e_telemetry.c
    dt_e2e_properties.c
    dt_e2e_util.c
    main.c
)

set(${theseTestsName}_test_h_files
    dt_e2e_commands.h
    dt_e2e_properties.h
    dt_e2e_telemetry.h
    dt_e2e_util.h
)

include_directories(. ${SHARED_UTIL_INC_FOLDER} ${IOTHUB_CLIENT_INC_FOLDER} ${DIGITALTWIN_CLIENT_INC_FOLDER} ${IOTHUB_TEST_INC_FOLDER})

add_executable(dt_e2e_exe ${dt_e2e_test_files})

if(WIN32)
   if(TARGET ${theseTestsName}_dll)
           target_link_libraries(${theseTestsName}_dll
           	iothub_client
		digitaltwin_client
           	rpcrt4
           )
           linkSharedUtil(${theseTestsName}_dll)
           linkMqttLibrary(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
           	iothub_client
	    	digitaltwin_client
           	iothub_service_client
           	aziotsharedutil
           	rpcrt4
           )
           linkSharedUtil(${theseTestsName}_exe)
           linkMqttLibrary(${theseTestsName}_exe)
   endif()

#else()
 #   if(UNIX) #LINUX OR APPLE
  #      find_package(PkgConfig REQUIRED)
   #     pkg_search_module(UUID REQUIRED uuid)
    #    link_directories(${UUID_LIBRARY_DIRS})
#    endif()

 #   if(APPLE)
  #      target_link_libraries(${theseTestsName}_exe -L${UUID_LIBRARY_DIRS} pthread ${UUID_LIBRARIES})
  #  elseif(LINUX)
   #     target_link_libraries(${theseTestsName}_exe pthread ${UUID_LIBRARIES})
   # endif()

#   if(TARGET ${theseTestsName}_exe)
 #          target_link_libraries(${theseTestsName}_exe
  #         	iothub_client
#		digitaltwin_client
 #          	iothub_service_client
  #          	aziotsharedutil
   #        )
    #       linkSharedUtil(${theseTestsName}_exe)
	#   linkMqttLibrary(${theseTestsName}_exe)
#   endif()
endif()
