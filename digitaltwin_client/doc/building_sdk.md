# Building the Digital Twin Device SDK

## Initial development environment setup
To get setup to build applications that enable the Digital Twin for C:
* Clone and perform initial cmake on the GitHub repo containing the Digital Twin public preview SDK.


```
 git clone -b public-preview --recursive https://github.com/Azure/azure-iot-sdk-c
 cd azure-iot-sdk-c
 mkdir cmake
 cd cmake
 cmake .. -Duse_prov_client=ON -Dhsm_type_symm_key:BOOL=ON
```

* You may need to reference the instructions for [compiling the C Client SDK](../../iothub_client/readme.md#compile) for additional instructions, especially if you are new to the C SDK.  Those instructions provide more details about installing prerequisites and advanced cmake options.

* **If you are targeting a very resource limited device:** Be aware the cmake flags above for `-Duse_prov_client=ON -Dhsm_type_symm_key:BOOL=ON` bring in the device provisioning client.  Provisioning is needed for connecting to IoT Central in particular.  If your device connects directly to IoTHub and you want to save some resources, you can remove these flags.

Because the Digital Twin client builds using the [IoTHub Device Client](../../iothub_client), optimizing the build for the IoTHub Device Client can further reduce your resource usage.  More information is available [here](../../doc/run_c_sdk_on_constrained_device.md).  Cmake flags set from this will apply to the Digital Twin SDK as well.
