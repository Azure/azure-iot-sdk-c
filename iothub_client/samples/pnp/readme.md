# Samples to demonstrate Azure IoT Plug and Play

The samples in this directory demonstrate how to implement an Azure IoT Plug and Play device.  Azure IoT Plug and Play is documented [here](aka.ms/iotpnp).  The samples assume basic familiarity with PnP concepts, though not in depth knowledge of the PnP "convention".  The "convention" is a set of rules for serializing and de-serialing data that uses IoTHub primitives for transport which the samples themselves implement.

## Directory structure

The directory contains the following samples:

* [pnp_simple_thermostat](./pnp_simple_thermostat) A simple thermostat that implements the model [dtmi:com:example:Thermostat;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).  This sample is considered simple because it only implements one component, the thermostat itself.

## Caveats

* Azure IoT PnP is only supported for MQTT and MQTT over WebSockets for the Azure IoT C Device SDK.  Modifying these samples to use AMQP, AMQP over WebSockets, or HTTP protocols **will not work**.  The underlying IoTHub core only supports PnP constructs (specifically `OPTION_MODEL_ID`) over MQTT and MQTT over WebSockets.

* When the thermostat receives a desired temperature, it immediately makes that the actual temperature to keep the simulation code easier to follow.  In a real thermostat there would be delay between the desired temperature being set and the room reaching that state.

* The command `getMaxMinReport` allows the application to specify statistics of the temperature since a given date.  To keep the sample simple, we ignore this field and instead return statistics from the entire lifecycle of the executable.
