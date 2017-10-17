#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(getIoTSDKVersion)
	# First find the applicable line in the file
	file (STRINGS "${CMAKE_SOURCE_DIR}/iothub_client/inc/iothub_client_version.h" iotsdkverstr
		REGEX "^[\t ]*#[\t ]*define[\t ]*IOTHUB_SDK_VERSION[\t ]*\"([0-9]+)[.]([0-9]+)[.]([0-9]+)\"")
		
	if (!MATCHES)
		message(FATAL_ERROR "Unable to find version in ${CMAKE_SOURCE_DIR}/iothub_client/inc/iothub_client_version.h")
	else(!MATCHES)
		# Parse out the three version identifiers
		set(CMAKE_MATCH_3 "")

		string(REGEX MATCH "^[\t ]*#[\t ]*define[\t ]*IOTHUB_SDK_VERSION[\t ]*\"([0-9]+)[.]([0-9]+)[.]([0-9]+)\"" temp "${iotsdkverstr}")

		if (CMAKE_MATCH_3)
			set (IOT_SDK_VERION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
			set (IOT_SDK_VERION_MINOR "${CMAKE_MATCH_2}" PARENT_SCOPE)
			set (IOT_SDK_VERION_FIX "${CMAKE_MATCH_3}" PARENT_SCOPE)
			set (IOT_SDK_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE)
		else (CMAKE_MATCH_3)
			message(FATAL_ERROR "Unable to find version in ${iotsdkverstr}")
		endif(CMAKE_MATCH_3)
	endif(!MATCHES)
endfunction(getIoTSDKVersion)

function(linkUAMQP whatExecutableIsBuilding)
    include_directories(${UAMQP_INC_FOLDER})
    
    if(WIN32)
        #windows needs this define
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)

        target_link_libraries(${whatExecutableIsBuilding} uamqp aziotsharedutil ws2_32 secur32)

        if(${use_openssl})
            target_link_libraries(${whatExecutableIsBuilding} $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        
            file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
            file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)

            file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
            file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
        endif()
    else()
        target_link_libraries(${whatExecutableIsBuilding} uamqp aziotsharedutil ${OPENSSL_LIBRARIES})
    endif()
endfunction(linkUAMQP)

function(includeMqtt)
    include_directories(${MQTT_INC_FOLDER})
endfunction(includeMqtt)

function(linkMqttLibrary whatExecutableIsBuilding)
    includeMqtt()
    target_link_libraries(${whatExecutableIsBuilding} umqtt)
endfunction(linkMqttLibrary)

function(linkUHTTP whatExecutableIsBuilding)
    include_directories(${UHTTP_C_INC_FOLDER})
    target_link_libraries(${whatExecutableIsBuilding} uhttp)
endfunction(linkUHTTP)

function(includeHttp)
endfunction(includeHttp)

function(linkHttp whatExecutableIsBuilding)
    includeHttp()
    if(WIN32)
        if(WINCE)
              target_link_libraries(${whatExecutableIsBuilding} crypt32.lib)
          target_link_libraries(${whatExecutableIsBuilding} ws2.lib)
        else()
            target_link_libraries(${whatExecutableIsBuilding} winhttp.lib)
        endif()
    else()
        target_link_libraries(${whatExecutableIsBuilding} curl)
    endif()
endfunction(linkHttp)

function(linkSharedUtil whatIsBuilding)
    target_link_libraries(${whatIsBuilding} aziotsharedutil)
endfunction(linkSharedUtil)

function(add_unittest_directory test_directory)
    if (${run_unittests})
        add_subdirectory(${test_directory})
    endif()
endfunction()

function(add_e2etest_directory test_directory)
    if (${run_e2e_tests} OR ${nuget_e2e_tests})
        add_subdirectory(${test_directory})
    endif()
endfunction()
