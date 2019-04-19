#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName iothubclient_uploadtoblob_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_nuget_test_files
    ${theseTestsName}.c
    ../../../certs/certs.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})

file(COPY ../global_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests" VALGRIND_SUPPRESSIONS_FILE global_valgrind_suppression.supp)

if(WIN32)
   if(TARGET ${theseTestsName}_dll)
           target_link_libraries(${theseTestsName}_dll
                   iothub_test
                   iothub_client
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           addSupportedTransportsToTest(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           addSupportedTransportsToTest(${theseTestsName}_exe)
   endif()

   if(TARGET ${theseTestsName}_nuget_exe)
           target_link_libraries(${theseTestsName}_nuget_exe
                   iothub_test
                   iothub_service_client
                   rpcrt4
        )
    endif()
else()
    if(APPLE)
        target_link_libraries(${theseTestsName}_exe pthread)
    elseif(LINUX)
        target_link_libraries(${theseTestsName}_exe pthread)
    endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_service_client
                   aziotsharedutil
           )
           addSupportedTransportsToTest(${theseTestsName}_exe)
   endif()
endif()
