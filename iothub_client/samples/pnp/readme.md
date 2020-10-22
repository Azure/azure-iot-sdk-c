# Samples to demonstrate Azure IoT Plug and Play

The samples in this directory demonstrate how to implement an Azure IoT Plug and Play device.  Azure IoT Plug and Play is documented [here](aka.ms/iotpnp).  The samples assume basic familiarity with PnP concepts, though not in depth knowledge of the PnP "convention".  The "convention" is a set of rules for serializing and de-serialing data that uses IoTHub primitives for transport which the samples themselves implement.

## Directory structure

The directory contains the following samples:

* [pnp_simple_thermostat](./pnp_simple_thermostat) A simple thermostat that implements the model [dtmi:com:example:Thermostat;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).  This sample is considered simple because it only implements one component, the thermostat itself.  **You should begin with this sample.**

* [pnp_temperature_controller](./pnp_temperature_controller) A temperature controller that implements the model [dtmi:com:example:TemperatureController;1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json).  This is considerably more complex than the [pnp_simple_thermostat](./pnp_simple_thermostat) and demonstrates the use of subcomponents.  **You should move onto this sample only after fully understanding pnp_simple_thermostat.**

* [common](./common) This directory contains functions for serializing and de-serializing data for PnP and for creating the `IOTHUB_DEVICE_CLIENT_HANDLE` that acts as the transport.  `pnp_temperature_controller` makes extensive use of these functions and demonstrates their use.  **The files in [common](./common) are written generically such that your PnP device application should be able to use them with little or no modification, speeding up your development.**

## Configuring the samples

Both samples use environment variables to retrieve configuration.  

* If you are using a connection string to authenticate:
  * set IOTHUB_DEVICE_SECURITY_TYPE="connectionString"
  * set IOTHUB_DEVICE_CONNECTION_STRING="\<connection string of your device\>"

* If you are using a DPS enrollment group to authenticate:
  * set IOTHUB_DEVICE_SECURITY_TYPE="DPS"
  * set IOTHUB_DEVICE_DPS_ID_SCOPE="\<ID Scope of DPS instance\>"
  * set IOTHUB_DEVICE_DPS_DEVICE_ID="\<Device's ID\>"
  * set IOTHUB_DEVICE_DPS_DEVICE_KEY="\<Device's security key \>"
  * *OPTIONAL*, if you do not wish to use the default endpoint "global.azure-devices-provisioning.net"
    * set IOTHUB_DEVICE_DPS_ENDPOINT="\<DPS endpoint\>"

* If you are running on a device that does not have environment variables, hardcode the values in the .c file itself.

## Enabling Device Provisioning Service client (DPS)

To enable DPS with symmetric keys (which is what this sample uses when DPS is configured), use the cmake flags `-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF `

If you are building connection string only authentication, these extra cmake flags are not required.

## Caveats

* Azure IoT Plug and Play is only supported for MQTT and MQTT over WebSockets for the Azure IoT C Device SDK.  Modifying these samples to use AMQP, AMQP over WebSockets, or HTTP protocols **will not work**.

* When the thermostat receives a desired temperature, it immediately makes that the actual temperature to keep the simulation code easier to follow.  In a real thermostat there would be delay between the desired temperature being set and the room reaching that state.

* The command `getMaxMinReport` allows the application to specify statistics of the temperature since a given date.  To keep the sample simple, we ignore this field and instead return statistics from the entire lifecycle of the executable.

* The temperature controller implements a command named `reboot` which takes a request payload indicating the delay in seconds.  The sample will log the value requested but will not take any further action.
