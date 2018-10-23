# IoT Hub Registry Manager Sample

## Overview
<a name="Overview"/>

This is a quick tutorial for **iothub_registrymanager_sample**.
This shows how to use CRUD operations on the device registry with using the Azure IoT service SDK for C.

This sample shows a new valid usage of deprecated API [IoTHubRegistryManager_GetDeviceList][IoTHubRegistryManager_GetDeviceList-link], which is not documented. In addition, it has the capability of executing some functions of the retired tool [IoTHub_Explorer][iothub-explorer-link] at a practical speed.
This sample is useful for using the management function of IoT Hub from Lightweight Linux platform including Azure Sphere.

## How to build the sample on Linux or Windows
<a name="Build"/>

See the [document][up-link] of ``How to compile and run the samples`` for ``Samples for the Azure IoT service SDK for C``.

## How to run the sample on Linux or Windows
<a name="Run"/>

1. Copy `iothub_registrymanager_sample` or `iothub_registrymanager_sample.exe` from `cmake/iothub_service_client/samples/iothub_registrymanager_sample/` directory to your working directory after built.
2. Run the execute binary from comamnd prompt with a command parameter and command options.
3. Command and parameter options are followings. Some of them have compatibilty as **IoTHub_Explorer**.

You can use the following help command to get all the **iothub_registrymanager_sample** command and options:

```shell
$ iothub_registrymanager_sample -h

  Usage: iothub_registrymanager_sample [options] <command> [command-options] [command-args]

  Options:

    -V, --version                   output the version number
    -h, --help                      output usage information

  Commands:

    create <device-id>              create a device identity in your IoT hub device registry
    read <device-id>                get a device identity from your IoT hub device registry
    get <device-id>                 get a device identity from your IoT hub device registry
    update <device-id>              update device key[s] in your IoT hub device registry
    delete <device-id>              delete a device identity from your IoT hub device registry
    statistic                       get a IoT Hub statistics
    list                            list the device identities currently in your IoT hub device registry

  Command-options:

    -l, --login <connection-string> connection string to use to authenticate with your IoT Hub instance
    -k1, --key1 <key>               specify the primary key for newly created device
    -k2, --key2 <key>               specify the secondary key for newly created device
```
## Examples
<a name="Examples"/>

```shell
$ iothub_registrymanager_sample create -l "HostName=<my-hub>.azure-devices.net;SharedAccessKeyName=<my-policy>;SharedAccessKey=<my-policy-key>" mydevice -k1 "aaabbbcccdddeeefffggghhhiiijjjkkklllmmmnnnoo" -k2 "111222333444555666777888999000aaabbbcccdddee"
```
## Notes
<a name="Notes"/>

The status of [IoTHubRegistryManager_GetDeviceList][IoTHubRegistryManager_GetDeviceList-link] API for `list` command
is under deprecated. You can disable to use this with uncomment `#define USE_DEPRECATED 1` on source code and rebuild. There is a limitation this API can handle only up to 1000 lists. 

[root-link]: https://github.com/Azure/azure-iot-sdk-c
[source-code-link]: ../../src
[iothub-explorer-link]: https://github.com/Azure/iothub-explorer
[up-link]: .. 
[IoTHubRegistryManager_GetDeviceList-link]:
https://github.com/Azure/azure-iot-sdk-c/blob/master/iothub_service_client/devdoc/requirement_docs/iothubserviceclient_registrymanager_requirements.md#iothubregistrymanager_getdevicelist-deprecated