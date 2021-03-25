#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's nothing here, except redirections to
#individual protocol samples

usePermissiveRulesForSdkSamplesAndTests()

add_subdirectory(iothub_client_device_twin_and_methods_sample)
add_subdirectory(iothub_convenience_sample)
add_subdirectory(iothub_ll_c2d_sample)
add_subdirectory(iothub_ll_client_x509_sample)
add_subdirectory(iothub_ll_telemetry_sample)
add_subdirectory(iotedge_downstream_device_sample)


if(${use_mqtt})
    add_subdirectory(pnp)
    if(LINUX)
        add_subdirectory(iothub_client_sample_mqtt_dm)
    endif()
    if(${use_edge_modules})
        add_subdirectory(iothub_client_sample_module_sender)
        add_subdirectory(iothub_client_sample_module_filter)
        add_subdirectory(iothub_client_sample_module_method_invoke)
    endif()
endif()

if(${use_amqp})
    add_subdirectory(iothub_client_sample_amqp_shared_methods)
    add_subdirectory(iothub_ll_client_sample_amqp_shared)
endif()

if(${use_http})
    if(NOT ${dont_use_uploadtoblob})
        add_subdirectory(iothub_client_sample_upload_to_blob)
        add_subdirectory(iothub_client_sample_upload_to_blob_mb)
    endif()
endif()

if(${use_amqp} OR ${use_http})
    add_subdirectory(iothub_ll_client_shared_sample)
endif()
