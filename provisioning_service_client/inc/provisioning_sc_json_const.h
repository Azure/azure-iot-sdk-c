// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROVISIONING_SC_JSON_CONST_H
#define PROVISIONING_SC_JSON_CONST_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static const char* ATTESTATION_TYPE_JSON_VALUE_TPM = "tpm";
static const char* ATTESTATION_TYPE_JSON_VALUE_X509 = "x509";

static const char* PROVISIONING_STATUS_JSON_VALUE_ENABLED = "enabled";
static const char* PROVISIONING_STATUS_JSON_VALUE_DISABLED = "disabled";

static const char* REGISTRATION_STATUS_JSON_VALUE_UNASSIGNED = "unassigned";
static const char* REGISTRATION_STATUS_JSON_VALUE_ASSIGNING = "assigning";
static const char* REGISTRATION_STATUS_JSON_VALUE_ASSIGNED = "assigned";
static const char* REGISTRATION_STATUS_JSON_VALUE_FAILED = "failed";
static const char* REGISTRATION_STATUS_JSON_VALUE_DISABLED = "disabled";

static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_REG_ID = "registrationId";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_DEVICE_ID = "deviceId";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_REG_STATE = "registrationState";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_ATTESTATION = "attestation";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_IOTHUB_HOSTNAME = "iotHubHostName";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_INITIAL_TWIN = "initialTwin";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_ETAG = "etag";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_PROV_STATUS = "provisioningStatus";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_CREATED_TIME = "createdDateTimeUtc";
static const char* INDIVIDUAL_ENROLLMENT_JSON_KEY_UPDATED_TIME = "lastUpdatedDateTimeUtc";

static const char* ENROLLMENT_GROUP_JSON_KEY_GROUP_ID = "enrollmentGroupId";
static const char* ENROLLMENT_GROUP_JSON_KEY_ATTESTATION = "attestation";
static const char* ENROLLMENT_GROUP_JSON_KEY_IOTHUB_HOSTNAME = "iotHubHostName";
static const char* ENROLLMENT_GROUP_JSON_KEY_INITIAL_TWIN = "initialTwin";
static const char* ENROLLMENT_GROUP_JSON_KEY_ETAG = "etag";
static const char* ENROLLMENT_GROUP_JSON_KEY_PROV_STATUS = "provisioningStatus";
static const char* ENROLLMENT_GROUP_JSON_KEY_CREATED_TIME = "createdDateTimeUtc";
static const char* ENROLLMENT_GROUP_JSON_KEY_UPDATED_TIME = "lastUpdatedDateTimeUtc";

static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_REG_ID = "registrationId";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_CREATED_TIME = "createdDateTimeUtc";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_DEVICE_ID = "deviceId";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_REG_STATUS = "status";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_UPDATED_TIME = "lastUpdatedDateTimeUtc";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_ERROR_CODE = "errorCode";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_ERROR_MSG = "errorMessage";
static const char* DEVICE_REGISTRATION_STATE_JSON_KEY_ETAG = "etag";

static const char* ATTESTATION_MECHANISM_JSON_KEY_TYPE = "type";
static const char* ATTESTATION_MECHANISM_JSON_KEY_TPM = "tpm";
static const char* ATTESTATION_MECHANISM_JSON_KEY_X509 = "x509";

static const char* TPM_ATTESTATION_JSON_KEY_EK = "endorsementKey";
static const char* TPM_ATTESTATION_JSON_KEY_SRK = "storageRootKey";

static const char* X509_ATTESTATION_JSON_KEY_CLIENT_CERTS = "clientCertificates";
static const char* X509_ATTESTATION_JSON_KEY_SIGNING_CERTS = "signingCertificates";
static const char* X509_ATTESTATION_JSON_KEY_CA_REFERENCES = "caReferences";

static const char* X509_CERTIFICATES_JSON_KEY_PRIMARY = "primary";
static const char* X509_CERTIFICATES_JSON_KEY_SECONDARY = "secondary";

static const char* X509_CERTIFICATE_WITH_INFO_JSON_KEY_CERTIFICATE = "certificate";
static const char* X509_CERTIFICATE_WITH_INFO_JSON_KEY_INFO = "info";

static const char* X509_CERTIFICATE_INFO_JSON_KEY_SUBJECT_NAME = "subjectName";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_SHA1 = "sha1Thumbprint";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_SHA256 = "sha256Thumbprint";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_ISSUER = "issuerName";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_NOT_BEFORE = "notBeforeUtc";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_NOT_AFTER = "notAfterUtc";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_SERIAL_NO = "serialNumber";
static const char* X509_CERTIFICATE_INFO_JSON_KEY_VERSION = "version";

static const char* X509_CA_REFERENCES_JSON_KEY_PRIMARY = "primary";
static const char* X509_CA_REFERENCES_JSON_KEY_SECONDARY = "secondary";

static const char* INITIAL_TWIN_JSON_KEY_TAGS = "tags";
static const char* INITIAL_TWIN_JSON_KEY_PROPERTIES = "properties";

static const char* INITIAL_TWIN_PROPERTIES_JSON_KEY_DESIRED = "desired";

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PROVISIONING_SC_JSON_CONST_H */