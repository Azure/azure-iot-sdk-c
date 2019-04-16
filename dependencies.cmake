#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${use_installed_dependencies})
    if (NOT azure_c_shared_utility_FOUND)
        message("!!!!!! azure_c_shared_utility being searched.")
        find_package(azure_c_shared_utility REQUIRED CONFIG)
    endif ()
    
    if (${use_amqp})
        if (NOT uamqp_FOUND)
            find_package(uamqp REQUIRED CONFIG)
        endif ()
    endif ()

    if (${use_mqtt})
        if (NOT umqtt_FOUND)
            find_package(umqtt REQUIRED CONFIG)
        endif ()
    endif ()

    find_package(unofficial-parson REQUIRED)
    link_libraries(unofficial::parson::parson)

else ()
    add_subdirectory(c-utility)

    if (${use_amqp})
        add_subdirectory(uamqp)
    endif ()

    if (${use_mqtt})
        add_subdirectory(umqtt)
    endif ()

    if (${use_http})
        add_subdirectory(deps/uhttp)
    endif ()
endif()

# The use of aziotsharedutil's INTERFACE_INCLUDE_DIRECTORIES is a more flexible replacement
# for the SHARED_UTIL_INC_FOLDER, which is a single path. It is expected that the 
# SHARED_UTIL_INC_FOLDER should eventually be eliminated as redundant.
get_target_property(AZURE_C_SHARED_UTILITY_INTERFACE_INCLUDE_DIRECTORIES aziotsharedutil INTERFACE_INCLUDE_DIRECTORIES)
if (AZURE_C_SHARED_UTILITY_INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${AZURE_C_SHARED_UTILITY_INTERFACE_INCLUDE_DIRECTORIES})
endif ()
