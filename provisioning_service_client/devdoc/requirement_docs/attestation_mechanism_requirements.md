# Attestation Mechanism Requirements

## Overview

This module is used to manage data related to the Attestation Mechanism model, used with the Provisioning Service

## Exposed API

```c
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithTpm(const char* endorsement_key, const char* storage_root_key);
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509ClientCert(const char* primary_cert, const char* secondary_cert);
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509SigningCert(const char* primary_cert, const char* secondary_cert);
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509CAReference(const char* primary_ref, const char* secondary_ref);
void attestationMechanism_destroy(ATTESTATION_MECHANISM_HANDLE att_mech);
bool attestationMechanism_isValidForIndividualEnrollment(ATTESTATION_MECHANISM_HANDLE att_mech);
bool attestationMechanism_isValidForEnrollmentGroup(ATTESTATION_MECHANISM_HANDLE att_mech);
ATTESTATION_TYPE attestationMechanism_getType(ATTESTATION_MECHANISM_HANDLE att_mech);
TPM_ATTESTATION_HANDLE attestationMechanism_getTpmAttestation(ATTESTATION_MECHANISM_HANDLE att_mech);
X509_ATTESTATION_HANDLE attestationMechanism_getX509Attestation(ATTESTATION_MECHANISM_HANDLE att_mech);
```


## attestationMechanism_createWithTpm

```c
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithTpm(const char* endorsement_key, const char* storage_root_key);
```

**SRS_PROV_ATTESTATION_22_001: [** If `endorsement_key` is `NULL`, `attestationMechanism_createWithTpm` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_029: [** If `storage_root_key` is `NULL`, `attestationMechanism_createWithTpm` shall create a TPM attestation without a storage root key **]**

**SRS_PROV_ATTESTATION_22_002: [** If allocating memory for the new attestation mechanism fails, `attestationMechanism_createWithTpm` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_003: [** Upon successful creation of a new attestation mechanism (using a TPM attestation), `attestationMechanism_createWithTpm` shall return an `ATTESTATION_MECHANISM_HANDLE` to access the model **]**


## attestationMechanism_createWithX509ClientCert

```c
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509ClientCert(const char* primary_cert, const char* secondary_cert);
```

**SRS_PROV_ATTESTATION_22_004: [** If `primary_cert` is `NULL`, `attestationMechanism_createWithX509ClientCert` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_005: [** If `secondary_cert` is `NULL`, `attestationMechanism_createWithX509ClientCert` shall create an X509 attestation without a secondary certificate **]**

**SRS_PROV_ATTESTATION_22_006: [** If allocating memory for the new attestation mechanism fails, `attestationMechanism_createWithX509ClientCert` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_007: [** Upon successful creation of a new attestation mechanism (using an X509 attestation with client certificates), `attestationMechanism_createWithX509ClientCert` shall return an `ATTESTATION_MECHANISM_HANDLE` to access the model **]**


## attestationMechanism_createWithX509SigningCert

```c
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509SigningCert(const char* primary_cert, const char* secondary_cert);
```

**SRS_PROV_ATTESTATION_22_008: [** If `primary_cert` is `NULL`, `attestationMechanism_createWithX509SigningCert` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_009: [** If `secondary_cert` is `NULL`,`attestationMechanism_createWithX509SigningCert` shall create an X509 attestation without a secondary certificate  **]**

**SRS_PROV_ATTESTATION_22_010: [** If allocating memory for the new attestation mechanism fails, `attestationMechanism_createWithX509SigningCert` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_011: [** Upon successful creation of a new attestation mechanism (using an X509 attestation with signing certificates), `attestationMechanism_createWithX509SigningCert` shall return an `ATTESTATION_MECHANISM_HANDLE` to access the model **]**


## attestationMechanism_createWithX509CAReference

```c
ATTESTATION_MECHANISM_HANDLE attestationMechanism_createWithX509CAReference(const char* primary_ref, const char* secondary_ref);
```

**SRS_PROV_ATTESTATION_22_012: [** If `primary_ref` is `NULL`, `attestationMechanism_createWithX509CAReference` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_013: [** If `secondary_ref` is `NULL`, `attestationMechanism_createWithX509CAReference` shall create an X509 attestation without a secondary CA reference **]**

**SRS_PROV_ATTESTATION_22_014: [** If allocating memory for the new attestation mechanism fails, `attestationMechanism_createWithX509CAReference` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_015: [** Upon successful creation of a new attestation mechanism (using an X509 attestation with CA references), `attestationMechanism_createWithX509CAReference` shall return an `ATTESTATION_MECHANISM_HANDLE` to access the model **]**


## attestationMechanism_destroy

```c
void attestationMechanism_destroy(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_016: [** `attestationMechanism_destory` shall free all memory contained in the `att_mech` handle **]**


## attestationMechanism_isValidForIndividualEnrollment

```c
bool attestationMechanism_isValidForIndividualEnrollment(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_017: [** `attestationMechanism_isValidForIndividualEnrollment` shall return `true` if the given attestation mechanism, `att_mech` is valid for use with an individual enrollment **]**

**SRS_PROV_ATTESTATION_22_018: [** `attestationMechanism_isValidForIndividualEnrollment` shall return `false` if the given attestation mechanism, `att_mech` is invalid for use with an individual enrollment **]**


## attestationMechanism_isValidForEnrollmentGroup

```c
bool attestationMechanism_isValidForEnrollmentGroup(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_019: [** `attestationMechanism_isValidForEnrollmentGroup` shall return `true` if the given attestation mechanism, `att_mech` is valid for use with an enrollment group **]**

**SRS_PROV_ATTESTATION_22_020: [** `attestationMechanism_isValidForEnrollmentGroup` shall return `false` if the given attestation mechanism, `att_mech` is invalid for use with an enrollment group **]**


## attestationMechanism_getType

```c
ATTESTATION_TYPE attestationMechanism_getType(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_021: [** If `att_mech` is `NULL`, `attestationMechanism_getType` shall return `ATTESTATION_TYPE_NONE` **]**


**SRS_PROV_ATTESTATION_22_022: [** Otherwise, `attestationMechanism_getType` shall return the type of `att_mech` **]**


## attestationMechanism_getTpmAttestation

```c
TPM_ATTESTATION_HANDLE attestationMechanism_getTpmAttestation(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_023: [** If `att_mech` is `NULL`, `attestationMechanism_getTpmAttestation` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_024: [** If `att_mech` does not have a TPM attestation, `attestationMechanism_getTpmAttestation` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_025: [** Otherwise, `attestationMechanism_getTpmAttestation` shall return the underlying TPM attestation of `att_mech` **]**


## attestationMechanism_getX509Attestation

```c
X509_ATTESTATION_HANDLE attestationMechanism_getX509Attestation(ATTESTATION_MECHANISM_HANDLE att_mech);
```

**SRS_PROV_ATTESTATION_22_026: [** If `att_mech` is `NULL`, `attestationMechanism_getX509Attestation` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_027: [** If `att_mech` does not have an X509 attestation, `attestationMechanism_getX509Attestation` shall fail and return `NULL` **]**

**SRS_PROV_ATTESTATION_22_028: [** Otherwise, `attestationMechanism_getX509Attestation` shall return the underlying X509 attestation of `att_mech` **]**