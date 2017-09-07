#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's noithig here, except redirections to 
#individual protocol samples

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "DPS_Samples")
endfunction()


if(${use_riot_module})
    #add_sample_directory(iothub_client_sample_riot)
else()
    #add_sample_directory(iothub_client_sample_tpm)
endif()

if(${use_mqtt} AND ${use_amqp})
    add_sample_directory(dps_client_sample)
endif()
add_sample_directory(uhttp_sample)
