# CMake generated Testfile for 
# Source directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut
# Build directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/iothub_client/tests/uamqp_messaging_ut
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(uamqp_messaging_ut "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/Debug/uamqp_messaging_ut_exe.exe")
  set_tests_properties(uamqp_messaging_ut PROPERTIES  _BACKTRACE_TRIPLES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;402;add_test;C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;529;c_windows_unittests_add_exe;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;18;build_c_test_artifacts;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(uamqp_messaging_ut "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/Release/uamqp_messaging_ut_exe.exe")
  set_tests_properties(uamqp_messaging_ut PROPERTIES  _BACKTRACE_TRIPLES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;402;add_test;C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;529;c_windows_unittests_add_exe;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;18;build_c_test_artifacts;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(uamqp_messaging_ut "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/MinSizeRel/uamqp_messaging_ut_exe.exe")
  set_tests_properties(uamqp_messaging_ut PROPERTIES  _BACKTRACE_TRIPLES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;402;add_test;C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;529;c_windows_unittests_add_exe;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;18;build_c_test_artifacts;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(uamqp_messaging_ut "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/RelWithDebInfo/uamqp_messaging_ut_exe.exe")
  set_tests_properties(uamqp_messaging_ut PROPERTIES  _BACKTRACE_TRIPLES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;402;add_test;C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake;529;c_windows_unittests_add_exe;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;18;build_c_test_artifacts;C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests/uamqp_messaging_ut/CMakeLists.txt;0;")
else()
  add_test(uamqp_messaging_ut NOT_AVAILABLE)
endif()
