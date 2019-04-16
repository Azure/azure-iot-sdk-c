// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tpm_msr.h"
#include "azure_c_shared_utility/envvariable.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_utpm_c/tpm_comm.h"
#include "azure_utpm_c/tpm_codec.h"
#include "azure_utpm_c/TpmTypes.h"

#include "azure_utpm_c/Marshal_fp.h"     // for activation blob unmarshaling

typedef struct TPM_INFO_TAG
{
    TPM_HANDLE tpm_handle;
    TSS_DEVICE tpm_device;
    TPM2B_PUBLIC ek_pub;
    TPM2B_PUBLIC srk_pub;
    TPM2B_PUBLIC id_key_public;
    TPM2B_PRIVATE id_key_dup_blob;
    TPM2B_PRIVATE id_key_priv;
} TPM_INFO;

static const UINT32 TPM_20_SRK_HANDLE = HR_PERSISTENT | 0x00000001;
static const UINT32 TPM_20_EK_HANDLE = HR_PERSISTENT | 0x00010001;
static const UINT32 DPS_ID_KEY_HANDLE = HR_PERSISTENT | 0x00000100;

static TPM2B_AUTH null_auth_token = { 0 };
static TSS_SESSION null_pw_sess;

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
    { 32,
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
    LogError("act_size %d < actSize %d\r\n", act_size, arrSize);   \
    if (act_size < arrSize)                                                     \
    {                                                                           \
        LogError("Unmarshaling " #dstPtr " failed: Need %d bytes, while only %d left", arrSize, act_size);  \
        result = MU_FAILURE;       \
    }                                                                           \
    else                            \
    {                                   \
        dstPtr = curr_pos - sizeof(UINT16);                                         \
        *(UINT16*)dstPtr = (UINT16)arrSize;                                         \
        curr_pos += arrSize;                         \
    }

static int create_tpm_session(TPM_INFO* tpm_info, TSS_SESSION* tpm_session)
{
    int result;
    TPMA_SESSION sess_attrib = { 1 };
    if (TSS_StartAuthSession(&tpm_info->tpm_device, TPM_SE_POLICY, TPM_ALG_SHA256, sess_attrib, tpm_session) != TPM_RC_SUCCESS)
    {
        LogError("Failure: Starting EK policy session");
        result = MU_FAILURE;
    }
    else if (TSS_PolicySecret(&tpm_info->tpm_device, &null_pw_sess, TPM_RH_ENDORSEMENT, tpm_session, NULL, 0) != TPM_RC_SUCCESS)
    {
        LogError("Failure: PolicySecret() for EK");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int insert_key_in_tpm(TPM_INFO* tpm_info, const unsigned char* key, size_t key_len)
{
    int result;
    TSS_SESSION ek_sess = { { TPM_RH_NULL } };
    if (create_tpm_session(tpm_info, &ek_sess) != 0)
    {
        LogError("Failure: Starting EK policy session");
        result = MU_FAILURE;
    }
    else
    {
        TPMT_SYM_DEF_OBJECT Aes128SymDef = { TPM_ALG_AES, 128, TPM_ALG_CFB };
        TPM2B_ID_OBJECT enc_key_blob;
        TPM2B_ENCRYPTED_SECRET tpm_enc_secret;
        TPM2B_PRIVATE id_key_dup_blob;
        TPM2B_ENCRYPTED_SECRET encrypt_wrap_key;
        TPM2B_PUBLIC id_key_Public = { TPM_ALG_NULL };
        UINT16 enc_data_size = 0;
        TPM2B_DIGEST inner_wrap_key = { 0 };
        TPM2B_PRIVATE id_key_priv;
        TPM_HANDLE load_id_key = TPM_ALG_NULL;

        uint8_t* curr_pos = (uint8_t*)key;
        uint32_t act_size = (int32_t)key_len;

        DPS_UNMARSHAL(TPM2B_ID_OBJECT, &enc_key_blob);
        DPS_UNMARSHAL(TPM2B_ENCRYPTED_SECRET, &tpm_enc_secret);
        DPS_UNMARSHAL(TPM2B_PRIVATE, &id_key_dup_blob);
        DPS_UNMARSHAL(TPM2B_ENCRYPTED_SECRET, &encrypt_wrap_key);
        DPS_UNMARSHAL_FLAGGED(TPM2B_PUBLIC, &id_key_Public);

        // The given TPM may support larger TPM2B_MAX_BUFFER than this API headers define.
        // So instead of unmarshaling data in a standalone data structure just reuse the
        // original activation buffer (after updating byte order of the UINT16 counter)
        DPS_UNMARSHAL(UINT16, &enc_data_size);

        if (TPM2_ActivateCredential(&tpm_info->tpm_device, &null_pw_sess, &ek_sess, TPM_20_SRK_HANDLE, TPM_20_EK_HANDLE,
            &enc_key_blob, &tpm_enc_secret, &inner_wrap_key) != TPM_RC_SUCCESS)
        {
            LogError("Failure: TPM2_ActivateCredential");
            result = MU_FAILURE;
        }
        else if (TPM2_Import(&tpm_info->tpm_device, &null_pw_sess, TPM_20_SRK_HANDLE, (TPM2B_DATA*)&inner_wrap_key, &id_key_Public, &id_key_dup_blob, &encrypt_wrap_key, &Aes128SymDef, &id_key_priv) != TPM_RC_SUCCESS)
        {
            LogError("Failure: importing dps Id key");
            result = MU_FAILURE;
        }
        else
        {
            TPM2B_SENSITIVE_CREATE sen_create = { 0 };
            TPM2B_PUBLIC sym_pub = { 0 };
            TPM2B_PRIVATE sym_priv = { 0 };

            static TPM2B_PUBLIC symTemplate = { 0,   // size will be computed during marshaling
            {
                TPM_ALG_SYMCIPHER,              // TPMI_ALG_PUBLIC      type
                TPM_ALG_SHA256,                 // TPMI_ALG_HASH        nameAlg
            { 0 },                          // TPMA_OBJECT  objectAttributes (set below)
            { 0 },                          // TPM2B_DIGEST         authPolicy
            { 0 },                          // TPMU_PUBLIC_PARMS    parameters (set below)
            { 0 }                           // TPMU_PUBLIC_ID       unique
            } };
            symTemplate.publicArea.objectAttributes = ToTpmaObject(Decrypt | FixedTPM | FixedParent | UserWithAuth);
            symTemplate.publicArea.parameters.symDetail.sym.algorithm = TPM_ALG_AES;
            symTemplate.publicArea.parameters.symDetail.sym.keyBits.sym = inner_wrap_key.t.size * 8;
            symTemplate.publicArea.parameters.symDetail.sym.mode.sym = TPM_ALG_CFB;

            memcpy(sen_create.sensitive.data.t.buffer, inner_wrap_key.t.buffer, inner_wrap_key.t.size);
            sen_create.sensitive.data.t.size = inner_wrap_key.t.size;

            if (TSS_Create(&tpm_info->tpm_device, &null_pw_sess, TPM_20_SRK_HANDLE, &sen_create, &symTemplate, &sym_priv, &sym_pub) != TPM_RC_SUCCESS)
            {
                LogError("Failed to inject symmetric key data");
                result = MU_FAILURE;
            }
            else if (TPM2_Load(&tpm_info->tpm_device, &null_pw_sess, TPM_20_SRK_HANDLE, &id_key_priv, &id_key_Public, &load_id_key, NULL) != TPM_RC_SUCCESS)
            {
                LogError("Failed Load Id key.");
                result = MU_FAILURE;
            }
            else
            {
                // Remove old Id key
                (void)TPM2_EvictControl(&tpm_info->tpm_device, &null_pw_sess, TPM_RH_OWNER, DPS_ID_KEY_HANDLE, DPS_ID_KEY_HANDLE);

                if (TPM2_EvictControl(&tpm_info->tpm_device, &null_pw_sess, TPM_RH_OWNER, load_id_key, DPS_ID_KEY_HANDLE) != TPM_RC_SUCCESS)
                {
                    LogError("Failed Load Id key.");
                    result = MU_FAILURE;
                }
                else if (TPM2_FlushContext(&tpm_info->tpm_device, load_id_key) != TPM_RC_SUCCESS)
                {
                    LogError("Failed Load Id key.");
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}

static int initialize_tpm(TPM_INFO* tpm_info)
{
    int result;
    if (TSS_CreatePwAuthSession(&null_auth_token, &null_pw_sess) != TPM_RC_SUCCESS)
    {
        LogError("Failure calling TSS_CreatePwAuthSession");
        result = MU_FAILURE;
    }
    else if (Initialize_TPM_Codec(&tpm_info->tpm_device) != TPM_RC_SUCCESS)
    {
        LogError("Failure initializeing TPM Codec");
        result = MU_FAILURE;
    }
    else if ((TSS_CreatePersistentKey(&tpm_info->tpm_device, TPM_20_EK_HANDLE, &null_pw_sess, TPM_RH_ENDORSEMENT, GetEkTemplate(), &tpm_info->ek_pub)) == 0)
    {
        LogError("Failure calling creating persistent key for Endorsement key");
        result = MU_FAILURE;
    }
    else if (TSS_CreatePersistentKey(&tpm_info->tpm_device, TPM_20_SRK_HANDLE, &null_pw_sess, TPM_RH_OWNER, GetSrkTemplate(), &tpm_info->srk_pub) == 0)
    {
        LogError("Failure calling creating persistent key for Storage Root key");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

TPM_INFO_HANDLE tpm_msr_create()
{
    TPM_INFO* result;
    result = (TPM_INFO*)malloc(sizeof(TPM_INFO));
    if (result == NULL)
    {
        LogError("Failure: malloc TPM_INFO.");
    }
    else
    {
        memset(result, 0, sizeof(TPM_INFO));

        result->tpm_device.comms_endpoint = environment_get_variable("IOT_DPS_TPM_SIMULATOR_IP_ADDRESS");
        if (result->tpm_device.comms_endpoint == NULL)
        {
            LogError("Failure retrieving endpoint address");
        }
        else if (initialize_tpm(result) != 0)
        {
            LogError("Failure initializing tpm device.");
            free(result);
            result = NULL;
        }
    }
    return result;

}

void tpm_msr_destroy(TPM_INFO_HANDLE handle)
{
    if (handle != NULL)
    {
        Deinit_TPM_Codec(&handle->tpm_device);
        free(handle);
    }
}

int tpm_msr_get_ek(TPM_INFO_HANDLE handle, unsigned char* data_pos, size_t* key_len)
{
    int result;
    if (handle == NULL || key_len == 0)
    {
        LogError("Failure with parameter.");
        result = __LINE__;
    }
    else
    {
        if (handle->ek_pub.publicArea.unique.rsa.t.size == 0)
        {
            LogError("Endorsement key is invalid.");
            result = __LINE__;
        }
        else
        {
            INT32 len = (INT32)*key_len;
            *key_len = (size_t)TPM2B_PUBLIC_Marshal(&handle->ek_pub, &data_pos, &len);
            result = 0;
        }
    }
    return result;
}

int tpm_msr_get_srk(TPM_INFO_HANDLE handle, unsigned char* data_pos, size_t* key_len)
{
    int result;
    if (handle == NULL || key_len == 0)
    {
        LogError("Failure with parameter.");
        result = __LINE__;
    }
    else
    {
        if (handle->srk_pub.publicArea.unique.rsa.t.size == 0)
        {
            LogError("Endorsement key is invalid.");
            result = __LINE__;
        }
        else
        {
            INT32 len = (INT32)*key_len;
            *key_len = (size_t)TPM2B_PUBLIC_Marshal(&handle->srk_pub, &data_pos, &len);
            result = 0;
        }
    }
    return result;
}

int tpm_msr_import_key(TPM_INFO_HANDLE handle, const unsigned char* key, size_t key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == 0)
    {
        LogError("Failure with parameter.");
        result = __LINE__;
    }
    else
    {
        if (insert_key_in_tpm(handle, key, key_len))
        {
            LogError("Failure with parameter.");
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

int tpm_msr_sign_data(TPM_INFO_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char* data_signature, size_t* signed_len)
{
    int result;
    if (handle == NULL || data == NULL || data_len == 0 || data_signature == NULL || signed_len == NULL)
    {
        LogError("Failure with parameter.");
        result = __LINE__;
    }
    else
    {
        BYTE* data_copy = (unsigned char*)data;

        /* Codes_SRS_HSM_CLIENT_TPM_07_021: [ hsm_client_tpm_sign_data shall call into the tpm to hash the supplied data value. ] */
        *signed_len = (size_t)SignData(&handle->tpm_device, &null_pw_sess, data_copy, (UINT32)data_len, data_signature, (INT32)*signed_len);
        if (*signed_len == 0)
        {
            /* Codes_SRS_HSM_CLIENT_TPM_07_023: [ If an error is encountered hsm_client_tpm_sign_data shall return NULL. ] */
            LogError("Failure signing data from hash");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}