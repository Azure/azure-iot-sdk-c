# Digital Twin C Client samples directory

This sample in this directory is almost identical to [digitaltwin_sample_device](../digitaltwin_sample_device), including using the identical Digital Twin interfaces.  The main differences are:

* This sample can connect using Device Provisioning Service (DPS), including to IoT Central.  The other sample [digitaltwin_sample_device](../digital_sample_device) currently cannot.
* This sample is designed to use the \_LL\_ layer, appropriate for single threaded applications and devices.

To connect to IoT Central:  

* Open [digitaltwin_sample_ll_device.c](./digitaltwin_sample_ll_device.c). **Specify parameters requested by the commented TODO's for your configuration.**
* Run `digitaltwin_sample_ll_device InitialRegistrationWithIoTCentral`.  When running `digitaltwin_sample_ll_device` with any other command parameter, the application will treat this as an IoTHub connection string.  `InitialRegistrationWithIoTCentral` forces the app to use provisioning.

To use Device Provisioning Service without IoT Central:

* If you provisioned using IoT Central, the DPS instance IoT Central uses will automatically be whitelisted correctly.

To connect to IoTHub directly, without provisioning or IoT Central involvement:

* Compile and run the application and run `digitaltwin_sample_ll_device`, passing your device's IoTHub connection string as the only paramater.

Other than the ability to connect to IoT Central, the  \_LL\_ samples are otherwise almost identical to the ../digitaltwin_sample directory.  The other large difference is that they use the \_LL\_ layer of Digital Twin instead of the convenience layer.  See [here](../../doc/threading_notes.md) for more details about the differences.
