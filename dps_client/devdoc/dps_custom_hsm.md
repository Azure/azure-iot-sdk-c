# Implementing Custom HSM

The DPS Client API enables manufacturers wanting to produce a custom HSM to seamlessly integrate their custom code into the DPS system.

## IoT SDK HSM High Level Design

![][1]

Outlined below are the steps to customize a device HSM for the IoThub SDK Client

## Develop Custom Repo

- You must develop a library to provide access to the target HSM.  This library will need to be a static library for the IoThub sdk to consume.
- You library must implement the function defined in the header file for your HSM authentication type [tpm](https://github.com/Azure/azure-iot-device-auth/blob/master/dps_client/adapters/custom_hsm_tpm_impl.h) or [x509](https://github.com/Azure/azure-iot-device-auth/blob/master/dps_client/adapters/custom_hsm_x509_impl.h)

## DPS client

- Once your library is successfully compiled, you can move to the IoThub C-SDK and pull in the repo:

  - Supply the custom HSM library path and name in the cmake command:

  ```Shell
    cmake -Ddps_auth_type=[HSM_AUTH_TYPE] -Ddps_hsm_custom_lib=[CUSTOM HSM PATH] [PATH_TO_AZURE_IOT_SDK]
  ```

  The IoThub SDK will link with the custom HSM on the cmake command line

## Custom HSM API

The following is the list mandatory functions that need to be implemented for x509 or TPM custom HSMs.

### initialize_hsm_system

```c
int initialize_hsm_system();
```

Called to initialized the custom HSM system

### deinitialize_hsm_system

```c
void deinitialize_hsm_system();
```

Method used to deinitialize the HSM system

### custom_hsm_create

```C
DPS_CUSTOM_HSM_HANDLE custom_hsm_create();
```

Creates a custom HSM handle used for all the subsequent calls to the custom HSM

### custom_hsm_destroy

```C
void custom_hsm_destroy(DPS_CUSTOM_HSM_HANDLE handle);
```

Frees resources allocated in this module

#### HSM X509 API

### custom_hsm_get_certificate

```c
char* custom_hsm_get_certificate(DPS_CUSTOM_HSM_HANDLE handle);
```

- Retrieves the certificate to be used for x509 communication.

### custom_hsm_get_alias_key

```c
char* custom_hsm_get_alias_key(DPS_CUSTOM_HSM_HANDLE handle);
```

- Retrieves the alias key from the x509 certificate.

### custom_hsm_get_common_name

```c
char* custom_hsm_get_common_name(DPS_CUSTOM_HSM_HANDLE handle);
```

- Retrieves the common name from the x509 certificate.

### custom_hsm_get_signer_cert

```c
char* custom_hsm_get_signer_cert(DPS_CUSTOM_HSM_HANDLE handle);
```

- Retrieves the signer certificate used to sign the x509 certificate.

### custom_hsm_get_root_cert

```c
char* custom_hsm_get_root_cert(DPS_CUSTOM_HSM_HANDLE handle);
```

- Retrieves the root CA certificate used to sign the x509 certificate.

#### HSM TPM API

### custom_hsm_get_endorsement_key

```c
int custom_hsm_get_endorsement_key(DPS_CUSTOM_HSM_HANDLE handle, unsigned char** key, size_t* key_len);
```

- Retrieves the endorsement key of the TPM in the parameter key and the size in key_len.

### custom_hsm_get_storage_root_key

```c
int custom_hsm_get_storage_root_key(DPS_CUSTOM_HSM_HANDLE handle, unsigned char** key, size_t* key_len);
```

- Retrieves the storage root key of the TPM in the parameter key and the size in key_len.

### custom_hsm_import_key

```c
int custom_hsm_import_key(DPS_CUSTOM_HSM_HANDLE handle, const unsigned char* key, size_t key_len);
```

- Imports `key` that has been previously encrypted with the endorsement key and storage root key into the TPM key storage.

### custom_hsm_sign_key

```c
int custom_hsm_sign_key(DPS_CUSTOM_HSM_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len);
```

- Hashes the parameter `data` with the key previously stored in the TPM and returns  the value in `signed_value`.


[1]: ./media/dps_high_level_diagram.png