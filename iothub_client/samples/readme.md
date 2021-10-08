# Samples for the Azure IoT device SDK for C

This folder contains simple samples showing how to use the various features of the Microsoft Azure IoT Hub service from a device running C code.

## Note
* When using low level samples (iothub_ll_*), the IoTHubDeviceClient_LL_DoWork function must be called regularly (e.g. every 100 milliseconds) for the IoT device client to work properly.

## List of samples

* Simple send and receive messages:
  * **iothub_ll_telemetry_sample**: Sends messages from a device
  * **iothub_ll_client_x509_sample** Sends messages from a device using X.509 based authentication
  * **iotedge_downstream_device_sample**: Sends messages from a device to an IoT Edge device

* Device Twins, Methods, and Cloud-to-Device messages:
  * **iothub_ll_c2d_sample**: Listens for incoming Cloud to Device messages 
  * **iothub_client_device_twin_and_methods_sample**: Implements a simple Cloud to Device, Direct Method, and Device Twin sample
  * **iothub_convenience_sample**: Implements a simple Cloud to Device, Direct Method, and Device Twin sample using the convenience layer

* IoT Plug and Play
  * **pnp**: Samples demonstrating IoT Plug and Play device functionality

* Running inside an IoT Edge hosted container:
  * **iothub_client_sample_module_filter**: Listens for messages from *iothub_client_sample_module_sender* sample and method calls from *iothub_client_sample_module_method_invoke*
  * **iothub_client_sample_module_sender**: Sends messages to IoT Edge host, which can be routed to *iothub_client_sample_module_filter*
  * **iothub_client_sample_module_method_invoke**: Invokes a method on a module, which can be routed to *iothub_client_sample_module_filter*
  
* Uploading blob to Azure:
  * **iothub_client_sample_upload_to_blob**: Uploads a blob to Azure through IoT Hub
  * **iothub_client_sample_upload_to_blob_mb**: Uploads a blob to Azure through IoT Hub using multiple blocks

* Multiplexing send and receive of several devices over a single connection.  (Please see [documentation](../../doc/multiplexing_limitations.md) about multiplexing limitations and alternatives.)
  * **iothub_ll_client_shared_sample**: Send and receive messages from 2 devices over a single AMQP or HTTP connection
  * **iothub_client_sample_amqp_shared_methods**: Receive device methods from 2 devices over a single AMQP connection

* iOS
  * **ios**: More information about using the SDK on iOS devices

* Deprecated samples
  * **iothub_client_sample_mqtt_dm**: Shows the implementation of a firmware update of a device (Raspberry Pi 3)

## How to compile and run the samples

Prior to running the samples, you will need to have an [instance of Azure IoT Hub][lnk-setup-iot-hub]  available and a [device Identity created][lnk-manage-iot-hub] in the hub.

It is recommended to leverage the library packages when available to run the samples, but sometimes you will need to compile the SDK for/on your device in order to be able to run the samples.

[This document][devbox-setup] describes in detail how to prepare your development environment as well as how to run the samples on Linux, Windows or other platforms.

[devbox-setup]: ../../doc/devbox_setup.md
[lnk-setup-iot-hub]: https://aka.ms/howtocreateazureiothub
[lnk-manage-iot-hub]: https://aka.ms/manageiothub
