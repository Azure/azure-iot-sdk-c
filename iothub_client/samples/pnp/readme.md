---
page_type: sample
description: "A set of C samples that show how a device that uses the IoT Plug and Play conventions interacts with either IoT Hub or IoT Central."
languages:
- c
products:
- azure-iot-hub
- azure-iot-central
- azure-iot-pnp
urlFragment: azure-iot-pnp-device-samples-for-c
---

# IoT Plug And Play device samples

[![Documentation](../../../doc/media/docs-link-buttons/azure-documentation.svg)](https://docs.microsoft.com/azure/iot-develop/)

These samples demonstrate how a device that follows the [IoT Plug and Play conventions](https://docs.microsoft.com/azure/iot-pnp/concepts-convention) interacts with IoT Hub or IoT Central, to:

- Send telemetry.
- Update device twin properties.
- Respond to command invocation.

The samples demonstrate two scenarios:

- An IoT Plug and Play device that implements the [Thermostat](https://devicemodels.azure.com/dtmi/com/example/thermostat-1.json) model. This model has a single interface that defines telemetry, read-only and read-write properties, and commands.
- An IoT Plug and Play device that implements the [Temperature controller](https://devicemodels.azure.com/dtmi/com/example/temperaturecontroller-1.json) model. This model uses multiple components:
  - The top-level interface defines telemetry, read-only property and commands.
  - The model includes two [Thermostat](https://devicemodels.azure.com/dtmi/com/example/thermostat-1.json) components, and a [device information](https://devicemodels.azure.com/dtmi/azure/devicemanagement/deviceinformation-1.json) component.

## Quickstarts and tutorials

To learn more about how to configure and run the Thermostat device sample with IoT Hub, see [Quickstart: Connect a sample IoT Plug and Play device application running on Linux or Windows to IoT Hub](https://docs.microsoft.com/azure/iot-pnp/quickstart-connect-device?pivots=programming-language-c).

To learn more about how to configure and run the Temperature Controller device sample with:

- IoT Hub, see [Tutorial: Connect an IoT Plug and Play multiple component device application running on Linux or Windows to IoT Hub](https://docs.microsoft.com/azure/iot-pnp/tutorial-multiple-components?pivots=programming-language-c)
- IoT Central, see [Tutorial: Create and connect a client application to your Azure IoT Central application](https://docs.microsoft.com/azure/iot-central/core/tutorial-connect-device?pivots=programming-language-c)

## Directory structure

The directory contains the following samples:

* [pnp_simple_thermostat](./pnp_simple_thermostat) A simple thermostat that implements the model [dtmi:com:example:Thermostat;1](https://devicemodels.azure.com/dtmi/com/example/thermostat-1.json).  This sample is considered simple because it only implements one component, the thermostat itself.  **You should begin with this sample.**

* [pnp_temperature_controller](./pnp_temperature_controller) A temperature controller that implements the model [dtmi:com:example:TemperatureController;1](https://devicemodels.azure.com/dtmi/com/example/temperaturecontroller-1.json).  This is considerably more complex than the [pnp_simple_thermostat](./pnp_simple_thermostat) and demonstrates the use of subcomponents.  **You should move onto this sample only after fully understanding pnp_simple_thermostat.**

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
 
