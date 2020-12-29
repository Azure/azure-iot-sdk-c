# Provisioning Device Client SDK

The Provisioning Device SDK enables automatic provisioning of a device using an HSM (Hardware Security Module) against an IoThub.  There are three different authentication mode that the client supports: x509, TPM or Symmetric Keys.

## Enabling Provisioning Device Client

To use the Provisioning Device client code to connect to windows or linux HSM requires a switch to be sent during cmake initialization.  The following cmake command will enable provisioning:

```Shell
cmake -Duse_prov_client:BOOL=ON ..
```

For Development scenarios, the provisioning client relies on generated test x509 certificates with a pre-generated key within [hsm_client_riot.c](https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/adapters/hsm_client_riot.c)

For Production scenarios, you need to add the USE_CUSTOM_HSM cmake flag with the full path to your custom hsm lib (see custom hsm example below).

## Enabling Provisioning Device Client simulator

For development purposes the Provisioning Device Client uses simulators to mock hardware chips functionality:

### TPM Simulator

The SDK will ship with a windows tpm simulator binary.  The following cmake command will enable the sas token authentication and then you will need to run the tpm simulator on the windows OS (the Simulator will listen over a socket on ports 2321 and 2322).

```Shell
cmake -Duse_prov_client:BOOL=ON -Duse_tpm_simulator:BOOL=ON ..

./azure-iot-sdk-c/provisioning_client/deps/utpm/tools/tpm_simulator/Simulator.exe
```

### TPM Resource Manager

When running on Linux with a hardware TPM (or Hyper-V with virtualized TPMv2) a TPM resource manager should be used to broker access to the hardware and allow multi-threaded operation.

__Important__: Multi-threaded or multi-process TPMv2 usage is supported only when using a TPM resource manager.

See the following resources as well as the tpm2-abrmd manpage for details on how to properly secure the resource manager and access to the hardware TPM: https://tpm2-software.github.io/, https://github.com/tpm2-software/

To install the TPM2 software stack and service on Ubuntu systems:

```
sudo apt install tpm2-abrmd libtss2-tcti-tabrmd-dev
```

### DICE Simulator

For x509 the Provisioning Device Client enables a DICE hardware simulator that emulators the DICE hardware operations.

## Adding Enrollments with Azure Portal

To enroll a device in the azure portal you will need to either get the Registration Id and Endorsement Key for TPM devices, the root CA certificate for x509 device or the Symmetric Key.  Running the provisioning tool will print out the information to be used in the portal:

### TPM Provisioning Tool

```Shell
./[cmake dir]/provisioning_client/tools/tpm_device_provision/tpm_device_provision.exe
```

### x509 Provisioning Tool

```Shell
./[cmake dir]/provisioning_client/tools/dice_device_provision/dice_device_provision.exe
```

### Symmetric Key

- For Symmetric Key the key value can be retrieve upon the creation of the device registration or you can use the symm_key_provisioning tool to create a individual or group symmetric key.

```Shell
./[cmake dir]/provisioning_client/tools/symm_key_provision/symm_key_provision.exe
```

### Provisioning Samples

There are two provisioning samples in the SDK and two HSM samples.  The provisioning samples, `prov_dev_client_ll_sample` and `prov_dev_client_sample` show how to use the provisioning client to connect to DPS and recieve your IoTHub credentials.  The HSM samples include `iothub_client_sample_hsm`, which shows the us of the IoThub device client on an previously provisioned device and `custom_hsm_example`, which shows how to create a sample hsm to be used along side the provisioning client.

The `prov_dev_client_ll_sample` version uses a non-threaded version of the api and the `prov_dev_client_sample` uses the threaded api version.  Before compiling the samples you must make modifications to the c files to get provisioning to work correctly.

- Add your id_scope to the variable:

```C
static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = "[ID Scope]";
```

- You will also need to change the `SECURE_DEVICE_TYPE` to the HSM type that you are using:

```C
SECURE_DEVICE_TYPE hsm_type;
hsm_type = SECURE_DEVICE_TYPE_TPM;
// or
hsm_type = SECURE_DEVICE_TYPE_X509;
// or
hsm_type = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;
```

- If you are using symmetric key authentication you will need to update the following variables in the provided sample.

```C
prov_dev_set_symmetric_key_info("<symm_registration_id>", "<symmetric_Key>");
```

Once these changes are made you can compile and run the sample that you have chosen.

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

or if you are using the lower layer interface

```C
// Run the Sample
./azure-iot-sdk-c/cmake/provisioning_client/samples/prov_dev_client_ll_sample/prov_dev_client_ll_sample
```
