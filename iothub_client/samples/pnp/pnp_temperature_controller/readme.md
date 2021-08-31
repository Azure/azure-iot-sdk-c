# Azure IoT Plug and Play Temperature Controller sample

[![Documentation](../../../../doc/media/docs-link-buttons/azure-documentation.svg)](https://docs.microsoft.com/azure/iot-develop/)

This directory contains a sample a temperature controller that implements the model [dtmi:com:example:TemperatureController;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json).

The model consists of:

* The `reboot` command, `serialNumber` property, and `workingSet` telemetry that the temperature controller implements on the root interface.

* Two thermostat subcomponents, `thermostat1` and `thermostat2`.  (To get a sense of this sample's greater complexity compared to the simpler one, [pnp_simple_thermostat](../pnp_simple_thermostat) *only* implemented a single thermostat with no subcomponents.  The sample in this directory has two such thermostats in addition to everything else.)

* A `deviceInfo` component that reports properties about the device itself.

Note that the individual components are in separate .c and .h files for easier composition.

* [pnp_thermostat_component.c](./pnp_thermostat_component.c) implements the root temperature controller component.  Because this is the root component, this code also handles routing to the subcomponents.  For instance, if a PnP command is intended for `thermostat1`, code in this file first needs to parse this and route to the appropriate logic.

* [pnp_deviceinfo_component.c](./pnp_deviceinfo_component.c) implements a simple, simulated DeviceInformation component whose DTDL is defined [here](https://repo.azureiotrepository.com/Models/dtmi:azure:DeviceManagement:DeviceInformation;1?api-version=2020-05-01-preview).  This component only does a one-time reporting of the device state on program initiation.

* [pnp_thermostat_component.c](./pnp_thermostat_component.c) implements a thermostat component.  The temperature controller can have multiple components of the same type.  The components `thermostat1` and `thermostat2` both implement `dtmi:com:example:Thermostat;1` in the temperature controller model.

## Configuring the sample

See [../readme.md](../readme.md) for how to configure this sample.
