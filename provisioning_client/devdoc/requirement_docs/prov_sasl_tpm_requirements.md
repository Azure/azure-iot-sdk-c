# DPS SASL TPM Requirements

================================

## Overview

prov_sasl_tpm module implements the sasl protocol for the amqp dps system.

## Dependencies

uAmqp repo

## Exposed API

```c
    typedef char*(*TPM_CHALLENGE_CALLBACK)(const unsigned char* data, size_t length, void* user_ctx);

    typedef struct SASL_TPM_CONFIG_INFO_TAG
    {
        BUFFER_HANDLE endorsement_key;
        BUFFER_HANDLE storage_root_key;
        const char* registration_id;
        const char* hostname;

        TPM_CHALLENGE_CALLBACK challenge_cb;
        void* user_ctx;
    } SASL_TPM_CONFIG_INFO;

    MOCKABLE_FUNCTION(, const SASL_MECHANISM_INTERFACE_DESCRIPTION*, prov_sasltpm_get_interface);
```

### prov_sasltpm_create

```c
static CONCRETE_SASL_MECHANISM_HANDLE prov_sasltpm_create(void* config)
```

**SRS_PROV_SASL_TPM_07_001: [** On success `prov_sasltpm_create` shall allocate a new instance of the `SASL_TPM_INSTANCE`. **]**

**SRS_PROV_SASL_TPM_07_002: [** If `config` is NULL, `prov_sasltpm_create` shall return NULL. **]**

**SRS_PROV_SASL_TPM_07_003: [** If any error is encountered, `prov_sasltpm_create` shall return NULL. **]**

**SRS_PROV_SASL_TPM_07_029: [** `prov_sasltpm_create` shall copy the `config` data where needed. **]**

**SRS_PROV_SASL_TPM_07_004: [** if `SASL_TPM_CONFIG_INFO`, `challenge_cb`, `endorsement_key`, `storage_root_key`, or `prov_hostname`, `registration_id` is NULL, `prov_sasltpm_create` shall fail and return NULL. **]**

### prov_sasltpm_destroy

```c
void prov_sasltpm_destroy(CONCRETE_SASL_MECHANISM_HANDLE handle)
```

**SRS_PROV_SASL_TPM_07_005: [** if handle is NULL, `prov_sasltpm_destroy` shall do nothing. **]**

**SRS_PROV_SASL_TPM_07_006: [** `prov_sasltpm_create` shall free the `SASL_TPM_INSTANCE` instance. **]**

**SRS_PROV_SASL_TPM_07_007: [** `prov_sasltpm_create` shall free all resources allocated in this module. **]**

### prov_sasltpm_get_mechanism_name

```c
const char* prov_sasltpm_get_mechanism_name(CONCRETE_SASL_MECHANISM_HANDLE handle)
```

**SRS_PROV_SASL_TPM_07_008: [** if handle is NULL, `prov_sasltpm_get_mechanism_name` shall return NULL. **]**

**SRS_PROV_SASL_TPM_07_009: [** `prov_sasltpm_get_mechanism_name` shall return the mechanism name `TPM`. **]**

### construct_send_data

**SRS_PROV_SASL_TPM_07_024: [** If the data to be sent is greater than 470 bytes the data will be chunked to the server. **]**

**SRS_PROV_SASL_TPM_07_025: [** If data is chunked a single control byte will be prepended to the data as described below: **]**

| Name            |  Size  |Position |
|-----------------|--------|---------|
| ctrl byte used  | 1 byte |    7    |
| Last Seg sent   | 1 byte |    6    |
| Sequence Num    | 3 byte | 0, 1, 2 |

**SRS_PROV_SASL_TPM_07_026: [** If the current bytes are the last chunk in the sequence `construct_send_data` shall mark the Last seg Sent bit **]**

**SRS_PROV_SASL_TPM_07_027: [** The sequence number shall be incremented after every send of chunked bytes. **]**

**SRS_PROV_SASL_TPM_07_028: [** If the data is less than 470 bytes the control byte shall be set to 0. **]**

### prov_sasltpm_get_init_bytes

```c
int prov_sasltpm_get_init_bytes(CONCRETE_SASL_MECHANISM_HANDLE handle, SASL_MECHANISM_BYTES* init_bytes)
```

**SRS_PROV_SASL_TPM_07_010: [** If `handle` or `init_bytes` are NULL, `prov_sasltpm_get_init_bytes` shall return NULL. **]**

**SRS_PROV_SASL_TPM_07_011: [** If `sasl_tpm_info initial_data` has been allocated, `prov_sasltpm_get_init_bytes` shall free the memory **]**

**SRS_PROV_SASL_TPM_07_012: [** `prov_sasltpm_get_init_bytes` shall send the control byte along with the EK value, ctrl byte detailed in Send Data to dps sasltpm **]**

**SRS_PROV_SASL_TPM_07_013: [** If any error is encountered, `prov_sasltpm_get_init_bytes` shall return a non-zero value. **]**

**SRS_PROV_SASL_TPM_07_014: [** On success `prov_sasltpm_get_init_bytes` shall return a zero value. **]**

### prov_sasltpm_challenge

```c
int prov_sasltpm_challenge(CONCRETE_SASL_MECHANISM_HANDLE handle, const SASL_MECHANISM_BYTES* challenge_bytes, SASL_MECHANISM_BYTES* resp_bytes)
```

**SRS_PROV_SASL_TPM_07_015: [** if `handle` or `resp_bytes` are NULL, `prov_sasltpm_challenge` shall return the `X509_SECURITY_INTERFACE` structure. **]**

**SRS_PROV_SASL_TPM_07_016: [** If the `challenge_bytes->bytes` first byte is NULL `prov_sasltpm_challenge` shall send the SRK data to the server. **]**

**SRS_PROV_SASL_TPM_07_022: [** `prov_sasltpm_challenge` shall send the control byte along with the SRK value, ctrl byte detailed in Send Data to dps sasl tpm **]**

**SRS_PROV_SASL_TPM_07_017: [** `prov_sasltpm_challenge` accumulates challenge bytes and waits for the last sequence bit to be set. **]**

**SRS_PROV_SASL_TPM_07_023: [** If the last sequence bit is not encountered `prov_sasltpm_challenge` shall return 1 byte to the service. **]**

**SRS_PROV_SASL_TPM_07_018: [** When the all the bytes are store, `prov_sasltpm_challenge` shall call the challenge callback to receive the sas token. **]**

**SRS_PROV_SASL_TPM_07_019: [** The Sas Token shall be put into the response bytes buffer to be return to the DPS SASL server. **]**

**SRS_PROV_SASL_TPM_07_020: [** If any error is encountered `prov_sasltpm_challenge` shall return a non-zero value. **]**

### prov_sasltpm_get_interface

```c
const SASL_MECHANISM_INTERFACE_DESCRIPTION* prov_sasltpm_get_interface(void)
```

**SRS_PROV_SASL_TPM_07_021: [** `prov_sasltpm_get_interface` shall return the `SASL_MECHANISM_INTERFACE_DESCRIPTION` structure. **]**
