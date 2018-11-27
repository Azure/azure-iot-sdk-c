cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(build-example C)

include_directories(/usr/local/include/azureiot/)

# uses one of the other samples to demonstrate building (this cmake file is independent of the others in this tree)
add_executable(build-example ../iothub_client_device_twin_and_methods_sample/iothub_client_device_twin_and_methods_sample.c)

target_link_libraries(build-example
        iothub_client
        iothub_client_mqtt_transport
        aziotsharedutil
        pthread
        curl
        ssl
        crypto
        m
        umqtt
        # see issue https://github.com/Azure/azure-iot-sdk-c/issues/550
		# prov_auth_client # causes conflicts on source-built sdk, oct 2018
		# hsm_security_client # causes conflicts on source-built sdk, oct 2018
        uhttp
        uuid
        parson
        )
