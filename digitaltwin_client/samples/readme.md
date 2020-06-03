# Digital Twin C Client samples directory

This directory contains samples demonstrating creating and using Digital Twin interfaces.  

## Directory contents

### Directories that build executables
* [digitaltwin\_sample\_device](./digitaltwin_sample_device) - This directory builds the sample executable.  This executable interacts with the Digital Twin interfaces of the neighboring libraries.  It demonstrates creation of interfaces and basic operation when using Azure IoT Hub. It is appropriate for non-resource-constrained devices.
* [digitaltwin\_sample\_ll_device](./digitaltwin_sample_ll_device) - This directory builds the sample_ll executable. This executable interacts with the Digital Twin interfaces of the neighboring libraries.  It demonstrates creation of interfaces and basic operation when using Azure IoT Hub or Azure IoT Central.  It uses the lower level (LL) layer and enables the Azure IoT Hub Device Provisioning Service (DPS) if using Azure IoT Hub.  It is appropriate for devices with limited resources or single threaded applications and devices.

### Directories that build libraries
* [digitaltwin\_sample\_device_info](./digitaltwin_sample_device_info) - This directory builds the sample device info library.  This is a Digital Twin interface of helper functions that report information about the device - such as OS version, amount of storage, etc.
* [digitaltwin\_sample\_environmental_sensor](./digitaltwin_sample_environmental_sensor) - This directory builds the samples environmental sensor library.  This is a Digital Twin interface of helper functions that demonstrates all concepts of implementing a Digital Twin model: commands (synchronous and asynchronous), properties, and telemetry.

## Getting started

* Build the Digital Twin Client SDK, if not already completed. See [here](../doc/building_sdk.md)

### digitaltwin\_sample\_device

* Use CMake to build the [digitaltwin\_sample\_device](digitaltwin_sample_device) directory.

  From the cmake directory:
  ```
  cmake --build .
  ```
  This will build the `digitaltwin_sample_device` executable along with the aforementioned sample interfaces.

* Copy the Connection String of the Azure IoT Hub device you wish to enable for Digital Twin:

* Run the sample.

  From the cmake directory:
  
  **If using Windows:** 

  ```
  cd .\digitaltwin_client\samples\digitaltwin_sample_device\Debug\
  ```
  Run the executable using your connection string, e.g.:
  ```
  .\digitaltwin_sample_device.exe "HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret"
  ```

  **If using Linux:**

  ```
  cd digitaltwin_client/samples/digitaltwin_sample_device
  ```
  Run the executable using your connection string, e.g.:
  ```
  ./digitaltwin_sample_device "HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret"
  ```

  Note: In both OS options, be sure to include quotations around the Connection String.



### digitaltwin\_sample\_ll_device

* Configure the files.

  Please go [here](./digitaltwin_sample_ll_device) to follow the configuration steps before building the SDK.

* Use cmake to build the [digitaltwin\_sample\_ll_device](digitaltwin_sample_ll_device) directory.

  ```
  cmake --build .
  ```
  This will build the `digitaltwin_sample_ll_device` executable along with the aforementioned sample interfaces.
  
* Run the sample.

  From the cmake directory:

  **If using Windows:** 
  ```
  cd .\digitaltwin_client\samples\digitaltwin_sample_ll_device\Debug\

  .\digitaltwin_sample_ll_device.exe ..\..\..\..\..\digitaltwin_client\samples\digitaltwin_sample_ll_device\sample_config\dpsSymmKey.json
  ```

  **If using Linux:**
  ```
  cd digitaltwin_client/samples/digitaltwin_sample_ll_device/
  
  ./digitaltwin_sample_ll_device dpsSymmKey.json ../../../../../digitaltwin_client/samples/digitaltwin_sample_ll_device/sample_config/dpsSymmKey.json
  ```


## Further Explanation

  The executable you have run does a number of things:

  * It registers the sample Digital Twin interfaces with the service.  Currently `digitaltwin_sample_device` registers the sample device info and the sample environmental sensor Digital Twin Interfaces.

  * It invokes the interface samples' logic to report their read-only properties after the interfaces have been registered.

  * It periodically invokes `digitaltwin_sample_environmental_sensor` logic to send temperature and humidity data on its interface.

  * It listens indefinitely, which lets Digital Twin's worker thread accept commands and read/write property updates for  `digitaltwin_sample_environmental_sensor`.
