# Multiplexing limitations and alternatives

The Azure IoT C SDK supports multiplexing - the ability to have multiple device identities sharing the same underlying TLS connection.  **There are many limitations in its support, however.  Before using multiplexing you need to carefully understand these.**

## Prefer IoT Edge or Azure IoT C# Device SDK for multiplexing if possible
If you are creating a gateway device, [Azure IoT Edge](http://aka.ms/iotedge) is highly recommend over using this SDK to multiplex leaf devices.  Edge was designed from the ground up for gateway scenarios including full multiplexing support, store and forward of messages, enhanced diagnostics, module support, and more.

If you need multiplexing for a non-gateway scenario, and if your device has the required resources, using the [Azure IoT Device SDK for C#](https://github.com/Azure/azure-iot-sdk-csharp) is highly recommended.  The C# Device SDK has a more complete feature set for multiplexing support.

*The rest of this document will discuss the Azure IoT Device SDK for C (this repo's) multiplexing support.*

## Supported Protocols
Multiplexing is supported on the AMQP, AMQP over WebSockets, and HTTPS protocols.  It is not supported on MQTT or MQTT over WebSockets.

## Limited functionality
When multiplexing over an AMQP, AMQP over WebSockets, or HTTP connection, the SDK supports:
* Sending telemetry (`IoTHubDeviceClient_LL_SendEventAsync` and `IoTHubDeviceClient_SendEventAsync`)
* Receiving incoming Cloud-to-Device messages (via callbacks set in `IoTHubDeviceClient_LL_SetMessageCallback` `IoTHubDeviceClient_SetMessageCallback`)

When multiplexing over an AMQP or AMQP over WebSockets connection (but not HTTP), the SDK also supports
* Receiving incoming device methods (via callbacks set in `IoTHubDeviceClient_LL_SetDeviceMethodCallback` and `IoTHubDeviceClient_SetDeviceMethodCallback`).

The SDK does not support any other features, such as receiving device twins, sending device twin updates, or uploading files to Azure Storage.  Any additional IoT features added to the SDK will *not* support multiplexing.

## Device client support only
Multiplexing only works when using device client handles (`IOTHUB_DEVICE_CLIENT_HANDLE` and `IOTHUB_DEVICE_CLIENT_LL_HANDLE`).  Modules (`IOTHUB_MODULE_CLIENT_HANDLE` and `IOTHUB_MODULE_CLIENT_LL_HANDLE`) are not supported.

## Samples
Samples demonstrating multiplexing are available in the [device client samples folder](../iothub_client/samples/)
* [Sending telemetry and receiving Cloud-to-Device messages](../iothub_client/samples/iothub_ll_client_shared_sample)
* [Receiving device methods](../iothub_client/samples/iothub_client_sample_amqp_shared_methods)