#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubclient_mqtt_device_method_e2e
cmake_minimum_required(VERSION 2.8.11)

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${IOTHUB_CLIENT_HTTP_TRANSPORT_INC_FOLDER})
include_directories(${UAMQP_INCLUDES})
include_directories(${IOTHUB_CLIENT_AMQP_TRANSPORT_INC_FOLDER} ${UAMQP_INC_FOLDER})
include_directories(${iothub_client_amqp_ws_transport_INC_FOLDER} ${UAMQP_INC_FOLDER})
include_directories(${UMQTT_INCLUDES})
include_directories(${IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER} ${MQTT_INC_FOLDER})


add_subdirectory(iothubclient_badnetwork_e2e)

