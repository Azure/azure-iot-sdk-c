# Common utilities for PnP Sample

This directory contains sample code that your application should be able to take with little or no modification to accelerate implementing a PnP device application.

Probably the easiest way to understand how they work is to review the [pnp_temperature_controller](../pnp_temperature_controller) sample, which uses them extensively.

For reference the files are:

* `pnp_device_client` header and .c file implement a function to help create a `IOTHUB_DEVICE_CLIENT_HANDLE`.  The `IOTHUB_DEVICE_CLIENT_HANDLE` is an existing IoTHub Device SDK that can be used for device <=> IoTHub communication.  (There are many samples demonstrating its non-PnP uses in the [samples parent directory](../..).)

    Creating a `IOTHUB_DEVICE_CLIENT_HANDLE` that works with PnP requires a few additional steps, most importantly defining the device's PnP ModelId, which this code does.

    The Azure IoTHub SDK defines additional types of handles - namely `IOTHUB_DEVICE_CLIENT_LL_HANDLE` (for a device using the \_LL\_ lower layer) and for modules the analogous `IOTHUB_MODULE_CLIENT_HANDLE` and `IOTHUB_MODULE_CLIENT_LL_HANDLE`.  The code in this file can easily be modified to use a different handle flavor for your code.

* `pnp_protocol` header and .c file implement functions to help with serializing and de-serializing the PnP convention.  As an example of their usefulness, PnP properties are sent between the device and IoTHub using a specific JSON convention over the device twin.  Functions in this header perform some of the tedious parsing and JSON string generation that your PnP application would need to do.

    The functions are agnostic to the underlying transport handle used.  If you use `IOTHUB_DEVICE_CLIENT_LL_HANDLE`, `IOTHUB_MODULE_CLIENT_HANDLE` or `IOTHUB_MODULE_CLIENT_LL_HANDLE` instead of the sample's `IOTHUB_DEVICE_CLIENT_HANDLE`, the `pnp_protocol` logic does not need to change.
