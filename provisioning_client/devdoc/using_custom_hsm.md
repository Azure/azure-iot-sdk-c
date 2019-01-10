# Implementing A Custom HSM

The Provisioning Device Client API enables manufacturers wanting to produce a custom HSM to seamlessly integrate their custom code into the Provisioning Device Client system.

## IoT SDK HSM High Level Design

![][1]

Outlined below are the steps to customize a device HSM for the IoThub SDK Client

## Developing a Custom Repo

- You must develop a library to provide access to the target HSM.  This library will need to be a static library for which the IoThub SDK to link against.  To see an example of how a custom HSM works see the [custom_hsm_example](https://github.com/Azure/azure-iot-sdk-c/tree/master/provisioning_client/samples/custom_hsm_example) project in the sdk.

- The library must implement functions defined in the [hsm_client_data file](https://github.com/Azure/azure-iot-sdk-c/tree/master/provisioning_client/adapters/hsm_client_data.h)

- To expediate the development process there is a custom_hsm_example.c file that gives an example of the interface that will need to be implemented for the custom HSM.

- The following is the list of functions that need to be implemented for either x509, TPM or Symmetric Key custom HSMs.

### hsm_client_x509_init

```c
int hsm_client_x509_init();
```

- Called to initialized the x509 HSM system

### hsm_client_x509_deinit

```c
void hsm_client_x509_deinit();
```

- Method used to deinitialize the x509 HSM system

### hsm_client_tpm_init

```c
int hsm_client_tpm_init();
```

- Called to initialized the tpm HSM system

### hsm_client_tpm_deinit

```c
void hsm_client_tpm_deinit();
```

- Method used to deinitialize the tpm HSM system

### hsm_client_tpm_interface

```c
const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface()
```

- Returns the HSM_CLIENT_TPM_INTERFACE structure containing the function pointers to be called from the provisioning device client.

### hsm_client_x509_interface

```c
const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface()
```

- Returns the HSM_CLIENT_X509_INTERFACE structure containing the function pointers to be called from the provisioning device client.

### custom_hsm_create

```C
HSM_CLIENT_HANDLE custom_hsm_create();
```

- Creates a custom HSM handle used for all the subsequent calls to the custom HSM

### custom_hsm_destroy

```C
void custom_hsm_destroy(HSM_CLIENT_HANDLE handle);
```

- Frees resources allocated in this module

#### HSM X509 API

### custom_hsm_get_certificate

```c
char* custom_hsm_get_certificate(HSM_CLIENT_HANDLE handle);
```

- Retrieves the certificate to be used for x509 communication.  This value is sent unmodified to the tlsio layer as a set_options of OPTION_X509_ECC_CERT.

### custom_hsm_get_alias_key

```c
char* custom_hsm_get_alias_key(HSM_CLIENT_HANDLE handle);
```

- Retrieves the alias key from the x509 certificate.  This value is sent unmodified to the tlsio layer as a set_options of OPTION_X509_ECC_KEY.

### custom_hsm_get_common_name

```c
char* custom_hsm_get_common_name(HSM_CLIENT_HANDLE handle);
```

- Retrieves the common name from the x509 certificate.  Passed to the Device Provisioning Service as a registration Id.

#### HSM TPM API

### custom_hsm_get_endorsement_key

```c
int custom_hsm_get_endorsement_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len);
```

- Retrieves the endorsement key of the TPM in the parameter key and the size in key_len.

### custom_hsm_get_storage_root_key

```c
int custom_hsm_get_storage_root_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len);
```

- Retrieves the storage root key of the TPM in the parameter key and the size in key_len.

### custom_hsm_import_key

```c
int custom_hsm_import_key(HSM_CLIENT_HANDLE handle, const unsigned char* key, size_t key_len);
```

- Imports `key` that has been previously encrypted with the endorsement key and storage root key into the TPM key storage.

### hsm_client_sign_with_identity

```c
int hsm_client_sign_with_identity(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len);
```

- Hashes the parameter `data` with the key previously stored in the TPM and returns  the value in `signed_value`.

#### HSM Symmetric Key API

```c
char* hsm_client_get_symmetric_key(HSM_CLIENT_HANDLE handle);
```

- Returns the symmetric key to be used for authentication

```c
char* hsm_client_get_registration_name(HSM_CLIENT_HANDLE handle);
```

- Returns the registration name to be used for authentication

## Provisioning Device client

- Once your library is successfully compiled and the HSM functionality complete, you can move to the IoThub C-SDK:

  - Supply the custom HSM library path and name in the cmake command:

  ```Shell
    cmake -Duse_prov_client:BOOL=ON -Dhsm_custom_lib=[CUSTOM HSM PATH] [PATH_TO_AZURE_IOT_SDK]
  ```

  The IoThub SDK will link with the custom HSM on the cmake command line

[1]: ./media/client_high_level_diagram.png
