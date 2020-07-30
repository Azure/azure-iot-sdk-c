# PnP Simple Thermostat sample

This directory contains a sample thermostat that implements the model [dtmi:com:example:Thermostat;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).  

This sample is relatively straightforward, only implementing telemetry, properties, and commands *without* using sub-components.  Compare this to the more complex [pnp_temperature_controller](../pnp_temperature_controller).

## Configuring the sample

The `pnp_simple_thermostat` uses environment variables to retrieve configuration.  

* Set the environment variable `IOTHUB_DEVICE_CONNECTION_STRING` to a connection string of a device.  

* To run the application on a system that does not have environment variables, you will need to hardcode the values in [pnp_simple_thermostat.c](./pnp_simple_thermostat.c).
