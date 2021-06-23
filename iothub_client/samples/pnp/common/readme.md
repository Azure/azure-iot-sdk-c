# Common utilities for PnP Sample

This directory contains common code used by the samples in the [parent](..) directory.

## Deprecation Notices

Samples in an earlier version of the SDK used the files listed below.  **These files are deprecated.**  You should move to replacements as soon as practical.  They are left as part in place so as not to break existing users.

* `pnp_protocol.*` Before Plug and Play support was officially added to the device client,  `pnp_protocol.*` had functions intended to help abstract application developers from tedious elements of Plug and Play device authoring.  
  Examples of the official API include all functions in `iothub_client_properties.h` as well as functions such as `IoTHubDeviceClient_LL_SendTelemetryAsync`, `IoTHubDeviceClient_LL_SubscribeToCommands`, `IoTHubDeviceClient_LL_SendPropertiesAsync`, `IoTHubDeviceClient_LL_GetPropertiesAsync`, and `IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync`.
  
  Applications instead should use the official Plug and Play API because:
  * The API has been more thoroughly reviewed than the sample helper and more intuitive to use.  
  * The API, unlike the samples but like all product code in the IoT Device SDK, receives fully automated testing on every checkin.
  * The API will also receive support for any future Plug and Play innovations and bug fixes.  `pnp_protocol.*` will not.

  The samples in the [parent](..) directory use the official API.

* `pnp_device_client_ll.*` Creating a device client handle has been moved into the sample implementations themselves.
