# Samples for the Azure IoT device SDK for C

This folder contains simple samples showing how to use the various features of the Microsoft Azure IoT Hub service from a device running C code.

## Note
* When using low level samples (iothub_ll_*), the IoTHubDeviceClient_LL_DoWork function must be called regularly (eg. every 100 milliseconds) for the IoT device client to work properly.

## List of samples

* Simple send and receive messages:
  * **iothub_ll_telemetry_sample**: sends messages from a single device
  * **iotedge_downstream_device_sample**: sends messages from a single device to an IoT Edge device

* Multiplexing send and receive of several devices over a single connection (useful in Gateway scenarios where multiplexing might be needed):
  * **iothub_client_sample_amqp_shared**: send and receive messages from 2 devices over a single AMQP connection
  * **iothub_ll_client_sample_amqp_shared**: send and receive messages from 2 devices over a single AMQP connection using the lower level API set of the SDK
  * **iothub_client_sample_amqp_websockets_shared**: send and receive messages from 2 devices over a single AMQP over Websockets connection
  * **iothub_client_sample_http_shared**: send and receive messages from 2 devices over a single HTTP connection (multiplexing) (useful in Gateway scenarios)

* Device services samples (Device Twins, Methods, and Device Management):
  * **iothub_client_device_twin_and_methods_sample**: Implements a simple Cloud to Device Direct Method and Device Twin sample
  * **iothub_client_sample_mqtt_dm**: Shows the implementation of a firmware update of a device (Raspberry Pi 3)

* Uploading blob to Azure:
  * **iothub_client_sample_upload_to_blob**: Uploads a blob to Azure through IoT Hub

## How to compile and run the samples

Prior to running the samples, you will need to have an [instance of Azure IoT Hub][lnk-setup-iot-hub]  available and a [device Identity created][lnk-manage-iot-hub] in the hub.

It is recommended to leverage the library packages when available to run the samples, but sometimes you will need to compile the SDK for/on your device in order to be able to run the samples.

[This document][devbox-setup] describes in detail how to prepare your development environment as well as how to run the samples on Linux, Windows or other platforms.

[devbox-setup]: ../../doc/devbox_setup.md
[lnk-setup-iot-hub]: https://aka.ms/howtocreateazureiothub
[lnk-manage-iot-hub]: https://aka.ms/manageiothub
