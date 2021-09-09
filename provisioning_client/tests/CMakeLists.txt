#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.5)

usePermissiveRulesForSdkSamplesAndTests()

add_unittest_directory(prov_transport_http_client_ut)

if(${use_prov_client})
    if (${use_amqp})
        add_unittest_directory(prov_transport_amqp_client_ut)
        add_unittest_directory(prov_transport_amqp_ws_client_ut)
        add_unittest_directory(prov_transport_amqp_common_ut)
        add_unittest_directory(prov_sasl_tpm_ut)
    endif ()

    if (${use_mqtt})
        add_unittest_directory(prov_transport_mqtt_client_ut)
        add_unittest_directory(prov_transport_mqtt_ws_client_ut)
        add_unittest_directory(prov_transport_mqtt_common_ut)
    endif ()
endif ()

add_unittest_directory(prov_device_client_ut)
add_unittest_directory(prov_device_client_ll_ut)
add_unittest_directory(prov_security_factory_ut)

if (${hsm_type_x509})
    add_unittest_directory(hsm_client_riot_ut)
endif ()

if (${hsm_type_sastoken})
    add_unittest_directory(hsm_client_tpm_ut)
endif ()

if (${hsm_type_symm_key})
    add_unittest_directory(hsm_client_key_ut)
endif ()

if(${hsm_type_edge_module})
    add_unittest_directory(hsm_client_http_edge_ut)
endif()

if (${hsm_type_sastoken} AND ${hsm_type_sastoken})
    add_unittest_directory(prov_auth_client_ut)
endif ()

if (LINUX)
    #add_e2etest_directory(prov_symm_key_client_e2e)
    #add_e2etest_directory(prov_tpm_client_e2e)
    add_e2etest_directory(prov_x509_client_e2e)
endif ()

add_unittest_directory(iothub_auth_client_ut)
#add_e2etest_directory(prov_invalidcert_e2e)
