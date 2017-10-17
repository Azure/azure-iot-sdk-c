#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${use_installed_dependencies})
    if(NOT azure_c_shared_utility_FOUND)
        find_package(azure_c_shared_utility REQUIRED CONFIG)
    endif()
    
    if(${use_amqp})
        if(NOT uamqp_FOUND)
            find_package(uamqp REQUIRED CONFIG)
        endif()
    endif()

    if(${use_mqtt})
        if(NOT umqtt_FOUND)
            find_package(umqtt REQUIRED CONFIG)
        endif()
    endif()	

else()
    add_subdirectory(c-utility)

    if(${use_amqp})
        add_subdirectory(uamqp)
    endif()

    if(${use_mqtt})
        add_subdirectory(umqtt)
    endif()

    if(${use_http})
        add_subdirectory(deps/uhttp)
    endif()
endif()
