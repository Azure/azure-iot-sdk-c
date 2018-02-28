# X509 Attestation Requirements

## Overview

This module is used to manage data related to the X509 Attestation and X509 Certificate models, used with the Provisioning Service

## Exposed API

```c
typedef struct X509_ATTESTATION_TAG* X509_ATTESTATION_HANDLE;
typedef struct X509_CERTIFICATE_WITH_INFO_TAG* X509_CERTIFICATE_HANDLE;

typedef enum X509_CERTIFICATE_TYPE_VALUES {
    X509_CERTIFICATE_TYPE_NONE,
    X509_CERTIFICATE_TYPE_CLIENT,
    X509_CERTIFICATE_TYPE_SIGNING,
    X509_CERTIFICATE_TYPE_CA_REFERENCES
} X509_CERTIFICATE_TYPE

X509_CERTIFICATE_TYPE x509Attestation_getCertificateType(X509_ATTESTATION_HANDLE x509_att);
X509_CERTIFICATE_HANDLE x509Attestation_getPrimaryCertificate(X509_ATTESTATION_HANDLE x509_att);
X509_CERTIFICATE_HANDLE x509Attestation_getSecondaryCertificate(X509_ATTESTATION_HANDLE x509_att);

const char* x509Certificate_getSubjectName(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getSha1Thumbprint(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getSha256Thumbprint(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getIssuerName(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getNotBeforeUtc(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getNotAfterUtc(X509_CERTIFICATE_HANDLE x509_cert);
const char* x509Certificate_getSerialNumber(X509_CERTIFICATE_HANDLE x509_cert);
int x509Certificate_getVersion(X509_CERTIFICATE_HANDLE x509_cert);
```


## x509Attestation_getCertificateType

```c
X509_CERTIFICATE_TYPE x509Attestation_getCertificateType(X509_ATTESTATION_HANDLE x509_att);
```

**SRS_X509_ATTESTATION_22_001: [** If the given handle, `x509_att` is `NULL`, `x509Attestation_getCertificateType` shall fail and return `X509_CERTIFICATE_TYPE_NONE` **]**

**SRS_X509_ATTESTATION_22_002: [** Otherwise, `x509Attestation_getCertificateType` shall return the certificate type of `x509_att` **]**


## x509Attestation_getPrimaryCertificate

```c
X509_CERTIFICATE_HANDLE x509Attestation_getPrimaryCertificate(X509_ATTESTATION_HANDLE x509_att);
```

**SRS_X509_ATTESTATION_22_003: [** If the given handle, `x509_att` is `NULL`, `x509Attestation_getPrimaryCertificate` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_004: [** If `x509_att` is an X509 attestation with client certificate(s), `x509Attestation_getPrimaryCertificate` shall return the primary client certificate as an `X509_CERTIFICATE_HANDLE` **]**

**SRS_X509_ATTESTATION_22_005: [** If `x509_att` is an X509 attestation with signing certificate(s), `x509Attestation_getPrimaryCertificate` shall return the primary signing certificate as an `X509_CERTIFICATE_HANDLE` **]**

**SRS_X509_ATTESTATION_22_006: [** If `x509_att` is another type of X509 attestation, `x509Attestation_getPrimaryCertificate` shall fail and return `NULL` **]**


## x509Attestation_getSecondaryCertificate

```c
X509_CERTIFICATE_HANDLE x509Attestation_getSecondaryCertificate(X509_ATTESTATION_HANDLE x509_att);
```

**SRS_X509_ATTESTATION_22_007: [** If the given handle, `x509_att` is `NULL`, `x509Attestation_getSecondaryCertificate` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_035: [** If `x509_att` does not have a secondary certificate, `x509Attestation_getSecondaryCertificate` shall return `NULL` **]**

**SRS_X509_ATTESTATION_22_008: [** If `x509_att` is an X509 attestation with client certificate(s), `x509Attestation_getSecondaryCertificate` shall return the secondary client certificate as an `X509_CERTIFICATE_HANDLE` **]**

**SRS_X509_ATTESTATION_22_009: [** If `x509_att` is an X509 attestation with signing certificate(s), `x509Attestation_getSecondaryCertificate` shall return the secondary signing certificate as an `X509_CERTIFICATE_HANDLE` **]**

**SRS_X509_ATTESTATION_22_010: [** If `x509_att` is another type of X509 attestation, `x509Attestation_getSecondaryCertificate` shall fail and return `NULL` **]**


## x509Certificate_getSubjectName

```c
const char* x509Certificate_getSubjectName(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_011: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getSubjectName` shall fail and return `NULL` **]**

**SRS_X509_ATTESTAITON_22_012: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getSubjectName` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_013: [** Otherwise, `x509Certificate_getSubjectName` shall return the subject name from `x509_cert` **]**


## x509Certificate_getSha1Thumbprint

```c
const char* x509Certificate_getSha1Thumbprint(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_014: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getSha1Thumbprint` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_015: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getSha1Thumbprint` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_016: [** Otherwise, `x509Certificate_getSha1Thumbprint` shall return the sha1 thumbprint from `x509_cert` **]**


## x509Certificate_getSha256Thumbprint

```c
const char* x509Certificate_getSha256Thumbprint(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_017: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getSha256Thumbprint` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_018: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getSha256Thumbprint` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_019: [** Otherwise, `x509Certificate_getSha256Thumbprint` shall return the sha256 thumbprint from `x509_cert` **]**


## x509Certificate_getIssuerName

```c
const char* x509Certificate_getIssuerName(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_020: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getIssuerName` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_021: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getIssuerName` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_022: [** Otherwise, `x509Certificate_getIssuerName` shall return the issuer name from `x509_cert` **]**


## x509Certificate_getNotBeforeUtc

```c
const char* x509Certificate_getNotBeforeUtc(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_023: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getNotBeforeUtc` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_024: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getNotBeforeUtc` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_025: [** Otherwise, `x509Certificate_getNotBeforeUtc` shall return the "not before utc" from `x509_cert` **]**


## x509Certificate_getNotAfterUtc

```c
const char* x509Certificate_getNotAfterUtc(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_026: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getNotAfterUtc` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_027: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getNotAfterUtc` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_028: [** Otherwise, `x509Certificate_getNotAfterUtc` shall return the "not after utc" from `x509_cert` **]**


## x509Certificate_getSerialNumber

```c
const char* x509Certificate_getSerialNumber(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_029: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getSerialNumber` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_030: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getSerialNumber` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_031: [** Otherwise, `x509Certificate_getSerialNumber` shall return the serial number from `x509_cert` **]**


## x509Certificate_getVersion

```c
int x509Certificate_getVersion(X509_CERTIFICATE_HANDLE x509_cert);
```

**SRS_X509_ATTESTATION_22_032: [** If the given handle, `x509_cert` is `NULL`, `x509Certificate_getVersion` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_033: [** If the given handle, `x509_cert` has not yet been processed by the Provisioning Service, `x509Certificate_getVersion` shall fail and return `NULL` **]**

**SRS_X509_ATTESTATION_22_034: [** Otherwise, `x509Certificate_getVersion` shall return the version from `x509_cert` **]**