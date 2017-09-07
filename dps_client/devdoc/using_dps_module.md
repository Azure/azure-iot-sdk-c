# DPS Client SDK

The DPS Client SDK enables automatic provisioning of a device using an HSM (Hardware Security Module) against an IoThub.  There are two different authentication mode that DPS supports: x509 or tpm.

## Enabling DPS using sas token

Using sas token authentication DPS ships with code to connect to windows and linux hardware tpm chips.  The following cmake command will enable the tpm` authentication:

```Shell
cmake -Ddps_auth_type=tpm ..
```

## Enabling DPS using sas token with tpm simulator

For development purposes DPS will use a windows tpm simulator.  The following command will enable the sas token authentication and then run the tpm simulator on the windows OS (the Simulator will listen over a socket on ports 2321 and 2322):

```Shell
cmake -Ddps_auth_type=tpm_simulator ..

./azure-iot-sdk-c/dps_client/deps/utpm/tools/tpm_simulator/Simulator.exe
```

## Enabling DPS using DICE hardware

Using x509 authentication DPS ships with a DICE emulator that generates the x509 certificate that the device will use for authentication.  The following cmake command will enable the x509 authentication:

```Shell
cmake -Ddps_auth_type=x509 ..
```

## Adding Enrollments with Azure Portal

To enroll a device in the azure portal you will need to either get the Registration Id and Endorsement Key for TPM devices or the root CA certificate.  Running the provisioning tool will print out the information to be used in the portal:

### TPM Provisioning Tool

```Shell
./azure-iot-sdk-c/dps_client/tools/tpm_device_provision/tpm_device_provision.exe
```

### x509 Provisioning Tool

```Shell
./azure-iot-sdk-c/dps_client/tools/x509_device_provision/x509_device_provision.exe
```

## Using IoTHub Client with DPS

Once the device has been provisioned with DPS the following API will use the hsm authentication method to connect with the IoThub:

```C
IOTHUB_CLIENT_LL_HANDLE handle = IoTHubClient_LL_CreateFromDeviceAuth(iothub_uri, device_id, iothub_transport);
```

## Running DPS samples

```C
// Run the Sample
cmake/dps_client/samples/dps_client_sample/dps_client_sample
```
