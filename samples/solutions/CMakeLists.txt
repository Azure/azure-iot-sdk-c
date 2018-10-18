#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for solution samples.

usePermissiveRulesForSdkSamplesAndTests()

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "IoTHub_Samples_Solutions")
endfunction()

if(${use_mqtt})
    add_sample_directory(remote_monitoring_client)
endif()