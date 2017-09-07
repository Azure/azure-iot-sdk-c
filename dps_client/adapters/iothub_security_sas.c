// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/sha.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/buffer_.h"

#include "azure_hub_modules/iothub_security_factory.h"
#include "iothub_security_sas.h"

#include "azure_utpm_c/tpm_comm.h"
#include "azure_utpm_c/tpm_codec.h"

#include "azure_utpm_c/Marshal_fp.h"     // for activation blob unmarshaling

#define EPOCH_TIME_T_VALUE          0
#define HMAC_LENGTH                 32

static TPM2B_AUTH      NullAuth = { 0 };
static TSS_SESSION     NullPwSession;
static const UINT32 TPM_20_SRK_HANDLE = HR_PERSISTENT | 0x00000001;
static const UINT32 TPM_20_EK_HANDLE = HR_PERSISTENT | 0x00010001;

typedef struct SECURITY_DEVICE_INFO_TAG
{
    TPM_HANDLE tpm_handle;
    TSS_DEVICE tpm_device;
    TPM2B_PUBLIC ek_pub;
    TPM2B_PUBLIC srk_pub;

    TPM2B_PUBLIC id_key_public;
    TPM2B_PRIVATE id_key_dup_blob;
    TPM2B_PRIVATE id_key_priv;
} SECURITY_DEVICE_INFO;

static const SAS_SECURITY_INTERFACE sas_security_interface =
{
    iothub_security_sas_create,
    iothub_security_sas_destroy,
    iothub_security_sas_sign_data,
};

static TPMS_RSA_PARMS  RsaStorageParams = {
    { TPM_ALG_AES, 128, TPM_ALG_CFB },      // TPMT_SYM_DEF_OBJECT  symmetric
    { TPM_ALG_NULL },                       // TPMT_RSA_SCHEME      scheme
    2048,                                   // TPMI_RSA_KEY_BITS    keyBits
    0                                       // UINT32               exponent
};

static TPM2B_PUBLIC* GetEkTemplate ()
{
    static TPM2B_PUBLIC EkTemplate = { 0,   // size will be computed during marshaling
    {
        TPM_ALG_RSA,                    // TPMI_ALG_PUBLIC      type
        TPM_ALG_SHA256,                 // TPMI_ALG_HASH        nameAlg
        { 0 },                          // TPMA_OBJECT  objectAttributes (set below)
        {32,
        { 0x83, 0x71, 0x97, 0x67, 0x44, 0x84, 0xb3, 0xf8,
        0x1a, 0x90, 0xcc, 0x8d, 0x46, 0xa5, 0xd7, 0x24,
        0xfd, 0x52, 0xd7, 0x6e, 0x06, 0x52, 0x0b, 0x64,
        0xf2, 0xa1, 0xda, 0x1b, 0x33, 0x14, 0x69, 0xaa }
        },                              // TPM2B_DIGEST         authPolicy
        { 0 },                          // TPMU_PUBLIC_PARMS    parameters (set below)
        { 0 }                           // TPMU_PUBLIC_ID       unique
    } };
    EkTemplate.publicArea.objectAttributes = ToTpmaObject(
        Restricted | Decrypt | FixedTPM | FixedParent | AdminWithPolicy | SensitiveDataOrigin);
    EkTemplate.publicArea.parameters.rsaDetail = RsaStorageParams;
    return &EkTemplate;
}

static TPM2B_PUBLIC* GetSrkTemplate()
{
    static TPM2B_PUBLIC SrkTemplate = { 0,  // size will be computed during marshaling
    {
        TPM_ALG_RSA,                // TPMI_ALG_PUBLIC      type
        TPM_ALG_SHA256,             // TPMI_ALG_HASH        nameAlg
        { 0 },                      // TPMA_OBJECT  objectAttributes (set below)
        { 0 },                      // TPM2B_DIGEST         authPolicy
        { 0 },                      // TPMU_PUBLIC_PARMS    parameters (set before use)
        { 0 }                       // TPMU_PUBLIC_ID       unique
    } };
    SrkTemplate.publicArea.objectAttributes = ToTpmaObject(
        Restricted | Decrypt | FixedTPM | FixedParent | NoDA | UserWithAuth | SensitiveDataOrigin);
    SrkTemplate.publicArea.parameters.rsaDetail = RsaStorageParams;
    return &SrkTemplate;
}

