cmake_minimum_required (VERSION 3.5)
project(myapp_project)

# The demonstration uses C99 but it could just as easily be a C++ application
set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")

# Assume we will use the built in trusted certificates. 
# Many embedded devices will need this.
option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" ON)

set(iothub_c_files
    iothub_cross_compile_simple_sample.c
)

# Conditionally use the SDK trusted certs in the samples (is set to true in cmake toolchain file)
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories($ENV{WORK_ROOT}/azure-iot-sdk-c/certs)
    set(iothub_c_files 
        ${iothub_c_files} 
        $ENV{WORK_ROOT}/azure-iot-sdk-c/certs/certs.c)
endif()

# Set up the include and library paths
include_directories(${CMAKE_INSTALL_PREFIX}/include/)
include_directories(${CMAKE_INSTALL_PREFIX}/include/azureiot)
link_directories(/usr/local/lib)
link_directories($ENV{TOOLCHAIN_PREFIX}/lib)

add_executable(myapp ${iothub_c_files})

# Redundant in this case but shows how to rename your output executable
set_target_properties(myapp PROPERTIES OUTPUT_NAME "myapp")

# If OpenSSL::SSL OR OpenSSL::Crypto are not set then you need to run
# the find package for openssl
if (NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto OR NOT ${OPENSSL_INCLUDE_DIR})
    find_package(OpenSSL REQUIRED)
endif()
include_directories(${OPENSSL_INCLUDE_DIR})

# List the libraries required by the link step
target_link_libraries(myapp iothub_client prov_device_client iothub_client_mqtt_transport prov_auth_client umqtt hsm_security_client utpm dl aziotsharedutil parson pthread curl OpenSSL::SSL OpenSSL::Crypto m )
