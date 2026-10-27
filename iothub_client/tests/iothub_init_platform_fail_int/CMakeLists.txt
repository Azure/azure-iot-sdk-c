#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Plain executable, not a ctest suite: ctest logs through c-logging and needs logger_init, which this test
# must not call. iothub.c runs against a platform stub that fails and does not initialize c-logging, so only
# IoTHub_Init's own logger reference keeps its failure log from aborting.
add_executable(iothub_init_platform_fail_int_exe
    iothub_init_platform_fail_int.c
    ../../src/iothub.c
)
target_include_directories(iothub_init_platform_fail_int_exe PRIVATE ${IOTHUB_CLIENT_INC_FOLDER})
target_link_libraries(iothub_init_platform_fail_int_exe aziotsharedutil)
set_target_properties(iothub_init_platform_fail_int_exe PROPERTIES FOLDER "tests/iothub_client_tests")
add_test(NAME iothub_init_platform_fail_int COMMAND iothub_init_platform_fail_int_exe)
