# Samples for Azure IoT Provisioning SDK

## List of samples

- custom_hsm_examples

  - This example shows how to create a custom HSM to retrieve the secruity information needed for the provisioning SDK to communication the the DPS Service.

- iothub_client_sample_hsm

  - This sample show how to use the IoT hub Device Client with the device authentication system without using provisioning.

- prov_dev_client_ll_sample

  - This sample uses the non-threaded SDK to connect the provisioning client and get the IoThub information and then connects to the iothub.  This example would be for devices that require greater control over communications by calling DoWork to control the message pump.

- prov_dev_client_sample

  - This sample uses the convenience layer SDK to connect the provisioning client and get the IoThub information.  This example would be used for devices that don't want to deal with calling DoWork for communication

### Running Samples

You don't need any special hardware to test the samples.  After you create your [Device Provisioning hub](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision), you can just follow the [provisioning SDK documentation](https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/devdoc/using_provisioning_client.md).
