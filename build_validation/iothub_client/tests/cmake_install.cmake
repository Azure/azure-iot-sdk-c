# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/tests

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/azure_iot_sdks")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothub_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothub_client_authorization_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothub_transport_ll_private_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclient_ll_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclientcore_ll_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclient_diagnostic_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubdeviceclient_ll_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclient_ll_u2b_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/blob_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclient_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubclientcore_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubdeviceclient_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubmessage_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothub_client_properties_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothub_client_retry_control_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/message_queue_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubmoduleclient_ll_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubmoduleclient_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransporthttp_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransportmqtt_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_mqtt_common_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_mqtt_common_int_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransportmqtt_ws_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/uamqp_messaging_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_amqp_common_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_amqp_device_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_amqp_cbs_auth_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransportamqp_methods_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransport_amqp_connection_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtr_amqp_tel_msgr_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtr_amqp_msgr_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtr_amqp_twin_msgr_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransportamqp_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/iothubtransportamqp_ws_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/version_ut/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
