# Provisioning Device Client SDK

The Provisioning Device SDK enables automatic provisioning of a device using an HSM (Hardware Security Module) against an IoThub.  There are two different authentication mode that the client supports: x509 or TPM.

## Enabling Provisioning Device Client

To use the Provisioning Device client code to connect to windows and linux HSM requires a switch to be sent during cmake initialization.  The following cmake command will enable provisioning:

```Shell
cmake -Duse_prov_client:BOOL=ON ..
```

## Enabling Provisioning Device Client  simulator

For development purposes the Provisioning Device Client will use simulators to mock hardware chips.

### TPM Simulator

The SDK will ship with a windows tpm simulator binary.  The following command will enable the sas token authentication and then run the tpm simulator on the windows OS (the Simulator will listen over a socket on ports 2321 and 2322).

### DICE Simulator

For x509 the Provisioning Device Client enables a DICE hardware simulator that emulators the DICE hardware operations.

```Shell
cmake -Duse_prov_client:BOOL=ON -Duse_tpm_simulator:BOOL=ON ..

./azure-iot-sdk-c/provisioning_client/deps/utpm/tools/tpm_simulator/Simulator.exe
```

## Adding Enrollments with Azure Portal

To enroll a device in the azure portal you will need to either get the Registration Id and Endorsement Key for TPM devices or the root CA certificate.  Running the provisioning tool will print out the information to be used in the portal:

### TPM Provisioning Tool

```Shell
./azure-iot-sdk-c/provisioning_client/tools/tpm_device_provision/tpm_device_provision.exe
```

### x509 Provisioning Tool

```Shell
./azure-iot-sdk-c/provisioning_client/tools/dice_device_provision/dice_device_provision.exe
```

## Using IoTHub Client with Provisioning Device Client

Once the device has been provisioned with the Provisioning Device Client the following API will use the `HSM` authentication method to connect with the IoThub:

```C
IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_CreateFromDeviceAuth(iothub_uri, device_id, iothub_transport);
```

## Running Provisioning Device Client samples

```C
// Run the Sample
./azure-iot-sdk-c/cmake/provisioning_client/samples/prov_dev_client_sample/prov_dev_client_sample
```
