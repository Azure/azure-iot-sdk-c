#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "Provisioning_Service_Client_Samples")
endfunction()

add_sample_directory(prov_sc_individual_enrollment_sample)
add_sample_directory(prov_sc_enrollment_group_sample)
add_sample_directory(prov_sc_bulk_operation_sample)
add_sample_directory(prov_sc_query_sample)
