# Configuration Instructions

Note: These configuration steps are part of a greater set of instructions found [here](../). 

***Please note, the device must use SAS authentication for this sample.***

## Model ID

### Get device Model ID

* If using Azure IoT Hub, locate the Model ID of your device.   
* If using Azure IoT Central, locate the Model ID of the template used for your device.
* Copy this ID.
  
  The Model ID will be of a form similar to `dtmi:com:example:SampleDevice;1`. 

 

### Update Model ID in source file
* From the azure-iot-sdk-c repository root:

  **Windows:**  
  ```
  cd .\digitaltwin_client\samples\digitaltwin_sample_ll_device
  ```
  **Linux:**
  ```
  cd digitaltwin_client/samples/digitaltwin_sample_ll_device
  ```

* Open `digitaltwin_sample_ll_device.c` and replace the value for `DIGITALTWIN_SAMPLE_MODEL_ID` with the Model ID. 
  
  Example:
  ```
  #define DIGITALTWIN_SAMPLE_MODEL_ID "dtmi:com:example:SampleDevice;1"
  ```
* Save the file. 

## Symmetric key configuration

The configuration file indicates how the DPS connection is to be established.  For this sample, we are using a DPS_SymmetricKey.

* From the `digitaltwin_sample_ll_device\sample_config` directory, open `dpsSymmKey.json`.  

### Get and update configuration values

There are three `[TODO]`s in this file that need to be replaced: IDScope, deviceId, and deviceKey.

* If using Azure IoT Hub, you will find this information via your Azure IoT Hub Device Provisioning Service.

* If using Azure IoT Central, you will find this information via that platform.

* Copy the values found for IDScope, deviceId, and deviceKey, and replace the corresponding `[TODO]`s in the json.  

* Save the file.

