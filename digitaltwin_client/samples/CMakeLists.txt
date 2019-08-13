#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for DigitalTwin samples.

usePermissiveRulesForSdkSamplesAndTests()

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "DigitalTwin_Client_Samples")
endfunction()

set(DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/digitaltwin_sample_environmental_sensor CACHE INTERNAL "this is what needs to be included if using digitaltwin_client lib" FORCE)

add_sample_directory(digitaltwin_sample_device)
if (${use_prov_client})
    add_sample_directory(digitaltwin_sample_ll_device)
endif()
add_sample_directory(digitaltwin_sample_device_info)
add_sample_directory(digitaltwin_sample_environmental_sensor)
add_sample_directory(digitaltwin_sample_model_definition)
