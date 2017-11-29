#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's noithig here, except redirections to 
#individual protocol samples

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "Provision_Samples")
endfunction()

add_sample_directory(iothub_client_sample_hsm)

if(${use_mqtt} AND ${use_amqp})
    add_sample_directory(prov_dev_client_ll_sample)
    add_sample_directory(prov_dev_client_sample)
endif()
