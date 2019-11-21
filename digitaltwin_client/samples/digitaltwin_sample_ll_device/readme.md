# Digital Twin C Client samples directory

This sample in this directory is almost identical to [digitaltwin_sample_device](../digitaltwin_sample_device), including using the identical Digital Twin interfaces.  The main differences are:

* This sample can connect using Device Provisioning Service (DPS), including to IoT Central.  The other sample [digitaltwin_sample_device](../digital_sample_device) currently cannot.
* This sample is designed to use the \_LL\_ layer, appropriate for single threaded applications and devices.

To use this sample:
* Modify in `digitaltwin_sample_ll_device.c` the `DIGITALTWIN_SAMPLE_DEVICE_CAPABILITY_MODEL_ID` value to reference the DCM name you have setup, if you are using a different one.

* Create a configuration file, per the instructions below.

## Configuration file setup

The sample takes a single command line argument, which is the name of a JSON file containing configuration.  The JSON file requires a field named "securityType" which indicates how the connection to IoTHub (optionally via DPS) is to be established.

Samples of all types of connection are included in the [./sample_config](./sample_config) directory.  Choose the appropriate file type and fill in the [TODO] sections.  The security types / samples are:  

* **ConnectionString** indicates that the device connection string is specified in the "connectionString" JSON field.  This connects directly to IoTHub, not DPS.  A sample file is [here](./sample_config/connectionString.json).
* **DPS_SymmetricKey** indicates the device is connecting to DPS, using a symmetric key as the authentication mechanism.  A sample file is [here](./sample_config/dpsSymmKey.json).
* **DPS_X509** indicates the device is connecting to DPS, using X509 certificates.  A sample file is [here](./sample_config/dpsX509.json).  This may require additional setup, e.g. using the TPM simulator during initial development.

IoT Central requires DPS based connections.  You will need to setup either **DPS_SymmetricKey** or **DPS_X509** to connect.
