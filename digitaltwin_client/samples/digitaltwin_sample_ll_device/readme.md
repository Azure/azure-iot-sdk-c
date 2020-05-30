# Configuration Instructions

Note: These configuration steps are part of a greater set of instructions found [here](../). 

## Model ID

### Get a device Model ID

* If using IoT Hub:
    1. Open the Azure IoT Explorer.  If you do not have it, please follow the download instructions [here](https://docs.microsoft.com/en-us/azure/iot-pnp/howto-install-iot-explorer).
    2. Under Device ID, select a device using Sas authentication.  If none, add a new device (with a symmetric key authentication type) and then select it.
    3. Under Digital Twin in the left pane, expand the menu and select Interface.
    4. Copy the value for "@id" without quotes, e.g. `urn:azureiot:ModelDiscovery:DigitalTwin:1`

* If using IoT Central:
    1. Go to your IoT Central Application resource via the [azure portal](https://portal.azure.com) and navigate to its IoT Central Application URL.
    2. Select Device templates in the left pane.
    3. Select the template used for your device.  If none, add a new template.
    4. Select View identity.
    5. Copy the Identity, e.g. `urn:testazuresphere:AzureSphereSampleDevice_5vg:1`

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
  #define DIGITALTWIN_SAMPLE_MODEL_ID "urn:azureiot:ModelDiscovery:DigitalTwin:1"
  ```

* Save the file. 

## Symmetric key configuration

The configuration file indicates how the DPS connection is to be established.  For this sample, we are using a DPS_SymmetricKey.

* From the `digitaltwin_sample_ll_device` directory, open `dpsSymmKey.json`.

* If using IoT Hub:

  **IDScope**
    1. Go to your DPS resource via the [azure portal](https://portal.azure.com).  For more information on setting up a DPS resource and linking your IoT Hub to it, go [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision.)
    2. On the Overview page, copy the ID Scope value.
    3. Replace the IDScope `[TODO]` in the json.

  **deviceId**
    1. Select Manage enrollments in the left pane and add individual enrollment.
    2. Change Mechanism to Symmetric Key, fill in the blanks, and save.
    3. Select Individual enrollments.
    4. Select the Registration Id. Copy this from the top of the page.
    5. Replace the deviceId `[TODO]` in the json.

  **deviceKey**
    1. Copy the Primary Key.
    2. Replace the deviceKey `[TODO]` in the json. 
    3. Save the file.

* If using IoT Central:

    1. In your IoT Central application, select Devices in the left pane.
    2. Add a device using the template selected earlier.  Select the device.
    3. Locate the Connect option in the upper right corner.  It looks like an 'X'.  Select it.
    4. Copy from Device connection and replace for each `[TODO]` in the json: ID scope, Device ID, and Primary key.
    5. Save the file.


