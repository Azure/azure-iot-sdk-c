#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

add_unittest_directory(device_auth_ut)
add_unittest_directory(dps_sec_client_ut)
add_unittest_directory(dps_transport_http_client_ut)

if (${use_amqp} AND ${DPS_PROTOCOL_FEATURE})
    add_unittest_directory(dps_transport_amqp_client_ut)
    add_unittest_directory(dps_transport_amqp_ws_client_ut)
    add_unittest_directory(dps_transport_amqp_common_ut)
    add_unittest_directory(dps_sasl_tpm_ut)
endif()

if (${use_mqtt} AND ${DPS_PROTOCOL_FEATURE})
    add_unittest_directory(dps_transport_mqtt_client_ut)
    add_unittest_directory(dps_transport_mqtt_ws_client_ut)
    add_unittest_directory(dps_transport_mqtt_common_ut)
endif()

add_unittest_directory(iothub_drs_client_ut)

if(${dps_hsm_x509})
    add_unittest_directory(iothub_security_x509_ut)
    add_unittest_directory(dps_hsm_riot_ut)
    #add_e2etest_directory(dps_client_e2e)
else()
    add_unittest_directory(iothub_security_sas_ut)
    add_unittest_directory(dps_hsm_tpm_ut)
endif()

add_unittest_directory(uhttp_ut)
add_unittest_directory(base32_ut)

