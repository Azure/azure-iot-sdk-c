#Copyright (c) Toradex AG. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists for tools.

function(add_tool_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "Provision_Tools")
endfunction()

if (NOT ${hsm_type_custom} AND NOT ${run_e2e_tests})
    add_tool_directory(dice_device_enrollment)
    add_tool_directory(tpm_device_provision)
endif()
