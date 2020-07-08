# Common helper utilities for PnP Sample

This directory contains sample code that your application should be able to take with little or no modification to speed implementing a PnP device.

* `pnp_device_client_helpers` header and .c file implement a function to help create a `IOTHUB_DEVICE_CLIENT_HANDLE`.  The `IOTHUB_DEVICE_CLIENT_HANDLE` is an existing IoTHub Device SDK that can be used for device <=> IoTHub communication.  (There are many samples demonstrating its non-PnP uses in the [samples parent directory](../..).

    Creating a `IOTHUB_DEVICE_CLIENT_HANDLE` that works with PnP requires a few additional steps, most importantly defining the device's PnP ModelId.  This sample's code performs the required functionality.

    The Azure IoT SDK defines additional types of handles - namely `IOTHUB_DEVICE_CLIENT_LL_HANDLE` (for a device using the \_LL\_ lower layer) and for modules the analogous `IOTHUB_MODULE_CLIENT_HANDLE` and `IOTHUB_MODULE_CLIENT_LL_HANDLE`.  The code in this file can easily be modified to use a different handle flavor for your code.

* `pnp_protocol_helpers` header and .c file implement functions to help with serializing and de-serializing the PnP convention.  For instance, PnP desired properties are sent to the device via an IoTHub device twin.  PnP defines rules about how the service must format properties and components in the properties.  This header has a function to process an IoTHub device twin and transform the raw JSON into the constituent PnP properties.

  The helpers are documented in more detail in [pnp_protocol_helpers.h](./pnp_protocol_helpers).

    The helpers are agnostic to the underlying transport handle used.  If you use `IOTHUB_DEVICE_CLIENT_LL_HANDLE`, `IOTHUB_MODULE_CLIENT_HANDLE` or `IOTHUB_MODULE_CLIENT_LL_HANDLE` instead of the sample's `IOTHUB_DEVICE_CLIENT_HANDLE`, the `pnp_protocol_helpers` logic does not need to change.