# Digital Twin C Client samples directory

This directory contains samples demonstrating creating and using Digital Twin interfaces.  

## Directory contents

### Executable Source Code
* [digitaltwin\_sample\_device](./digitaltwin_sample_device) - This executable interacts with the Digital Twin interfaces of the neighboring libraries, demonstrating creation of interfaces and basic operation when using IoT Hub.
* [digitaltwin\_sample\_ll_device](./digitaltwin_sample_ll_device) - This executable interacts with the Digital Twin interfaces of the neighboring libraries, demonstrating creation of interfaces and basic operation.  It uses the lower level (LL) layer and enables the Device Provisioning Service (DPS) to connect to IoT Central or IoT Hub. It is appropriate for devices with limited resources or single threaded applications and devices.

### Libraries
* [digitaltwin\_sample\_device_info](./digitaltwin_sample_device_info) - Library that implements a sample device info Digital Twin interface of helper functions.  This interface reports information about the device - such as OS version, amount of storage, etc.
* [digitaltwin\_sample\_environmental_sensor](./digitaltwin_sample_environmental_sensor) - Library that implements a sample environmental sensor Digital Twin interface of helper functions.  This interface demonstrates all concepts of implementing a Digital Twin model: commands (synchronous and asynchronous), properties, and telemetry.

## Getting started

### digitaltwin\_sample\_device

* Build the Digital Twin Client SDK, if not already completed.  (See [here](../doc/building_sdk.md) for further information.)

  From the azure-iot-sdk-c repository root:
  ```
  cd cmake
  cmake .. -Duse_prov_client=ON -Dhsm_type_symm_key:BOOL=ON
  ```

* Use cmake to build the [digitaltwin\_sample\_device](digitaltwin_sample_device) directory.

  ```
  cmake --build .
  ```
  This will build the `digitaltwin_sample_device` executable along with the aforementioned sample interfaces it uses.

* Get the Connection String of the IoT Hub device you wish to enable for Digital Twin:

    1. Go to your IoT Hub resource via the [azure portal](https://portal.azure.com).
    2. Under Explorers in the left pane, select IoT devices.
    3. Under Device ID, select a device using Sas authentication.  If none, add a new device (with a symmetric key authentication type) and then select it.
    4. Copy the Primary Connection String.

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

* Configure the files.

  Please go [here](./digitaltwin_sample_ll_device) to follow the configuration steps before building the SDK.

* Rebuild the Digital Twin Client SDK.  This step needs to occur after configuring the json in the step above. (See [here](../doc/building_sdk.md) for further information.)

  From the azure-iot-sdk-c repository root:
  ```
  cd cmake
  cmake .. -Duse_prov_client=ON -Dhsm_type_symm_key:BOOL=ON
  ```

* Use cmake to build the [digitaltwin\_sample\_ll_device](digitaltwin_sample_ll_device) directory.

  ```
  cmake --build .
  ```
  This will build the `digitaltwin_sample_ll_device` executable along with the aforementioned sample interfaces it uses.
  
* Run the sample.

  From the cmake directory:

  **If using Windows:** 
  ```
  cd .\digitaltwin_client\samples\digitaltwin_sample_ll_device\Debug\

  .\digitaltwin_sample_ll_device.exe ..\dpsSymmKey.json
  ```

  **If using Linux:**
  ```
  cd digitaltwin_client/samples/digitaltwin_sample_ll_device/
  
  ./digitaltwin_sample_ll_device dpsSymmKey.json
  ```


## Further Explanation

  The executable you have run does a number of things:

  * It registers the sample Digital Twin interfaces with the service.  Currently `digitaltwin_sample_device` registers the sample device info and the sample environmental sensor Digital Twin Interfaces.

  * It invokes the interface samples' logic to report their read-only properties after the interfaces have been registered.

  * It periodically invokes `digitaltwin_sample_environmental_sensor` logic to send temperature and humidity data on its interface.

  * It listens indefinitely, which lets Digital Twin's worker thread accept commands and read/write property updates for  `digitaltwin_sample_environmental_sensor`.
