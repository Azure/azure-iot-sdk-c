# Samples for the Azure IoT device SDK for C leveraging the serializer library

**Please read and understand serializer limitations and alternatives as described [here](../readme.md) before beginning to use the serializer.**

This folder contains simple samples showing how to use the various features of the Microsoft Azure IoT Hub service leveraging the serializer library from a device running C code.

## List of samples

* Simple send and receive messages:
   * **simplesample_mqtt**: send and receive messages from a single device over an MQTT connection
   * **simplesample_amqp**: send and receive messages from a single device over an AMQP connection
   * **simplesample_http**: send and receive messages from a single device over an HTTP connection
   * **remote_monitoring**: Implements the device code used to connect to an [Azure IoT Suite Remote Monitoring preconfigured solution][remote-monitoring-pcs]
   * **temp_sensor_anomaly**: simple sample for an mbed application that collects temperature information from sensors, sends them to Azure IoT Hub and trigger a buzzer when receiving a command from Azure IoT Hub

* Device services samples (Device Twins, Methods, and Device Management):
   * **devicetwin_simplesample**: Implements the device code to utilize the Azure IoT Hub Device Twins feature

## How to compile and run the samples

Prior to running the samples, you will need to have an [instance of Azure IoT Hub][lnk-setup-iot-hub]  available and a [device Identity created][lnk-manage-iot-hub] in the hub.

It is recommended to leverage the library packages when available to run the samples, but sometimes you will need to compile the SDK for/on your device in order to be able to run the samples.

[This documents][devbox-setup] describes in details how to prepare you development environment as well as how to run the samples on Linux, Windows or other platforms.

[remote-monitoring-pcs]: https://docs.microsoft.com/en-us/azure/iot-suite/iot-suite-remote-monitoring-sample-walkthrough
[devbox-setup]: ../../doc/devbox_setup.md
[lnk-setup-iot-hub]: https://aka.ms/howtocreateazureiothub
[lnk-manage-iot-hub]: https://aka.ms/manageiothub
