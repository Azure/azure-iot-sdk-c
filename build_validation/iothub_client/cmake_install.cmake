# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azureiot" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_core.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_core_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_core_common.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_diagnostic.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_properties.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_internal_consts.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_options.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_client_version.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_device_client.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_device_client_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_module_client.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_module_client_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_transport_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_message.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_message_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/blob.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_ll_uploadtoblob.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_hsm_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_authorization.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_retry_control.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_transport_ll_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothubtransporthttp.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothub_transport_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_authorization.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_retry_control.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_transport_ll_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_common.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_device.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_cbs_auth.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_connection.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_telemetry_messenger.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_twin_messenger.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_amqp_messenger.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransportamqp_methods.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/message_queue.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/uamqp_messaging.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothubtransportamqp.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothubtransportamqp_websockets.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_authorization.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_retry_control.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_transport_ll_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_mqtt_common.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothubtransportmqtt.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_authorization.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_client_retry_control.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothub_transport_ll_private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/internal/iothubtransport_mqtt_common.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_client/inc/iothubtransportmqtt_websockets.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client_http_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client_http_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client_http_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client_http_transport.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client_amqp_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client_amqp_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client_amqp_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client_amqp_transport.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client_amqp_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client_amqp_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client_amqp_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client_amqp_ws_transport.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client_mqtt_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client_mqtt_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client_mqtt_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client_mqtt_transport.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Debug/iothub_client_mqtt_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/Release/iothub_client_mqtt_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/MinSizeRel/iothub_client_mqtt_ws_transport.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/RelWithDebInfo/iothub_client_mqtt_ws_transport.lib")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/tests/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_client/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
