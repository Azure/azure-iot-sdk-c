// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROVISIONING_SC_ENROLLMENT_H
#define PROVISIONING_SC_ENROLLMENT_H

#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    #define REGISTRATION_STATUS_VALUES \
    REG_STATUS_UNASSIGNED, \
    REG_STATUS_ASSIGNING, \
    REG_STATUS_ASSIGNED, \
    REG_STATUS_REGISTRATION_FAILED, \
    REG_STATUS_REGISTRATION_DISABLED \

    DEFINE_ENUM(REGISTRATION_STATUS, REGISTRATION_STATUS_VALUES);

    #define ATTESTATION_TYPE_VALUES \
    ATTESTATION_TYPE_NONE, \
    ATTESTATION_TYPE_TPM, \
    ATTESTATION_TYPE_X509 \

    DEFINE_ENUM(ATTESTATION_TYPE, ATTESTATION_TYPE_VALUES);

    #define PROVISIONING_STATUS_VALUES \
    PROVISIONING_STATUS_ENABLED, \
    PROVISIONING_STATUS_DISABLED \

    DEFINE_ENUM(PROVISIONING_STATUS, PROVISIONING_STATUS_VALUES);

    typedef struct TPM_ATTESTATION_TAG
    {
        char* endorsement_key;
        char* storage_root_key;
    } TPM_ATTESTATION;

    typedef struct X509_CERTIFICATE_INFO_TAG
    {
        char* subject_name;
        char* sha1_thumbprint;
        char* sha256_thumbprint;
        char* issuer_name;
        time_t not_before_utc;
        time_t not_after_utc;
        char* serial_number;
        int version;
    } X509_CERTIFICATE_INFO;

    typedef struct X509_CERTIFICATE_WITH_INFO_TAG
    {
        void* certificate;
        X509_CERTIFICATE_INFO* info;
    } X509_CERTIFICATE_WITH_INFO;

    typedef struct X509_CERTIFICATES_TAG
    {
        X509_CERTIFICATE_WITH_INFO* primary;
        X509_CERTIFICATE_WITH_INFO* secondary;
    } X509_CERTIFICATES;

    typedef struct X509_ATTESTATION_TAG
    {
        X509_CERTIFICATES* client_certificates;
        X509_CERTIFICATES* signing_certificates;
    } X509_ATTESTATION;

    typedef struct ATTESTATION_MECHANISM_TAG
    {
        ATTESTATION_TYPE type;
        union {
            TPM_ATTESTATION tpm;
            X509_ATTESTATION x509;
        } attestation;
    } ATTESTATION_MECHANISM;

    typedef struct METADATA_TAG
    {
        time_t last_updated;
        int last_updated_version;
    } METADATA;

    typedef struct TWIN_COLLECTION_TAG
    {
        int version;
        int count;
        METADATA* metadata;
    } TWIN_COLLECTION;

    typedef struct TWIN_STATE_TAG{
        TWIN_COLLECTION* tags;
        TWIN_COLLECTION* desired_properties;
    } TWIN_STATE;

    typedef struct DEVICE_REGISTRATION_STATUS_TAG
    {
        const char* registration_id;
        const time_t created_date_time_utc;
        const char* assigned_hub;
        const char* device_id;
        REGISTRATION_STATUS status;
        time_t updated_date_time_utc;
        int error_code;
        const char* error_message;
        const char* etag;
    } DEVICE_REGISTRATION_STATUS;

    typedef struct INDIVIDUAL_ENROLLMENT_TAG
    {
        const char* registration_id;
        const char* device_id;
        DEVICE_REGISTRATION_STATUS* registration_status;
        ATTESTATION_MECHANISM* attestation;
        char* iothub_hostname;
        TWIN_STATE* initial_twin_state;
        char* etag;
        char* generation_id;
        PROVISIONING_STATUS provisioning_status;
        const time_t created_date_time_utc;
        time_t updated_date_time_utc;
    } INDIVIDUAL_ENROLLMENT;

    typedef struct ENROLLMENT_GROUP_TAG
    {
        const char* enrollment_group_id;
        ATTESTATION_MECHANISM* attestation;
        char* iothub_hostname;
        TWIN_STATE* initial_twin_state;
        char* etag;
        PROVISIONING_STATUS provisioning_status;
        const time_t created_date_time_utc;
        time_t updated_date_time_utc;
    } ENROLLMENT_GROUP;


    MOCKABLE_FUNCTION(, char*, individualEnrollment_toJson, const INDIVIDUAL_ENROLLMENT*, enrollment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PROVISIONING_SC_ENROLLMENT_H */
