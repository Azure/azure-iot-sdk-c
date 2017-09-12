#Copyright (c) Toradex AG. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists for tools.

function(add_tool_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "DPS_Tools")
endfunction()

if(${dps_hsm_x509})
    add_tool_directory(dice_device_enrollment)
else()
    add_tool_directory(tpm_device_provision)
endif()