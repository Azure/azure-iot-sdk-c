# Samples to demonstrate Azure IoT Plug and Play

The samples in this directory demonstrate how to implement a Azure IoT Plug and Play device.  Azure IoT Plug and Play is documented [here](aka.ms/iotpnp).  The samples assume basic familairity with PnP concepts, though not in depth knowledge of the PnP "convention".  The "convention" is a set of rules for serializing and deserialing data that uses  IoTHub primitives for transport.  The samples themselves demonstrate and have a sample helper library.

## Directory structure

The directory contains the following samples:

* [pnp_simple_thermostat](./pnp_simple_thermostat) A simple thermostat that implements the model [dtmi:com:example:Thermostat;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).  This sample is considered simple because it only implements one component, the thermostat itself.

There is an additional helper directory, [common](./common), that is used by these samples.  Files in this directory:

* `pnp_device_client_helpers.c|.h` implement code that is used to create the `IOTHUB_DEVICE_CLIENT_HANDLE`.  This is almost identical to creating a standard `IOTHUB_DEVICE_CLIENT_HANDLE` for non-PnP devices; the helper code handles the minor additions required for PnP.

## Caveats

* As with all sample code, care must be taken when building a product based off of it.  Because this code is in a sample directory, it is subject to change over time and does not have the same contract that the [IoTHub SDK](../../inc) has.

* Azure IoT PnP is only support for MQTT and MQTT over WebSockets for the Azure IoT C Device SDK.  Modifying these samples to use AMQP, AMQP over WebSockets, or HTTP protocols **will not work**.  The underlying IoTHub core only supports PnP constructs (specifically `OPTION_MODEL_ID`) over MQTT and MQTT over WebSockets.

* When the thermostat receives a desired temperature, it immediately makes that the actual temperature to keep the simulation code easier to follow.  In a real thermostat there would be delay between the desired temperature being set and the room reaching that state.

* The command `getMaxMinReport` allows the application to specify statistics of the temperature since a given date.  To keep the sample simple, we ignore this field and instead return statistics from the entire lifecycle of the executable.
