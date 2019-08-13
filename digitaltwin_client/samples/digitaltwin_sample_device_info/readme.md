# Digital Twin C Client Device Information sample

This directory contains a sample implementation of the Digital Twin DeviceInformation interface that targets multiple platforms.    

## Getting started

* [Building the Azure IoT C SDK Digital Twin Device sample](../readme.md).

## Running the sample

* On Linux (device information is queried from kernel): 
  * Run the sample as root: `sudo digitaltwin_sample_device` [connectionString], where connectionString is the connection string of an IoTHub device you wish to Digital Twin enable.  For example, *digitaltwin_sample_device HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret*.
  * Note: the sample has been tested on the following distributions: Ubuntu 16.10, Ubuntu 18.04.

* On other platforms (device information is sample values):
  * Run `digitaltwin_sample_device` [connectionString], where connectionString is the connection string of an IoTHub device you wish to Digital Twin enable.  For example, *digitaltwin_sample_device HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret*.
