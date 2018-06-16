# Samples for the Azure IoT service SDK for C

This folder contains simple samples showing how to use the various features of the Microsoft Azure IoT Hub service from a back-end application running C code.

## List of samples

* Simple send of Cloud to Device messages:
   * **iothub_messaging_sample**: send Cloud to Device message and monitor feedback
   * **iothub_messaging_ll_sample**: send Cloud to Device message and monitor feedback using lower layer API of the SDK

* Device Registry:
   * **iothub_registrymanager_sample**: Shows how to use CRUD operations on the device registry 

* Device services samples (Device Twins, Methods, and Device Management):
   * **iothub_deviceconfiguration_sample**: Shows how to work with device Configurations in a back-end application
   * **iothub_devicemethod_sample**: Shows how to invoke a Cloud to Device direct Method from a back-end application
   * **iothub_devicetwin_sample**: Shows how to work with device Twins in a back-end application

## How to compile and run the samples

Prior to running the samples, you will need to have an [instance of Azure IoT Hub][lnk-setup-iot-hub]  available.

[This documents][devbox-setup] describes in details how to prepare you development environment as well as how to run the samples on Linux, Windows or other platforms.


[devbox-setup]: ../../doc/devbox_setup.md
[lnk-setup-iot-hub]: https://aka.ms/howtocreateazureiothub
