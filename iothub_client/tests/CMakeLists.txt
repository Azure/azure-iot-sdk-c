#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

if(${LINUX})
   add_definitions(-DAZIOT_LINUX)
endif()

# addSupportedTransportsToTest determines transport dependencies based on which transports are enabled via cmake and
# sets appropriate TEST_xyz #ifdef's so tests themselves are compiled against appropriate targets.
function(addSupportedTransportsToTest whatExecutableIsBuilding)
    if(${use_mqtt})
    	linkMqttLibrary(${whatExecutableIsBuilding})
    	target_link_libraries(${whatExecutableIsBuilding} umqtt iothub_client_mqtt_transport iothub_client_mqtt_ws_transport)
    	add_definitions(-DTEST_MQTT)
    endif()

    if(${use_amqp})
    	target_link_libraries(${whatExecutableIsBuilding} iothub_client_amqp_transport iothub_client_amqp_ws_transport)
        linkUAMQP(${whatExecutableIsBuilding})
    	add_definitions(-DTEST_AMQP)
    endif()

    if(${use_http})
    	target_link_libraries(${whatExecutableIsBuilding} iothub_client_http_transport)
    	add_definitions(-DTEST_HTTP)
    endif()
endfunction()

#this is CMakeLists for iothub_client tests folder
add_unittest_directory(iothub_client_authorization_ut)
add_unittest_directory(iothubclient_ll_ut)
add_unittest_directory(iothubclient_diagnostic_ut)
if(NOT ${dont_use_uploadtoblob})
    add_unittest_directory(iothubclient_ll_u2b_ut)
endif()

add_unittest_directory(iothubclient_ut)
add_unittest_directory(iothubmessage_ut)
add_unittest_directory(iothubtransport_ut)
add_unittest_directory(blob_ut)
add_unittest_directory(iothub_client_retry_control_ut)
add_unittest_directory(message_queue_ut)

add_e2etest_directory(iothubclient_uploadtoblob_e2e)

if(${use_http})
    add_unittest_directory(iothubtransporthttp_ut)
    add_e2etest_directory(iothubclient_http_e2e)
endif()

if(${use_mqtt})
    add_unittest_directory(iothubtransportmqtt_ut)
    add_unittest_directory(iothubtransport_mqtt_common_ut)
    add_unittest_directory(iothubtransportmqtt_ws_ut)

    add_e2etest_directory(iothubclient_mqtt_e2e)
    # add_e2etest_directory(iothubclient_mqtt_e2e_sfc)
    add_e2etest_directory(iothubclient_mqtt_dt_e2e)
    # add_e2etest_directory(iothubclient_mqtt_dt_e2e_sfc)
    add_e2etest_directory(iothubclient_mqtt_device_method_e2e)
    # add_e2etest_directory(iothubclient_mqtt_dm_e2e_sfc)
    add_e2etest_directory(iothubclient_mqtt_ws_e2e)
    # add_e2etest_directory(iothubclient_mqtt_ws_e2e_sfc)
    # add_e2etest_directory(iothubclient_mqtt_ws_e2e_sfc)
endif()

if(${use_amqp})
    add_unittest_directory(uamqp_messaging_ut)
    add_unittest_directory(iothubtransport_amqp_common_ut)
    add_unittest_directory(iothubtransport_amqp_device_ut)
    add_unittest_directory(iothubtransport_amqp_cbs_auth_ut)
    add_unittest_directory(iothubtransportamqp_methods_ut)
    add_unittest_directory(iothubtransport_amqp_connection_ut)
    add_unittest_directory(iothubtr_amqp_tel_msgr_ut)
    add_unittest_directory(iothubtr_amqp_msgr_ut)
    add_unittest_directory(iothubtr_amqp_twin_msgr_ut)
    add_unittest_directory(iothubtransportamqp_ut)
    add_unittest_directory(iothubtransportamqp_ws_ut)
    
    add_e2etest_directory(iothubclient_amqp_e2e)
    add_e2etest_directory(iothubclient_amqp_dt_e2e)
    # add_e2etest_directory(iothubclient_amqp_e2e_sfc)
    add_e2etest_directory(iothubclient_amqp_device_method_e2e)
    add_e2etest_directory(iothubclient_amqp_ws_e2e)
    # add_e2etest_directory(iothubclient_amqp_ws_e2e_sfc)
 
    if (${run_longhaul_tests})
        add_unittest_directory(longhaul_tests)
    endif()
endif()

add_unittest_directory(version_ut)
