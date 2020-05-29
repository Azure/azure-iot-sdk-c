# Digital Twin C Client samples directory

This directory contains samples demonstrating creating and using Digital Twin interfaces.  

## Directory contents

### Executable Source Code
* [digitaltwin\_sample\_device](./digitaltwin_sample_device) - The executable interacts with the Digital Twin interfaces of the neighboring libraries, demonstrating creation of interfaces and basic operation when using Iot Hub.
* [digitaltwin\_sample\_ll_device](./digitaltwin_sample_ll_device) - The executable interacts with the Digital Twin interfaces of the neighboring libraries, demonstrating creation interfaces and basic operation.  It uses the lower level (\_LL\_) layer and enables the Device Provisioning Service (DPS) to connect to Iot Central. It is appropriate for devices with limited resources.

### Libraries
* [digitaltwin\_sample\_device_info](./digitaltwin_sample_device_info) - Library that implements a sample device info Digital Twin interface of helper functions.  This interface reports information about the device - such as OS version, amount of storage, etc.
* [digitaltwin\_sample\_environmental_sensor](./digitaltwin_sample_environmental_sensor) - Library that implements a sample environmental sensor Digital Twin interface of helper functions.  This interface demonstrates all concepts of implementing a Digital Twin model: commands (synchronous and asynchronous), properties, and telemetry.

## Getting started
Build the Digital Twin Client SDK, if not already completed.  Follow instructions [here](../doc/building_sdk.md).

### digitaltwin\_sample\_device

* Use cmake to build the [digitaltwin\_sample\_device](digitaltwin_sample_device) directory.

  From the azure-iot-c-sdk repository root:
  ```
  cd cmake
  cmake --build .
  ```
  This will build the `digitaltwin_sample_device` executable along with the aforementioned sample interfaces it uses.

* Get the Connection String of the Iot Hub device you wish to DigitalTwin enable.  

  This can be found via the [azure portal](https://portal.azure.com):  
    1. Go to your IoT Hub
    2. Under Explorers in the left pane, select IoT Devices
    3. Under Device ID, select a device.  Else, add a new device and then select it.
    4. Copy the Primary Connection String

* Run the sample.

  **If using Windows:** 

  From the cmake directory:
  ```
  cd .\digitaltwin_client\samples\digitaltwin_sample_device\Debug\
  ```
  Run the executable using your connection string, e.g.:
  ```
  .\digitaltwin_sample_device.exe "HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret"
  ```

  **If using Linux:**

  From the cmake directory:
  ```
  cd digitaltwin_client/samples/digitaltwin_sample_device
  ```
  Run the executable using your connection string, e.g.:
  ```
  ./digitaltwin_sample_device "HostName=yourHub;DeviceId=yourDigitalTwinDeviceId;SharedAccessKey=secret"
  ```

  Note: In both OS options, be sure to include quotations around the Connection String.



### digitaltwin\_sample\_ll_device

* Before building the sample, follow additional setup instructions at the [digitaltwin\_sample\_ll_device](./digitaltwin_sample_ll_device/readme.md) directory.

* Follow the same steps as above to build and run the sample, replacing `digitaltwin_sample_device` with `digitaltwin_sample_ll_device`.


## Further Explanation

  The executable you have run does a number of things:

  * It registers the sample Digital Twin interfaces with the service.  Currently `digitaltwin_sample_device` registers the sample device info and the sample environmental sensor Digital Twin Interfaces.

  * It invokes the interface samples' logic to report their read-only properties after the interfaces have been registered.

  * It periodically invokes `digitaltwin_sample_environmental_sensor` logic to send temperature and humidity data on its interface.

  * It listens indefinitely, which lets Digital Twin's worker thread accept commands and read/write property updates for  `digitaltwin_sample_environmental_sensor`.
