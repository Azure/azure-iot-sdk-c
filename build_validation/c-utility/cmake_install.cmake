# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility

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
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/Debug/aziotsharedutil.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/Release/aziotsharedutil.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/MinSizeRel/aziotsharedutil.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/RelWithDebInfo/aziotsharedutil.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/Debug/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/Release/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/MinSizeRel/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/RelWithDebInfo/c_logging_v2.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/Debug/c_logging_v2_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/Release/c_logging_v2_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/MinSizeRel/c_logging_v2_core.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/deps/c-logging/v2/RelWithDebInfo/c_logging_v2_core.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azure_c_shared_utility" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/agenttime.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/azure_base32.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/azure_base64.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/buffer_.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/constbuffer_array.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/constbuffer_array_batcher.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/connection_string_parser.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/crt_abstractions.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/constmap.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/condition.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/const_defines.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/inc/azure_c_shared_utility/consolelogger.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/doublylinkedlist.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/envvariable.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/gballoc.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/gbnetwork.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/gb_stdio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/gb_time.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/hmac.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/hmacsha256.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/http_proxy_io.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/singlylinkedlist.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/lock.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/map.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/optimize_size.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/platform.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/refcount.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/sastoken.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/sha-private.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/shared_util_options.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/sha.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/socketio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/srw_lock.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/stdint_ce6.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/strings.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/strings_types.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/string_token.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/string_tokenizer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/string_tokenizer_types.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/string_utils.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/tlsio_options.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/tickcounter.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/threadapi.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/xio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/uniqueid.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/uuid.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/urlencode.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/vector.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/vector_types.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/vector_types_internal.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/xlogging.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/constbuffer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/tlsio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/optionhandler.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/memory_data.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/safe_math.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/wsio.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/uws_client.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/uws_frame_encoder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/utf8_checker.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/ws_url.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/httpapi.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/httpapiex.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/httpapiexsas.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/httpheaders.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/tlsio_schannel.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/x509_schannel.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./inc/azure_c_shared_utility/csr_gen.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/./pal/windows/refcount_os.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility/azure_c_shared_utilityTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility/azure_c_shared_utilityTargets.cmake"
         "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility/azure_c_shared_utilityTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility/azure_c_shared_utilityTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/CMakeFiles/Export/d38edef68031dc399a64718639337bf5/azure_c_shared_utilityTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/azure_c_shared_utility" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityConfig.cmake"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_c_shared_utilityFunctions.cmake"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/configs/azure_iot_build_rules.cmake"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/azure_c_shared_utility/azure_c_shared_utilityConfigVersion.cmake"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/c-utility/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
