#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's noithig here, except redirections to
#individual protocol samples

usePermissiveRulesForSdkSamplesAndTests()

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "IoTHub_Samples")
endfunction()

add_sample_directory(iothub_convenience_sample)
add_sample_directory(iothub_ll_c2d_sample)
add_sample_directory(iothub_ll_client_x509_sample)
add_sample_directory(iothub_ll_telemetry_sample)
add_sample_directory(iotedge_downstream_device_sample)

if(${use_http})
    if(NOT ${dont_use_uploadtoblob})
        add_sample_directory(iothub_client_sample_upload_to_blob)
        add_sample_directory(iothub_client_sample_upload_to_blob_mb)
    endif()
    if (${use_edge_modules})
        add_sample_directory(iothub_client_sample_module_method_invoke)
    endif()
endif()

if(${use_mqtt})
    if(LINUX)
        add_sample_directory(iothub_client_sample_mqtt_dm)
    endif()
    if(${use_edge_modules})
        add_sample_directory(iothub_client_sample_module_sender)
        add_sample_directory(iothub_client_sample_module_filter)
    endif()
endif()

if(${use_amqp})
    add_sample_directory(iothub_client_sample_amqp_shared_methods)
endif()

if(${use_amqp} AND ${use_http})
    add_sample_directory(iothub_ll_client_shared_sample)
endif()

if(${use_amqp} AND ${use_mqtt})
    add_sample_directory(iothub_client_device_twin_and_methods_sample)
endif()
