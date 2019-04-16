#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.
if(${use_installed_dependencies})

    message("azure_iot_sdksConfig.cmake: Line 4: ${CMAKE_CURRENT_LIST_DIR}")

    include(CMakeFindDependencyMacro)
    find_dependency(unofficial-parson)
    find_dependency(uamqp)
    find_dependency(umqtt)
    find_dependency(azure_c_shared_utility)
    find_library(UHTTP_LIBRARY uhttp)
    add_library(uhttp STATIC IMPORTED)
    set_target_properties(uhttp PROPERTIES IMPORTED_LOCATION ${UHTTP_LIBRARY})
endif()

include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_sdksTargets.cmake")

get_target_property(IOTHUB_CLIENT_INCLUDES iothub_client INTERFACE_INCLUDE_DIRECTORIES)

set(IOTHUB_CLIENT_INCLUDES ${IOTHUB_CLIENT_INCLUDES} CACHE INTERNAL "")

include("${CMAKE_CURRENT_LIST_DIR}/azure_iot_sdksFunctions.cmake")