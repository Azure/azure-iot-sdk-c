#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

#add_unittest_directory(dps_sec_client_ut)
add_unittest_directory(prov_transport_http_client_ut)

if (${use_amqp})
    add_unittest_directory(prov_transport_amqp_client_ut)
    add_unittest_directory(prov_transport_amqp_ws_client_ut)
    add_unittest_directory(prov_transport_amqp_common_ut)
    add_unittest_directory(prov_sasl_tpm_ut)
endif()

if (${use_mqtt})
    add_unittest_directory(prov_transport_mqtt_client_ut)
    add_unittest_directory(prov_transport_mqtt_ws_client_ut)
    add_unittest_directory(prov_transport_mqtt_common_ut)
endif()

add_unittest_directory(prov_device_client_ut)
add_unittest_directory(prov_security_factory_ut)

if(${dps_hsm_x509})
    add_unittest_directory(hsm_client_riot_ut)
    #add_e2etest_directory(dps_client_e2e)
endif()

if(${dps_hsm_sastoken})
    add_unittest_directory(hsm_client_tpm_ut)
endif()

add_unittest_directory(iothub_auth_client_ut)
