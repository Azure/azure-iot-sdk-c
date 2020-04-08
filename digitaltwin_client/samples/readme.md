# Digital Twin C Client samples directory

This directory contains samples demonstrating creating and using Digital Twin interfaces.  

## Directory contents

* [digitaltwin_sample_device](./digitaltwin_sample_device) - executable that interacts with the other Digital Twin interfaces in this directory.
* [digitaltwin_sample_ll_device](./digitaltwin_sample_ll_device) - executable that interacts with the other Digital Twin interfaces in this directory, using the \_LL\_ layer and enabling Device Provisioning Service (DPS).
* [digitaltwin_sample_device_info](./digitaltwin_sample_device_info) - library that implements a sample device info Digital Twin interface.  This interface reports information about the device - such as OS version, amount of storage, , etc.
* [digitaltwin_sample_environmental_sensor](./digitaltwin_sample_environmental_sensor) library that implements a sample environmental sensor Digital Twin interface.  This interface demonstrates all concepts of the implementing a Digital Twin model: commands (synchronous and asynchronous), properties, and telemetry.

## Getting started

* [Setup to build the Azure IoT C SDK and Digital Twin generally](../doc/building_sdk.md).

* If you wish to use `digitaltwin_sample_ll_device`, see  [digitaltwin_sample_ll_device](./digitaltwin_sample_ll_device/readme.md) for additional instructions on running the \_LL\_ sample.

* Build [digitaltwin_sample_device](digitaltwin_sample_device) directory.  
This will build the `digitaltwin_sample_device` executable along with the sample interfaces that it uses (which are also contained under the ./samples directory.  The libraries implementing the interfaces (`digitaltwin_sample_device_info` and `digitaltwin_sample_environmental_sensor`) implement helper functions that `digitaltwin_sample_device` invokes to perform basic DigitalTwin operations.

* Run `digitaltwin_sample_device` [connectionString], where connectionString is the connection string of an IoTHub device you wish to DigitalTwin enable.  For example, *digitaltwin_sample_device HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret*.

  `digitaltwin_sample_device` is the main program for this sample directory and does a number of things:
  * It registers the sample Digital Twin interfaces with the service.  Currently `digitaltwin_sample_device` registers a sample deviceInfo and sample environmentalSensor Digital Twin Interfaces.
  * It invokes the interface samples' logic to report their read-only properties after the interfaces have been registered.
  * It periodically invokes `digitaltwin_sample_environmental_sensor` logic to send temperature and humidity data on its interface.
  * It listens indefinitely, which lets Digital Twin's worker thread accept commands and read/write property updates for  `digitaltwin_sample_environmental_sensor`.
