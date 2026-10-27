#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Plain executable, not a ctest suite: ctest logs through c-logging and needs logger_init, which this test
# must not call. Without logger_init in IoTHub_Init/platform_init, the SDK's first log line aborts the process.
add_executable(iothub_init_logging_int_exe iothub_init_logging_int.c)
target_link_libraries(iothub_init_logging_int_exe iothub_client)
addSupportedTransportsToTest(iothub_init_logging_int_exe)
linkSharedUtil(iothub_init_logging_int_exe)
set_target_properties(iothub_init_logging_int_exe PROPERTIES FOLDER "tests/iothub_client_tests")
add_test(NAME iothub_init_logging_int COMMAND iothub_init_logging_int_exe)