#define DPS_UNMARSHAL(Type, pValue) \
{                                                                       \
    TPM_RC rc = Type##_Unmarshal(pValue, &curr_pos, (INT32*)&act_size);         \
    if (rc != TPM_RC_SUCCESS)                                           \
    {                                                                   \
        LogError(#Type"_Unmarshal() for " #pValue " failed");           \
    }                                                                   \
}

#define DPS_UNMARSHAL_FLAGGED(Type, pValue) \
{                                                                       \
    TPM_RC rc = Type##_Unmarshal(pValue, &curr_pos, (INT32*)&act_size, TRUE);   \
    if (rc != TPM_RC_SUCCESS)                                           \
    {                                                                   \
        LogError(#Type"_Unmarshal() for " #pValue " failed");           \
    }                                                                   \
}

#define DPS_UNMARSHAL_ARRAY(dstPtr, arrSize) \
    DPS_UNMARSHAL(UINT32, &(arrSize));                                          \
    printf("act_size %d < actSize %d\r\n", act_size, arrSize);   \
    if (act_size < arrSize)                                                     \
    {                                                                           \
        LogError("Unmarshaling " #dstPtr " failed: Need %d bytes, while only %d left", arrSize, act_size);  \
        result = __FAILURE__;       \
    }                                                                           \
    else                            \
    {                                   \
        dstPtr = curr_pos - sizeof(UINT16);                                         \
        *(UINT16*)dstPtr = (UINT16)arrSize;                                         \
        curr_pos += arrSize;                         \
    }

static int dps_umarshal_array(unsigned char* dst_ptr, uint32_t dest_size, unsigned char* act_buff, uint32_t act_size)
{
    int result;
    uint8_t* curr_pos = act_buff;

    DPS_UNMARSHAL(UINT32, &(dest_size));
    if (act_size < dest_size)
    {
        LogError("Unmarshaling failed: Need %d bytes, while only %d left", dest_size, act_size);
        result = __LINE__;
    }
    else
    {
        dst_ptr = act_buff - sizeof(UINT16);
        *(UINT16*)dst_ptr = (UINT16)dest_size;
        act_buff += dest_size;
        result = 0;
    }
    return result;
}

static int unmarshal_array(uint8_t* dstptr, uint32_t size, uint8_t** curr_pos, uint32_t* curr_size)
{
    int result;

    TPM_RC tpm_res = UINT32_Unmarshal((uint32_t*)dstptr, curr_pos, (int32_t*)curr_size);
    if (tpm_res != TPM_RC_SUCCESS)
    {
        LogError("Failure: unmarshalling array.");
        result = __FAILURE__;
    }
    else if (*curr_size < size)
    {
        LogError("Failure: unmarshalling array need %d bytes, while only %d left.", size, *curr_size);
        result = __FAILURE__;
    }
    else
    {
        dstptr = *curr_pos - sizeof(UINT16);
        *(UINT16*)dstptr = (UINT16)size;
        curr_pos += size;
        result = 0;
    }
    return result;
}

static int marshal_array_values(const unsigned char* key, size_t key_len, uint8_t** decrypt_blob, uint8_t** decrypt_secret, uint8_t** decrypt_wrap_key, TPM2B_PRIVATE* enc_key_blob)
{
    int result = 0;
    uint8_t* curr_pos = (uint8_t*)key;
    uint32_t act_size = (int32_t)key_len;
    uint32_t decrypt_size = 0;
    uint32_t decrypt_secret_size = 0;
    uint32_t decrypt_key_size = 0;
    TPM2B_PUBLIC id_key_Public = { TPM_ALG_NULL };
    UINT16 gratuitousSizeField;    // WORKAROUND for the current protocol

    DPS_UNMARSHAL_ARRAY(*decrypt_blob, decrypt_size);
    if (result != 0)
    {
        LogError("Failure: decrypting blob");
    }
    else
    {
        DPS_UNMARSHAL_ARRAY(*decrypt_secret, decrypt_secret_size);
        if (result != 0)
        {
            LogError("Failure: decrypting secret");
        }
        else
        {
            DPS_UNMARSHAL_ARRAY(*decrypt_wrap_key, decrypt_key_size);
            if (result != 0)
            {
                LogError("Failure: decrypting wrap secret");
            }
            else
            {
                DPS_UNMARSHAL_FLAGGED(TPM2B_PUBLIC, &id_key_Public);
                if (result != 0)
                {
                    LogError("Failure: id key public");
                }
                else
                {
                    DPS_UNMARSHAL(UINT16, &gratuitousSizeField);
                    if (result != 0)
                    {
                        LogError("Failure: gratuitousSizeField");
                    }
                    else
                    {
                        DPS_UNMARSHAL(TPM2B_PRIVATE, enc_key_blob);
                        if (result != 0)
                        {
                            LogError("Failure: enc key blob");
                        }
                    }
                }
            }
        }
    }
    return result;
}

static int create_tpm_session(SECURITY_DEVICE_INFO* sec_info, TSS_SESSION* tpm_session)
{
    int result;
    TPMA_SESSION sess_attrib = { 1 };
    if (TSS_StartAuthSession(&sec_info->tpm_device, TPM_SE_POLICY, TPM_ALG_SHA256, sess_attrib, tpm_session) != TPM_RC_SUCCESS)
    {
        LogError("Failure: Starting EK policy session");
        result = __FAILURE__;
    }
    else if (TSS_PolicySecret(&sec_info->tpm_device, &NullPwSession, TPM_RH_ENDORSEMENT, tpm_session, NULL, 0) != TPM_RC_SUCCESS)
    {
        LogError("Failure: PolicySecret() for EK");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static BUFFER_HANDLE decrypt_data(SECURITY_DEVICE_INFO* sec_info, const unsigned char* key, size_t key_len)
{
    BUFFER_HANDLE result;
    TSS_SESSION ek_sess = { { TPM_RH_NULL } };
    UINT32 max_tpm_data_size;

    if (create_tpm_session(sec_info, &ek_sess) != 0)
    {
        LogError("Failure: Starting EK policy session");
        result = NULL;
    }
    else if ((max_tpm_data_size = TSS_GetTpmProperty(&sec_info->tpm_device, TPM_PT_INPUT_BUFFER)) == 0)
    {
        LogError("Failure: TSS_GetTpmProperty Input buffer");
        result = NULL;
    }
    else
    {
        TPM2B_ID_OBJECT tpm_blob;
        TPM2B_ENCRYPTED_SECRET tpm_enc_secret;
        TPM2B_ENCRYPTED_SECRET tpm_enc_key;
        TPM2B_PRIVATE id_key_dup_blob;
        UINT16 enc_data_size = 0;
        TPM2B_PUBLIC id_key_Public = { TPM_ALG_NULL };
        TPM2B_MAX_BUFFER* target_data = NULL;
        TPM2B_DIGEST inner_wrap_key = { 0 };

        uint8_t* curr_pos = (uint8_t*)key;
        uint32_t act_size = (int32_t)key_len;

        DPS_UNMARSHAL(TPM2B_ID_OBJECT, &tpm_blob);
        DPS_UNMARSHAL(TPM2B_ENCRYPTED_SECRET, &tpm_enc_secret);
        DPS_UNMARSHAL(TPM2B_PRIVATE, &id_key_dup_blob);
        DPS_UNMARSHAL(TPM2B_ENCRYPTED_SECRET, &tpm_enc_key);
        DPS_UNMARSHAL_FLAGGED(TPM2B_PUBLIC, &id_key_Public);

        // The given TPM may support larger TPM2B_MAX_BUFFER than this API headers define.
        // So instead of unmarshaling data in a standalone data structure just reuse the
        // original activation buffer (after updating byte order of the UINT16 counter)
        DPS_UNMARSHAL(UINT16, &enc_data_size);

        if (enc_data_size > max_tpm_data_size)
        {
            LogError("Failure: The encrypted data len (%zu) is too long for tpm", enc_data_size);
            result = NULL;
        }
        else
        {
            target_data = (TPM2B_MAX_BUFFER*)(curr_pos - sizeof(UINT16));
            target_data->t.size = enc_data_size;

            // Update local vars in case activation blob contains more data to unmarshal
            curr_pos += enc_data_size;
            act_size -= enc_data_size;

            // decrypts encrypted symmetric key ‘encSecret‘ and returns it as 'tpm_blob'.
            // Later 'tpm_blob' is used as the inner wrapper key for import of the HMAC key blob.
            if (TPM2_ActivateCredential(&sec_info->tpm_device, &NullPwSession, &ek_sess, TPM_20_SRK_HANDLE, TPM_20_EK_HANDLE, &tpm_blob, &tpm_enc_secret, &inner_wrap_key) != TPM_RC_SUCCESS)
            {
                LogError("Failure: TPM2_ActivateCredential");
                result = NULL;
            }
            else
            {
                result = BUFFER_create(tpm_blob.t.credential, tpm_blob.t.size);
                if (result == NULL)
                {
                    LogError("Failure building buffer");
                    result = NULL;
                }
            }
        }
    }
    return result;
}

static int create_persistent_key(SECURITY_DEVICE_INFO* tpm_info, TPM_HANDLE request_handle, TPMI_DH_OBJECT hierarchy, TPM2B_PUBLIC* inPub, TPM2B_PUBLIC* outPub)
{
    int result;
    TPM_RC tpm_result;
    TPM2B_NAME name;
    TPM2B_NAME qName;

    tpm_result = TPM2_ReadPublic(&tpm_info->tpm_device, request_handle, outPub, &name, &qName);
    if (tpm_result == TPM_RC_SUCCESS)
    {
        tpm_info->tpm_handle = request_handle;
        result = 0;
    }
    else if (tpm_result != TPM_RC_HANDLE)
    {
        LogError("Failed calling TPM2_ReadPublic 0%x", tpm_result);
        result = __FAILURE__;
    }
    else
    {
        if (TSS_CreatePrimary(&tpm_info->tpm_device, &NullPwSession, hierarchy, inPub, &tpm_info->tpm_handle, outPub) != TPM_RC_SUCCESS)
        {
            LogError("Failed calling TSS_CreatePrimary");
            result = __FAILURE__;
        }
        else
        {
            if (TPM2_EvictControl(&tpm_info->tpm_device, &NullPwSession, TPM_RH_OWNER, tpm_info->tpm_handle, request_handle) != TPM_RC_SUCCESS)
            {
                LogError("Failed calling TSS_CreatePrimary");
                result = __FAILURE__;
            }
            else if (TPM2_FlushContext(&tpm_info->tpm_device, tpm_info->tpm_handle) != TPM_RC_SUCCESS)
            {
                LogError("Failed calling TSS_CreatePrimary");
                result = __FAILURE__;
            }
            else
            {
                tpm_info->tpm_handle = request_handle;
                result = 0;
            }
        }
    }
    return result;
}

static int initialize_tpm_device(SECURITY_DEVICE_INFO* tpm_info)
{
    int result;
    if (TSS_CreatePwAuthSession(&NullAuth, &NullPwSession) != TPM_RC_SUCCESS)
    {
        LogError("Failure calling TSS_CreatePwAuthSession");
        result = __FAILURE__;
    }
    else if (Initialize_TPM_Codec(&tpm_info->tpm_device) != TPM_RC_SUCCESS)
    {
        LogError("Failure initializeing TPM Codec");
        result = __FAILURE__;
    }
    /* Codes_iothub_security_sas_create shall get a handle to the Endorsement Key and Storage Root Key.*/
    else if (create_persistent_key(tpm_info, TPM_20_EK_HANDLE, TPM_RH_ENDORSEMENT, GetEkTemplate(), &tpm_info->ek_pub) != 0)
    {
        LogError("Failure calling creating persistent key for Endorsement key");
        result = __FAILURE__;
    }
    /* Codes_iothub_security_sas_create shall get a handle to the Endorsement Key and Storage Root Key.*/
    else if (create_persistent_key(tpm_info, TPM_20_SRK_HANDLE, TPM_RH_OWNER, GetSrkTemplate(), &tpm_info->srk_pub) != 0)
    {
        LogError("Failure calling creating persistent key for Storage Root key");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

SECURITY_DEVICE_HANDLE iothub_security_sas_create()
{
    SECURITY_DEVICE_INFO* result;
    /* Codes_SRS_SECURE_DEVICE_TPM_07_002: [ On success iothub_security_sas_create shall allocate a new instance of the secure device tpm interface. ] */
    result = malloc(sizeof(SECURITY_DEVICE_INFO) );
    if (result == NULL)
    {
        /* Codes_SRS_SECURE_DEVICE_TPM_07_001: [ If any failure is encountered iothub_security_sas_create shall return NULL ] */
        LogError("Failure: malloc SECURITY_DEVICE_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(SECURITY_DEVICE_INFO));
        /* Codes_iothub_security_sas_create shall call into the tpm_codec to initialize a TSS session.*/
        if (initialize_tpm_device(result) != 0)
        {
            /* Codes_SRS_SECURE_DEVICE_TPM_07_001: [ If any failure is encountered iothub_security_sas_create shall return NULL ] */
            LogError("Failure initializing tpm device.");
            free(result);
            result = NULL;
        }
    }
    return (SECURITY_DEVICE_HANDLE)result;
}

void iothub_security_sas_destroy(SECURITY_DEVICE_HANDLE handle)
{
    /* Codes_SRS_SECURE_DEVICE_TPM_07_005: [ if handle is NULL, iothub_security_sas_destroy shall do nothing. ] */
    if (handle != NULL)
    {
        /* Codes_SRS_SECURE_DEVICE_TPM_07_004: [ iothub_security_sas_destroy shall free the SECURITY_DEVICE_INFO instance. ] */
        Deinit_TPM_Codec(&handle->tpm_device);
        /* Codes_SRS_SECURE_DEVICE_TPM_07_006: [ iothub_security_sas_destroy shall free all resources allocated in this module. ]*/
        free(handle);
    }
}

int initialize_sas_system(void)
{
    return 0;
}

void deinitialize_sas_system(void)
{
}

BUFFER_HANDLE iothub_security_sas_sign_data(SECURITY_DEVICE_HANDLE handle, const unsigned char* data, size_t data_len)
{
    BUFFER_HANDLE result;
    /* Codes_SRS_SECURE_DEVICE_TPM_07_020: [ If handle or data is NULL or data_len is 0, iothub_security_sas_sign_data shall return NULL. ] */
    if (handle == NULL || data == NULL || data_len == 0)
    {
        LogError("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = NULL;
    }
    else
    {
        BYTE data_signature[1024];
        BYTE* data_copy = (unsigned char*)data;
        /* Codes_SRS_SECURE_DEVICE_TPM_07_021: [ iothub_security_sas_sign_data shall call into the tpm to hash the supplied data value. ] */
        uint32_t sign_len = SignData(&handle->tpm_device, &NullPwSession, data_copy, data_len, data_signature, sizeof(data_signature) );
        if (sign_len == 0)
        {
            /* Codes_SRS_SECURE_DEVICE_TPM_07_023: [ If an error is encountered iothub_security_sas_sign_data shall return NULL. ] */
            LogError("Failure signing data from hash");
            result = NULL;
        }
        else
        {
            /* Codes_SRS_SECURE_DEVICE_TPM_07_022: [ If hashing the data was successful, iothub_security_sas_sign_data shall create a BUFFER_HANDLE with the supplied signed data. ] */
            result = BUFFER_create(data_signature, sign_len);
            if (result == NULL)
            {
                /* Codes_SRS_SECURE_DEVICE_TPM_07_023: [ If an error is encountered iothub_security_sas_sign_data shall return NULL. ] */
                LogError("Failure allocating sign data");
            }
        }
    }
    return result;
}

/* Codes_SRS_SECURE_DEVICE_TPM_07_026: [ iothub_security_sas_interface shall return the SEC_TPM_INTERFACE structure. ] */
const SAS_SECURITY_INTERFACE* iothub_security_sas_interface()
{
    return &sas_security_interface;
}
