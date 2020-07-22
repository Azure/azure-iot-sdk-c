# Samples to demonstrate Azure IoT Plug and Play

The samples in this directory demonstrate how to implement an Azure IoT Plug and Play device.  Azure IoT Plug and Play is documented [here](aka.ms/iotpnp).  The samples assume basic familiarity with PnP concepts, though not in depth knowledge of the PnP "convention".  The "convention" is a set of rules for serializing and de-serialing data that uses IoTHub primitives for transport which the samples themselves implement.

## Directory structure

The directory contains the following samples:

* [pnp_simple_thermostat](./pnp_simple_thermostat) A simple thermostat that implements the model [dtmi:com:example:Thermostat;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).  This sample is considered simple because it only implements one component, the thermostat itself.  **You should begin with this sample.**

* [pnp_temperature_controller](./pnp_temperature_controller) A temperature controller that implements the model [dtmi:com:example:TemperatureController;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json).  This is considerably more complex than the [pnp_simple_thermostat](./pnp_simple_thermostat) and demonstrates the use of subcomponents.  **You should move onto this sample only after fully understanding pnp_simple_thermostat.**

* [common](./common) This directory contains helpers for serializing and de-serializing data for PnP and for creating the `IOTHUB_DEVICE_CLIENT_HANDLE` that acts as the transport.  `pnp_temperature_controller` makes extensive use of these helpers and demonstrates their use.  **The files in [common](./common) are written generically such that your PnP device application can use them with little or no modification, speeding up your devolpment.**

## Caveats

* Azure IoT PnP is only supported for MQTT and MQTT over WebSockets for the Azure IoT C Device SDK.  Modifying these samples to use AMQP, AMQP over WebSockets, or HTTP protocols **will not work**.  The underlying IoTHub core only supports PnP constructs (specifically `OPTION_MODEL_ID`) over MQTT and MQTT over WebSockets.

* When the thermostat receives a desired temperature, it immediately makes that the actual temperature to keep the simulation code easier to follow.  In a real thermostat there would be delay between the desired temperature being set and the room reaching that state.

* The command `getMaxMinReport` allows the application to specify statistics of the temperature since a given date.  To keep the sample simple, we ignore this field and instead return statistics from the entire lifecycle of the executable.

* The temperature controller implements a command named `reboot` which takes a request payload indicating the delay in seconds.  The sample will log the value requested but will not take any action delay seconds later. 
