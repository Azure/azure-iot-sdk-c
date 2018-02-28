#TPM Attestation Requirements

##Overview

This module is used to manage data related to the TPM Attestation model, used with the Provisioning Service.

## Exposed API

```c
typedef struct TPM_ATTESTATION_TAG* TPM_ATTESTATION_HANDLE;

const char* tpmAttestation_getEndorsementKey(TPM_ATTESTATION_HANDLE tpm_att);
```


## tpmAttestation_getEndorsementKey

```c
const char* tpmAttestation_getEndorsementKey(TPM_ATTESTATION_HANDLE tpm_att);
```

**SRS_PROV_TPM_ATTESTATION_22_001: [** If the given handle, `tpm_att`, is `NULL`, `tpmAttestation_getEndorsementKey` shall return `NULL` **]**

**SRS_PROV_TPM_ATTESTATION_22_002: [** Otherwise, `tpmAttestation_getEndorsementKey` shall return the endorsement key contained in `tpm_att` **]**