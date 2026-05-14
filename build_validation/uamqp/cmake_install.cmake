# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp

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
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/Debug/uamqp.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/Release/uamqp.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/MinSizeRel/uamqp.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/RelWithDebInfo/uamqp.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azureiot/azure_uamqp_c" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_role.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sender_settle_mode.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_receiver_settle_mode.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_handle.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_seconds.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_milliseconds.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_delivery_tag.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sequence_no.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_delivery_number.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_transfer_number.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_format.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_ietf_language_tag.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_fields.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_amqp_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_connection_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_session_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_link_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_open.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_begin.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_attach.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_flow.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_transfer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_disposition.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_detach.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_end.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_close.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_code.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_mechanisms.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_init.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_challenge.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_response.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_sasl_outcome.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_terminus_durability.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_terminus_expiry_policy.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_node_properties.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_filter_set.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_source.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_target.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_annotations.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_id_ulong.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_id_uuid.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_id_binary.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_id_string.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_address_string.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_header.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_delivery_annotations.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_message_annotations.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_application_properties.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_data.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_amqp_sequence.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_amqp_value.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_footer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_properties.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_received.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_accepted.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_rejected.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_released.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions_modified.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_definitions.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_frame_codec.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_management.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqp_types.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqpvalue.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/amqpvalue_to_string.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/async_operation.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/cbs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/connection.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/frame_codec.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/header_detect_io.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/link.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/message.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/message_receiver.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/message_sender.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/messaging.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_anonymous.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_frame_codec.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_mechanism.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_server_mechanism.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_mssbcbs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_plain.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/saslclientio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/sasl_server_io.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/server_protocol_io.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/session.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/socket_listener.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/./inc/azure_uamqp_c/uamqp.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/uamqpTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/uamqpTargets.cmake"
         "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/uamqpTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/uamqpTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/uamqpTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/uamqp/configs/uamqpConfig.cmake"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/uamqp/uamqpConfigVersion.cmake"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/uamqp/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
