#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

IF(WIN32)
	# Required files for running test in cmake binary directory
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\mocha_exec.cmd DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\dt_e2e_test.ts DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\iothub_test_core.ts DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\package.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	
	# node npm module install now happens in test script itself, to reduce complexity of pre run steps
	add_test(NAME "iothubclient_dt_e2e" 
			 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			 COMMAND mocha_exec.cmd ${CMAKE_CURRENT_BINARY_DIR})
ENDIF(WIN32)

IF(UNIX)
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\mocha_exec.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\dt_e2e_test.ts DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\iothub_test_core.ts DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}\\package.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

	# node npm module install now happens in test script itself, to reduce complexity of build for those who don't want to run this e2e
	add_test(NAME "iothubclient_dt_e2e" 
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		 COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}\\mocha_exec.sh)
ENDIF(UNIX)
	
