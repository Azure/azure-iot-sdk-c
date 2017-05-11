#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(${LINUX})
   add_definitions(-DAZIOT_LINUX)
endif()

#this is CMakeLists for iothub_client tests folder
add_unittest_directory(iothub_client_authorization_ut)
add_unittest_directory(iothubclient_ll_ut)
if(NOT ${dont_use_uploadtoblob})
    add_unittest_directory(iothubclient_ll_u2b_ut)
endif()

add_unittest_directory(iothubclient_ut)
add_unittest_directory(iothubmessage_ut)
add_unittest_directory(iothubtransport_ut)
add_unittest_directory(blob_ut)
add_unittest_directory(iothub_client_retry_control_ut)

if(${use_http})
    add_unittest_directory(iothubtransporthttp_ut)

    add_e2etest_directory(iothubclient_http_e2e)
#   add_e2etest_directory(iothubclient_http_e2e_svc_fault_ctrl)
endif()

if(${use_mqtt})
    add_unittest_directory(iothubtransportmqtt_ut)
    add_unittest_directory(iothubtransport_mqtt_common_ut)
    add_unittest_directory(iothubtransportmqtt_ws_ut)

    add_e2etest_directory(iothubclient_mqtt_e2e)
#   add_e2etest_directory(iothubclient_mqtt_e2e_svc_fault_ctrl)
    add_e2etest_directory(iothubclient_mqtt_dt_e2e)
    add_e2etest_directory(iothubclient_mqtt_device_method_e2e)
    add_e2etest_directory(iothubclient_mqtt_ws_e2e)
#   add_e2etest_directory(iothubclient_mqtt_ws_e2e_svc_fault_ctrl)
endif()

if(${use_amqp})
    add_unittest_directory(uamqp_messaging_ut)
    add_unittest_directory(iothubtransport_amqp_common_ut)
    add_unittest_directory(iothubtransport_amqp_device_ut)
    add_unittest_directory(iothubtransport_amqp_cbs_auth_ut)
    if(${wip_use_c2d_amqp_methods})
        add_unittest_directory(iothubtransportamqp_methods_ut)
    endif()
    add_unittest_directory(iothubtransport_amqp_connection_ut)
    add_unittest_directory(iothubtransport_amqp_messenger_ut)
    add_unittest_directory(iothubtransportamqp_ut)
    add_unittest_directory(iothubtransportamqp_ws_ut)
    
    add_e2etest_directory(iothubclient_amqp_e2e)
#   add_e2etest_directory(iothubclient_amqp_e2e_svc_fault_ctrl)
    if(${wip_use_c2d_amqp_methods})
        add_e2etest_directory(iothubclient_amqp_device_method_e2e)
    endif()
    add_e2etest_directory(iothubclient_amqp_ws_e2e)
#   add_e2etest_directory(iothubclient_amqp_ws_e2e_svc_fault_ctrl)
 
    if (${run_longhaul_tests})
        add_unittest_directory(longhaul_tests)
    endif()
endif()

add_unittest_directory(version_ut)
