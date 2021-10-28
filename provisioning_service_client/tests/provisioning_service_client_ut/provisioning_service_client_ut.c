// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

static void* real_malloc(size_t size)
{
    return malloc(size);
}

static void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "azure_uhttp_c/uhttp.h"

#include "prov_service_client/provisioning_sc_enrollment.h"
#include "prov_service_client/provisioning_sc_models_serializer.h"
#include "prov_service_client/provisioning_sc_shared_helpers.h"

#undef ENABLE_MOCKS

#include "prov_service_client/provisioning_service_client.h"
#include "prov_service_client/provisioning_sc_json_const.h"

typedef enum {RESPONSE_ON, RESPONSE_OFF} response_switch;
typedef enum {CERT, NO_CERT} cert_flag;
typedef enum {TRACE, NO_TRACE} trace_flag;
typedef enum { PROXY, NO_PROXY } proxy_flag;

static TEST_MUTEX_HANDLE g_testByTest;

static int g_uhttp_client_dowork_call_count;
static ON_HTTP_OPEN_COMPLETE_CALLBACK g_on_http_open;
static void* g_http_open_ctx;
static ON_HTTP_REQUEST_CALLBACK g_on_http_reply_recv;
static void* g_http_reply_recv_ctx;

static response_switch g_response_content_status;

static cert_flag g_cert;
static trace_flag g_trace;
static proxy_flag g_proxy;


#ifdef __cplusplus
extern "C"
{
#endif

    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...);
    STRING_HANDLE STRING_construct_sprintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#ifdef __cplusplus
extern "C"
{
#endif
    int STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        (void)handle;
        (void)format;
        return 0;
    }

    STRING_HANDLE STRING_construct_sprintf(const char* format, ...)
    {
        (void)format;
        return (STRING_HANDLE)real_malloc(1);
    }
#ifdef __cplusplus
}
#endif

//Control Parameters
#define IOTHUBHOSTNAME "HostName"
#define IOTHUBSHAREDACESSKEYNAME "SharedAccessKeyName"
#define IOTHUBSHAREDACESSKEY "SharedAccessKey"

static const MAP_HANDLE TEST_MAP_HANDLE = (MAP_HANDLE)0x11111111;
static const IO_INTERFACE_DESCRIPTION* TEST_INTERFACE_DESC = (IO_INTERFACE_DESCRIPTION*)0x11111112;
static const HTTP_CLIENT_HANDLE TEST_HTTP_CLIENT_HANDLE = (HTTP_CLIENT_HANDLE)0x11111113;
#define TEST_INDIVIDUAL_ENROLLMENT_HANDLE (INDIVIDUAL_ENROLLMENT_HANDLE)0x11111114
static ATTESTATION_MECHANISM_HANDLE TEST_ATT_MECH_HANDLE = (ATTESTATION_MECHANISM_HANDLE)0x11111115;
static PROVISIONING_SERVICE_CLIENT_HANDLE TEST_PROVISIONING_SC_HANDLE = (PROVISIONING_SERVICE_CLIENT_HANDLE)0x11111116;
static ENROLLMENT_GROUP_HANDLE TEST_ENROLLMENT_GROUP_HANDLE = (ENROLLMENT_GROUP_HANDLE)0x11111117;
static IO_INTERFACE_DESCRIPTION* TEST_IO_INTERFACE_DESC = (IO_INTERFACE_DESCRIPTION*)0x11111118;
static STRING_HANDLE TEST_STRING_HANDLE = (STRING_HANDLE)0x11111119;
static DEVICE_REGISTRATION_STATE_HANDLE TEST_DEVICE_REGISTRATION_STATE_HANDLE = (DEVICE_REGISTRATION_STATE_HANDLE)0x11111120;
#define TEST_INDIVIDUAL_ENROLLMENT_HANDLE2 (INDIVIDUAL_ENROLLMENT_HANDLE)0x11111121
#define TEST_HTTP_HEADERS_HANDLE (HTTP_HEADERS_HANDLE)0x11111122
static const unsigned char* TEST_REPLY_JSON = (const unsigned char*)"{my-json-reply}";
static const char* TEST_ENROLLMENT_JSON = "{my-json-serialized-enrollment}";
static const char* TEST_CONNECTION_STRING = "my-connection-string";
static const char* TEST_STRING = "my-string";
static const char* TEST_EK = "my-ek";
static const char* TEST_REGID = "my-regid";
static const char* TEST_GROUPID = "my-groupid";
static const char* TEST_ETAG = "my-etag";
static const char* TEST_ETAG_STAR = "*";
static const char* TEST_HOSTNAME = "my-hostname";
static const char* TEST_SHARED_ACCESS_KEY = "my-shared-access-key";
static const char* TEST_SHARED_ACCESS_KEY_NAME = "my-shared-access-key-name";
static const char* TEST_TRUSTED_CERT = "my-trusted-cert";
static const char* TEST_PROXY_HOSTNAME = "my-proxy-hostname";
static const char* TEST_PROXY_USERNAME = "my-username";
static const char* TEST_PROXY_PASSWORD = "my-password";
static const char* TEST_QUERY_STRING = "*";
static const char* TEST_CONT_TOKEN = "cont";
static const char* TEST_CONT_TOKEN2 = "cont2";
static int TEST_PROXY_PORT = 123;
static size_t TEST_REPLY_JSON_LEN = 15;
static unsigned int STATUS_CODE_SUCCESS = 204;

typedef enum {ETAG, NO_ETAG} etag_flag;
typedef enum {RESPONSE, NO_RESPONSE} response_flag;


static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static HTTP_CLIENT_HANDLE my_uhttp_client_create(const IO_INTERFACE_DESCRIPTION* io_interface_desc, const void* xio_param, ON_HTTP_ERROR_CALLBACK on_http_error, void* callback_ctx)
{
    (void)io_interface_desc;
    (void)xio_param;
    (void)on_http_error;
    (void)callback_ctx;

    return (HTTP_CLIENT_HANDLE)real_malloc(1);
}

static void my_uhttp_client_destroy(HTTP_CLIENT_HANDLE handle)
{
    real_free(handle);
}

static HTTP_CLIENT_RESULT my_uhttp_client_open(HTTP_CLIENT_HANDLE handle, const char* host, int port_num, ON_HTTP_OPEN_COMPLETE_CALLBACK on_connect, void* callback_ctx)
{
    (void)handle;
    (void)host;
    (void)port_num;
    g_on_http_open = on_connect;
    g_http_open_ctx = callback_ctx; //prov_client

    //note that a real malloc does occur in this fn, but it can't be mocked since it's in a field of handle

    return HTTP_CLIENT_OK;
}

static void my_uhttp_client_close(HTTP_CLIENT_HANDLE handle, ON_HTTP_CLOSED_CALLBACK on_close_callback, void* callback_ctx)
{
    (void)handle;
    (void)on_close_callback;
    (void)callback_ctx;

    //note that a real free does occur in this fn, but it can't be mocked since it's in a field of handle
}

static HTTP_CLIENT_RESULT my_uhttp_client_execute_request(HTTP_CLIENT_HANDLE handle, HTTP_CLIENT_REQUEST_TYPE request_type, const char* relative_path,
    HTTP_HEADERS_HANDLE http_header_handle, const unsigned char* content, size_t content_len, ON_HTTP_REQUEST_CALLBACK on_request_callback, void* callback_ctx)
{
    (void)handle;
    (void)request_type;
    (void)relative_path;
    (void)http_header_handle;
    (void)content;
    (void)content_len;
    g_on_http_reply_recv = on_request_callback;
    g_http_reply_recv_ctx = callback_ctx;

    return HTTP_CLIENT_OK;
}

static void my_uhttp_client_dowork(HTTP_CLIENT_HANDLE handle)
{
    (void)handle;

    unsigned char* content;
    if (g_response_content_status == RESPONSE_ON)
        content = (unsigned char*)TEST_REPLY_JSON;
    else
        content = NULL;

    if (g_uhttp_client_dowork_call_count == 0)
        g_on_http_open(g_http_open_ctx, HTTP_CALLBACK_REASON_OK);
    else if (g_uhttp_client_dowork_call_count == 1)
    {
        g_on_http_reply_recv(g_http_reply_recv_ctx, HTTP_CALLBACK_REASON_OK, content, 1, STATUS_CODE_SUCCESS, TEST_HTTP_HEADERS_HANDLE);
    }
    g_uhttp_client_dowork_call_count++;
}

static const char* my_Map_GetValueFromKey(MAP_HANDLE handle, const char* key)
{
    char* result = NULL;
    (void)handle;

    if (strcmp(key, IOTHUBHOSTNAME) == 0)
        result = (char*)TEST_HOSTNAME;
    else if (strcmp(key, IOTHUBSHAREDACESSKEY) == 0)
        result = (char*)TEST_SHARED_ACCESS_KEY;
    else if (strcmp(key, IOTHUBSHAREDACESSKEYNAME) == 0)
        result = (char*)TEST_SHARED_ACCESS_KEY_NAME;

    return result;
}

static int my_mallocAndStrcpy_s(char** destination, const char* source)
{
    (void)source;
    size_t src_len = strlen(source);
    *destination = (char*)real_malloc(src_len + 1);
    strcpy(*destination, source);
    return 0;
}

static int my_mallocAndStrcpy_overwrite(char** destination, const char* source)
{
    int result = 0;
    char* temp = NULL;

    if (destination == NULL || source == NULL)
    {
        LogError("Invalid input");
        result = MU_FAILURE;
    }
    else if (my_mallocAndStrcpy_s(&temp, source) != 0)
    {
        LogError("Failed to copy value from source");
        result = MU_FAILURE;
    }
    else
    {
        real_free(*destination);
        *destination = temp;
    }

    return result;
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)real_malloc(1);
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Clone(HTTP_HEADERS_HANDLE handle)
{
    (void)handle;
    return (HTTP_HEADERS_HANDLE)real_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
    real_free(handle);
}

static STRING_HANDLE my_STRING_construct(const char* psz)
{
    (void)psz;
    return (STRING_HANDLE)real_malloc(1);
}

static void my_STRING_delete(STRING_HANDLE handle)
{
    real_free(handle);
}

static STRING_HANDLE my_SASToken_CreateString(const char* key, const char* scope, const char* keyName, uint64_t expiry) {
    (void)key;
    (void)scope;
    (void)keyName;
    (void)expiry;
    return (STRING_HANDLE)real_malloc(1);
}

static STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
    (void)textEncode;
    return (STRING_HANDLE)real_malloc(1);
}

static INDIVIDUAL_ENROLLMENT_HANDLE my_individualEnrollment_deserializeFromJson(const char* json_string)
{
    INDIVIDUAL_ENROLLMENT_HANDLE result;
    if (json_string != NULL)
        result = (INDIVIDUAL_ENROLLMENT_HANDLE)real_malloc(1);
    else
        result = NULL;
    return result;
}

static ENROLLMENT_GROUP_HANDLE my_enrollmentGroup_deserializeFromJson(const char* json_string)
{
    ENROLLMENT_GROUP_HANDLE result;
    if (json_string != NULL)
        result = (ENROLLMENT_GROUP_HANDLE)real_malloc(1);
    else
        result = NULL;
    return result;
}

static DEVICE_REGISTRATION_STATE_HANDLE my_deviceRegistrationState_deserializeFromJson(const char* json_string)
{
    DEVICE_REGISTRATION_STATE_HANDLE result;
    if (json_string != NULL)
        result = (DEVICE_REGISTRATION_STATE_HANDLE)real_malloc(1);
    else
        result = NULL;
    return result;
}

static PROVISIONING_BULK_OPERATION_RESULT* my_bulkOperationResult_deserializeFromJson(const char* json_string)
{
    PROVISIONING_BULK_OPERATION_RESULT* result;
    if (json_string != NULL)
        result = (PROVISIONING_BULK_OPERATION_RESULT*)real_malloc(1);
    else
        result = NULL;
    return result;
}

static PROVISIONING_QUERY_RESPONSE* my_queryResponse_deserializeFromJson(const char* json_string, PROVISIONING_QUERY_TYPE type)
{
    (void)type;
    PROVISIONING_QUERY_RESPONSE* result;
    if (json_string != NULL)
        result = (PROVISIONING_QUERY_RESPONSE*)real_malloc(1);
    else
        result = NULL;
    return result;
}

static void my_individualEnrollment_destroy(INDIVIDUAL_ENROLLMENT_HANDLE handle)
{
    real_free(handle);
}

static void my_enrollmentGroup_destroy(ENROLLMENT_GROUP_HANDLE handle)
{
    real_free(handle);
}

static void my_deviceRegistrationState_destroy(DEVICE_REGISTRATION_STATE_HANDLE handle)
{
    real_free(handle);
}

static void my_bulkOperationResult_free(PROVISIONING_BULK_OPERATION_RESULT* bulk_res)
{
    real_free(bulk_res);
}

static void my_queryResponse_free(PROVISIONING_QUERY_RESPONSE* query_resp)
{
    real_free(query_resp);
}

static PROVISIONING_QUERY_TYPE my_queryType_stringToEnum(const char* string)
{
    PROVISIONING_QUERY_TYPE result;

    if (string == NULL)
    {
        result = QUERY_TYPE_INVALID;
    }
    else if (strcmp(string, QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT) == 0)
    {
        result = QUERY_TYPE_INDIVIDUAL_ENROLLMENT;
    }
    else if (strcmp(string, QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP) == 0)
    {
        result = QUERY_TYPE_ENROLLMENT_GROUP;
    }
    else if (strcmp(string, QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE) == 0)
    {
        result = QUERY_TYPE_DEVICE_REGISTRATION_STATE;
    }
    else
    {
        result = QUERY_TYPE_INVALID;
    }

    return result;
}

static INDIVIDUAL_ENROLLMENT_HANDLE my_individualEnrollment_create(const char* reg_id, ATTESTATION_MECHANISM_HANDLE att_handle)
{
    (void)reg_id;
    (void)att_handle;
    return (INDIVIDUAL_ENROLLMENT_HANDLE)real_malloc(1);
}

static ENROLLMENT_GROUP_HANDLE my_enrollmentGroup_create(const char* group_id, ATTESTATION_MECHANISM_HANDLE att_handle)
{
    (void)group_id;
    (void)att_handle;
    return (ENROLLMENT_GROUP_HANDLE)real_malloc(1);
}

static const char* my_individualEnrollment_getRegistrationId(INDIVIDUAL_ENROLLMENT_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_REGID;
    return result;
}

static const char* my_enrollmentGroup_getGroupId(ENROLLMENT_GROUP_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_GROUPID;
    return result;
}

static const char* my_individualEnrollment_getEtag(INDIVIDUAL_ENROLLMENT_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_ETAG;
    return result;
}

static const char* my_enrollmentGroup_getEtag(ENROLLMENT_GROUP_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_ETAG;
    return result;
}

static const char* my_deviceRegistrationState_getRegistrationId(DEVICE_REGISTRATION_STATE_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_REGID;
    return result;
}

static const char* my_deviceRegistrationState_getEtag(DEVICE_REGISTRATION_STATE_HANDLE handle)
{
    const char* result = NULL;
    if (handle != NULL)
        result = TEST_ETAG;
    return result;
}

static char* my_individualEnrollment_serializeToJson(INDIVIDUAL_ENROLLMENT_HANDLE handle)
{
    (void)handle;
    char* result = NULL;
    size_t len = strlen(TEST_ENROLLMENT_JSON);
    result = (char*)real_malloc(len + 1);
    strncpy(result, TEST_ENROLLMENT_JSON, len + 1);
    return result;
}

static char* my_enrollmentGroup_serializeToJson(ENROLLMENT_GROUP_HANDLE handle)
{
    (void)handle;
    char* result = NULL;
    size_t len = strlen(TEST_ENROLLMENT_JSON);
    result = (char*)real_malloc(len + 1);
    strncpy(result, TEST_ENROLLMENT_JSON, len + 1);
    return result;
}

static char* my_bulkOperation_serializeToJson(const PROVISIONING_BULK_OPERATION* bulkop)
{
    (void)bulkop;
    char* result = NULL;
    size_t len = strlen(TEST_ENROLLMENT_JSON);
    result = (char*)real_malloc(len + 1);
    strncpy(result, TEST_ENROLLMENT_JSON, len + 1);
    return result;
}

static char* my_querySpecification_serializeToJson(const PROVISIONING_QUERY_SPECIFICATION* query_spec)
{
    (void)query_spec;
    char* result = NULL;
    size_t len = strlen(TEST_ENROLLMENT_JSON);
    result = (char*)real_malloc(len + 1);
    strncpy(result, TEST_ENROLLMENT_JSON, len + 1);
    return result;
}

static void register_global_mock_hooks()
{
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, real_free);

    REGISTER_GLOBAL_MOCK_HOOK(Map_GetValueFromKey, my_Map_GetValueFromKey);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetValueFromKey, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, my_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_overwrite, my_mallocAndStrcpy_overwrite);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_overwrite, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_construct, my_STRING_construct);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(STRING_delete, my_STRING_delete);

    REGISTER_GLOBAL_MOCK_HOOK(SASToken_CreateString, my_SASToken_CreateString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(SASToken_CreateString, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_dowork, my_uhttp_client_dowork);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_create, my_uhttp_client_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_open, my_uhttp_client_open);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_open, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_close, my_uhttp_client_close);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_destroy, my_uhttp_client_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(uhttp_client_execute_request, my_uhttp_client_execute_request);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_execute_request, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Clone, my_HTTPHeaders_Clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Clone, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_deserializeFromJson, my_individualEnrollment_deserializeFromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(individualEnrollment_deserializeFromJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_create, my_individualEnrollment_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(individualEnrollment_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_destroy, my_individualEnrollment_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_getRegistrationId, my_individualEnrollment_getRegistrationId);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(individualEnrollment_getRegistrationId, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_getEtag, my_individualEnrollment_getEtag);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(individualEnrollment_getEtag, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_deserializeFromJson, my_enrollmentGroup_deserializeFromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(enrollmentGroup_deserializeFromJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_create, my_enrollmentGroup_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(enrollmentGroup_create, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_destroy, my_enrollmentGroup_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_getGroupId, my_enrollmentGroup_getGroupId);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(enrollmentGroup_getGroupId, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_getEtag, my_enrollmentGroup_getEtag);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(enrollmentGroup_getEtag, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(deviceRegistrationState_destroy, my_deviceRegistrationState_destroy);

    REGISTER_GLOBAL_MOCK_HOOK(deviceRegistrationState_getRegistrationId, my_deviceRegistrationState_getRegistrationId);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceRegistrationState_getRegistrationId, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(deviceRegistrationState_getEtag, my_deviceRegistrationState_getEtag);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceRegistrationState_getEtag, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(individualEnrollment_serializeToJson, my_individualEnrollment_serializeToJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(individualEnrollment_serializeToJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(enrollmentGroup_serializeToJson, my_enrollmentGroup_serializeToJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(enrollmentGroup_serializeToJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(bulkOperation_serializeToJson, my_bulkOperation_serializeToJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(bulkOperation_serializeToJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(deviceRegistrationState_deserializeFromJson, my_deviceRegistrationState_deserializeFromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(deviceRegistrationState_deserializeFromJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(bulkOperationResult_deserializeFromJson, my_bulkOperationResult_deserializeFromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(bulkOperationResult_deserializeFromJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(bulkOperationResult_free, my_bulkOperationResult_free);

    REGISTER_GLOBAL_MOCK_HOOK(querySpecification_serializeToJson, my_querySpecification_serializeToJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(querySpecification_serializeToJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(queryResponse_deserializeFromJson, my_queryResponse_deserializeFromJson);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(queryResponse_deserializeFromJson, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(queryResponse_free, my_queryResponse_free);

    REGISTER_GLOBAL_MOCK_HOOK(queryType_stringToEnum, my_queryType_stringToEnum);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(queryType_stringToEnum, QUERY_TYPE_INVALID);
}

static void register_global_mock_returns()
{
    REGISTER_GLOBAL_MOCK_RETURN(connectionstringparser_parse, TEST_MAP_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connectionstringparser_parse, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(platform_get_default_tlsio, TEST_INTERFACE_DESC);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(platform_get_default_tlsio, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trusted_cert, HTTP_CLIENT_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(uhttp_client_set_trace, HTTP_CLIENT_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(uhttp_client_set_trace, HTTP_CLIENT_INVALID_ARG);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_construct, TEST_STRING_HANDLE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(STRING_c_str, TEST_STRING);

    REGISTER_GLOBAL_MOCK_RETURN(http_proxy_io_get_interface_description, TEST_IO_INTERFACE_DESC);
}

static void register_global_mock_alias_types()
{
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ATTESTATION_MECHANISM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(INDIVIDUAL_ENROLLMENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ENROLLMENT_GROUP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DEVICE_REGISTRATION_STATE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const INDIVIDUAL_ENROLLMENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const ENROLLMENT_GROUP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const DEVICE_REGISTRATION_STATE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PROVISIONING_QUERY_SPECIFICATION*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_ERROR_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_OPEN_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_REQUEST_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HTTP_CLOSED_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_CLIENT_REQUEST_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(PROVISIONING_QUERY_TYPE, int);
}

BEGIN_TEST_SUITE(provisioning_service_client_ut)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);
    umocktypes_bool_register_types();

    int result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types");

    register_global_mock_alias_types();
    register_global_mock_hooks();
    register_global_mock_returns();

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    g_on_http_open = NULL;
    g_http_open_ctx = NULL;
    g_on_http_reply_recv = NULL;
    g_http_reply_recv_ctx = NULL;
    g_uhttp_client_dowork_call_count = 0;
    g_response_content_status = RESPONSE_ON;

    g_cert = NO_CERT;
    g_trace = NO_TRACE;
    g_proxy = NO_PROXY;

    umock_c_negative_tests_deinit();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = MU_FAILURE;
            break;
        }
    }
    return result;
}

static void set_response_status(response_switch response)
{
    g_response_content_status = response;
}

static void expected_calls_mallocAndStrcpy_overwrite()
{
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

static void expected_calls_construct_registration_path(bool has_id)
{
    if (has_id)
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
        //STRICT_EXPECTED_CALL(STRING_construct_sprintf(IGNORED_PTR_ARG, IGNORED_PTR_ARG)); <---- Call cannot be mocked
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    }
    else
    {
        STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    }
    //STRICT_EXPECTED_CALL(STRING_sprintf(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); <---- Call cannot be mocked

}

static void expected_calls_construct_http_headers(etag_flag etag_flag, HTTP_CLIENT_REQUEST_TYPE request)
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(get_time(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(SASToken_CreateString(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (request != HTTP_CLIENT_REQUEST_DELETE)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (etag_flag == ETAG)
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
}

static void expected_calls_add_query_headers(bool has_page_size, bool has_cont_token)
{
    if (has_cont_token)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
    if (has_page_size)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    }
}

static void expected_calls_connect_to_service()
{
    if (g_proxy == PROXY)
        STRICT_EXPECTED_CALL(http_proxy_io_get_interface_description()); //does not fail
    STRICT_EXPECTED_CALL(platform_get_default_tlsio());
    STRICT_EXPECTED_CALL(uhttp_client_create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (g_cert == CERT)
        STRICT_EXPECTED_CALL(uhttp_client_set_trusted_cert(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    if (g_trace == TRACE)
        STRICT_EXPECTED_CALL(uhttp_client_set_trace(IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_open(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
}

static void expected_calls_rest_call(HTTP_CLIENT_REQUEST_TYPE request_type, response_flag response_flag)
{
    expected_calls_connect_to_service();
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(uhttp_client_execute_request(IGNORED_PTR_ARG, request_type, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(uhttp_client_dowork(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG)); //this is in a callback for on_http_reply_recv
    if (response_flag == RESPONSE)
    {
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));  //this is also in the callback
    }
    STRICT_EXPECTED_CALL(uhttp_client_close(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(uhttp_client_destroy(IGNORED_PTR_ARG)); //does not fail
}

/* UNIT TESTS BEGIN */

TEST_FUNCTION(prov_sc_create_from_connection_string_ERROR_INPUT_NULL)
{
    //arrange

    //act
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(NULL);

    //assert
    ASSERT_IS_NULL(sc);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_from_connection_string_GOLDEN)
{
    //arrange
    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(connectionstringparser_parse(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    //assert
    ASSERT_IS_NOT_NULL(sc);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_from_connection_string_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    STRICT_EXPECTED_CALL(STRING_construct(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(connectionstringparser_parse(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_GetValueFromKey(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 9, 10 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_from_connection_string failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

        //assert
        ASSERT_IS_NULL(sc, tmp_msg);

        //cleanup
        prov_sc_destroy(sc);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_destroy_INPUT_NULL)
{
    //arrange

    //act
    prov_sc_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_destroy_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    prov_sc_destroy(sc);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(prov_sc_set_trace_INPUT_NULL)
{
    //arrange

    //act
    prov_sc_set_trace(NULL, TRACING_STATUS_ON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    //cleanup

}

TEST_FUNCTION(prov_sc_trace_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    prov_sc_set_trace(sc, TRACING_STATUS_ON);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    //can't really test that the value was set since it's abstracted behind a handle, so this is a pretty useless test for now

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_certificate_ERROR_NULL_HANDLE)
{
    //arrange

    //act
    int result = prov_sc_set_certificate(NULL, TEST_TRUSTED_CERT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
}

TEST_FUNCTION(prov_sc_set_certificate_NULL_CERTIFICATE)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    //act
    int result = prov_sc_set_certificate(sc, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_certificate_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_overwrite(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));

    //act
    int result = prov_sc_set_certificate(sc, TEST_TRUSTED_CERT);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_certificate_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mallocAndStrcpy_overwrite(IGNORED_PTR_ARG, TEST_TRUSTED_CERT));

    umock_c_negative_tests_snapshot();

    //size_t calls_cannot_fail[] = { };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = 0; //sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, NULL, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_set_certificate failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_set_certificate(sc, TEST_TRUSTED_CERT);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);
    }

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_ERROR_INPUT_NULL1)
{
    //arrange
    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = TEST_PROXY_PASSWORD;

    //act
    int result = prov_sc_set_proxy(NULL, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
}

TEST_FUNCTION(prov_sc_set_proxy_ERROR_INPUT_NULL2)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_ERROR_NO_HOST)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = NULL;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = TEST_PROXY_PASSWORD;

    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_ERROR_ONLY_PASSWORD)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = NULL;
    proxy_options.password = TEST_PROXY_PASSWORD;

    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_ERROR_ONLY_USERNAME)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = NULL;

    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_GOLDEN_NO_LOGIN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = NULL;
    proxy_options.password = NULL;

    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_set_proxy_GOLDEN_FULL_PROXY)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = TEST_PROXY_PASSWORD;

    umock_c_reset_all_calls();

    //act
    int result = prov_sc_set_proxy(sc, &proxy_options);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_ERROR_INPUT_NULL_SC_HANDLE)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_individual_enrollment(NULL, &ie);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_ERROR_INPUT_NULL_IE_HANDLE)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_individual_enrollment(sc, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_ERROR_INPUT_NULL_IE_HANDLE2)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = NULL;
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_individual_enrollment(sc, &ie);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    INDIVIDUAL_ENROLLMENT_HANDLE old_ie = ie;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_IS_TRUE(old_ie != ie); //ie was changed by the function
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_GOLDEN_w_etag)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    INDIVIDUAL_ENROLLMENT_HANDLE old_ie = ie;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG));
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_IS_TRUE(old_ie != ie); //ie was changed by the function
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_GOLDEN_ALL_HTTP_OPTIONS)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = TEST_PROXY_PASSWORD;

    prov_sc_set_proxy(sc, &proxy_options);
    prov_sc_set_certificate(sc, TEST_TRUSTED_CERT);
    prov_sc_set_trace(sc, TRACING_STATUS_ON);

    g_proxy = PROXY;
    g_cert = CERT;
    g_trace = TRACE;

    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    INDIVIDUAL_ENROLLMENT_HANDLE old_ie = ie;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_IS_TRUE(old_ie != ie); //ie was changed by the function
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_FAIL_null_etag)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4, 5, 7, 12, 14, 15, 19, 20, 21, 22, 24, 25, 27, 28, 29, 30, 31 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_or_update_individual_enrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_FAIL_w_etag)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4, 5, 7, 12, 15, 16, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_or_update_individual_enrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_create_or_update_individual_enrollment_FAIL_ALL_HTTP_OPTIONS)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    HTTP_PROXY_OPTIONS proxy_options;
    proxy_options.host_address = TEST_PROXY_HOSTNAME;
    proxy_options.port = TEST_PROXY_PORT;
    proxy_options.username = TEST_PROXY_USERNAME;
    proxy_options.password = TEST_PROXY_PASSWORD;

    prov_sc_set_proxy(sc, &proxy_options);
    prov_sc_set_certificate(sc, TEST_TRUSTED_CERT);
    prov_sc_set_trace(sc, TRACING_STATUS_ON);

    g_proxy = PROXY;
    g_cert = CERT;
    g_trace = TRACE;

    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_destroy(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4, 5,  7, 12, 14, 15, 16, 22, 24, 25, 27, 28, 30, 31, 32, 33, 34 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_or_update_individual_enrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_create_or_update_individual_enrollment(sc, &ie);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_individual_enrollment(NULL, TEST_INDIVIDUAL_ENROLLMENT_HANDLE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_individual_enrollment(sc, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //no etag
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_individual_enrollment(sc, ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_individual_enrollment(sc, ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = individualEnrollment_create(TEST_REGID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(individualEnrollment_getEtag(IGNORED_PTR_ARG)); //can fail, but for sake of this example, cannot
    STRICT_EXPECTED_CALL(individualEnrollment_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 3, 4, 6, 10, 13, 14, 18, 20, 21, 22, 23, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_individual_enrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_individual_enrollment(sc, ie);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_by_param_NULL_PROV_SC)
{
    //arrange

    //act
    int res = prov_sc_delete_individual_enrollment_by_param(NULL, TEST_REGID, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_by_param_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_delete_individual_enrollment_by_param(sc, NULL, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_by_param_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE); // <- this NO_ETAG flag is what proves Requirement 047 - if etag wasn't ignored, actual calls will not line up with expected
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_individual_enrollment_by_param(sc, TEST_REGID, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_by_param_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_individual_enrollment_by_param(sc, TEST_REGID, TEST_ETAG_STAR);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_individual_enrollment_by_param_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 8, 11, 12, 16, 18, 19, 20, 21, 22, 23, 24, 25 };

    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_individual_enrollment_by_param failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_individual_enrollment_by_param(sc, TEST_REGID, TEST_ETAG_STAR);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_get_individual_enrollment_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie;

    //act
    int res = prov_sc_get_individual_enrollment(NULL, TEST_REGID, &ie);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_get_individual_enrollment_ERROR_INPUT_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_individual_enrollment(sc, NULL, &ie);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_individual_enrollment_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_individual_enrollment(sc, TEST_REGID, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_individual_enrollment_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_get_individual_enrollment(sc, TEST_REGID, &ie);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(ie);

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
}

TEST_FUNCTION(prov_sc_get_individual_enrollment_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(individualEnrollment_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 9, 11, 12, 16, 18, 19, 21, 22, 24, 25, 26, 27};
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_get_individual_enrollment failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_get_individual_enrollment(sc, TEST_REGID, &ie);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    individualEnrollment_destroy(ie);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_ERROR_INPUT_NULL_SC_HANDLE)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_enrollment_group(NULL, &eg);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_ERROR_INPUT_NULL_EG_HANDLE)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_enrollment_group(sc, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_ERROR_INPUT_NULL_EG_HANDLE2)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = NULL;
    umock_c_reset_all_calls();

    //act
    int result = prov_sc_create_or_update_enrollment_group(sc, &eg);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_GOLDEN_no_etag)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    ENROLLMENT_GROUP_HANDLE old_eg = eg;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_create_or_update_enrollment_group(sc, &eg);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_IS_TRUE(old_eg != eg); //eg was changed by the function
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_GOLDEN_w_etag)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    ENROLLMENT_GROUP_HANDLE old_eg = eg;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_create_or_update_enrollment_group(sc, &eg);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_IS_TRUE(old_eg != eg); //eg was changed by the function
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_FAIL_no_etag)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4, 5, 7, 12, 14, 15, 19, 20, 21, 22, 24, 25, 27, 28, 29, 30, 31, 32 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_or_update_enrollment_group failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_create_or_update_enrollment_group(sc, &eg);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_create_or_update_enrollment_group_FAIL_w_etag)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_serializeToJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_PUT);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_PUT, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_destroy(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 4, 5, 7, 12, 15, 16, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32, 33 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_create_or_update_enrollment_group failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_create_or_update_enrollment_group(sc, &eg);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_enrollment_group(NULL, TEST_ENROLLMENT_GROUP_HANDLE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_enrollment_group(sc, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //no etag
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_enrollment_group(sc, eg);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_enrollment_group(sc, eg);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = enrollmentGroup_create(TEST_GROUPID, TEST_ATT_MECH_HANDLE);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(enrollmentGroup_getEtag(IGNORED_PTR_ARG)); //can fail, but for sake of this example, cannot
    STRICT_EXPECTED_CALL(enrollmentGroup_getGroupId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 3, 4, 6, 10, 13, 14, 18, 20, 21, 22, 23, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_enrollment_group failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_enrollment_group(sc, eg);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_by_param_NULL_PROV_SC)
{
    //arrange

    //act
    int res = prov_sc_delete_enrollment_group_by_param(NULL, TEST_GROUPID, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_by_param_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_delete_enrollment_group_by_param(sc, NULL, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_by_param_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE); // <- this NO_ETAG flag is what proves Requirement 054 - if etag wasn't ignored, actual calls will not line up with expected
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_enrollment_group_by_param(sc, TEST_REGID, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_by_param_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_enrollment_group_by_param(sc, TEST_REGID, TEST_ETAG_STAR);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_enrollment_group_by_param_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 8, 11, 12, 16, 18, 19, 20, 21, 22, 23, 24, 25 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_enrollment_group_by_param failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_enrollment_group_by_param(sc, TEST_GROUPID, TEST_ETAG_STAR);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_get_enrollment_group_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    ENROLLMENT_GROUP_HANDLE eg;

    //act
    int res = prov_sc_get_enrollment_group(NULL, TEST_GROUPID, &eg);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_get_enrollment_group_ERROR_INPUT_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_enrollment_group(sc, NULL, &eg);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_enrollment_group_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_enrollment_group(sc, TEST_GROUPID, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_enrollment_group_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_get_enrollment_group(sc, TEST_GROUPID, &eg);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(eg);

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
}

TEST_FUNCTION(prov_sc_get_enrollment_group_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    ENROLLMENT_GROUP_HANDLE eg = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(enrollmentGroup_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 9, 11, 12, 16, 18, 19, 21, 22, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_get_enrollment_group failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_get_enrollment_group(sc, TEST_REGID, &eg);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    enrollmentGroup_destroy(eg);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_get_device_registration_state_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    DEVICE_REGISTRATION_STATE_HANDLE drs;

    //act
    int res = prov_sc_get_device_registration_state(NULL, TEST_REGID, &drs);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_get_device_registration_state_ERROR_INPUT_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    DEVICE_REGISTRATION_STATE_HANDLE drs;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_device_registration_state(sc, NULL, &drs);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_device_registration_state_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_get_enrollment_group(sc, TEST_REGID, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_get_device_registration_state_GOLDEN)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    DEVICE_REGISTRATION_STATE_HANDLE drs = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(deviceRegistrationState_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_get_device_registration_state(sc, TEST_REGID, &drs);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(drs);

    //cleanup
    prov_sc_destroy(sc);
    deviceRegistrationState_destroy(drs);
}

TEST_FUNCTION(prov_sc_get_device_registration_state_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    DEVICE_REGISTRATION_STATE_HANDLE drs = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_GET);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_GET, RESPONSE);
    STRICT_EXPECTED_CALL(deviceRegistrationState_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 9, 11, 12, 16, 18, 19, 21, 22, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_get_device_registration_state failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_get_device_registration_state(sc, TEST_REGID, &drs);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    deviceRegistrationState_destroy(drs);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_ERROR_INPUT_NULL_PROV_SC)
{
    //arrange
    STRICT_EXPECTED_CALL(deviceRegistrationState_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_getRegistrationId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_device_registration_state(NULL, TEST_DEVICE_REGISTRATION_STATE_HANDLE);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_ERROR_INPUT_NULL_ENROLLMENT)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceRegistrationState_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_getRegistrationId(IGNORED_PTR_ARG));

    //act
    int res = prov_sc_delete_device_registration_state(sc, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(deviceRegistrationState_getEtag(IGNORED_PTR_ARG)).SetReturn(NULL); //no etag
    STRICT_EXPECTED_CALL(deviceRegistrationState_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_device_registration_state(sc, TEST_DEVICE_REGISTRATION_STATE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    STRICT_EXPECTED_CALL(deviceRegistrationState_getEtag(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(deviceRegistrationState_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_device_registration_state(sc, TEST_DEVICE_REGISTRATION_STATE_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(deviceRegistrationState_getEtag(IGNORED_PTR_ARG)); //can fail, but for sake of this example, cannot
    STRICT_EXPECTED_CALL(deviceRegistrationState_getRegistrationId(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 3, 4, 6, 10, 13, 14, 18, 20, 21, 22, 23, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_device_registration_state failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_device_registration_state(sc, TEST_DEVICE_REGISTRATION_STATE_HANDLE);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_by_param_NULL_PROV_SC)
{
    //arrange

    //act
    int res = prov_sc_delete_device_registration_state_by_param(NULL, TEST_REGID, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_by_param_NULL_REG_ID)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_delete_device_registration_state_by_param(sc, NULL, TEST_ETAG);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_by_param_GOLDEN_NO_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_DELETE); // <- this NO_ETAG flag is what proves Requirement 054 - if etag wasn't ignored, actual calls will not line up with expected
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_device_registration_state_by_param(sc, TEST_REGID, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_by_param_GOLDEN_WITH_ETAG)
{
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);
    umock_c_reset_all_calls();

    //arrange
    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    //act
    int res = prov_sc_delete_device_registration_state_by_param(sc, TEST_REGID, TEST_ETAG_STAR);

    //assert
    ASSERT_ARE_EQUAL(int, res, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_delete_device_registration_state_by_param_FAIL)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    set_response_status(RESPONSE_OFF);

    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(ETAG, HTTP_CLIENT_REQUEST_DELETE);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_DELETE, NO_RESPONSE);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //does not fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 8, 11, 12, 16, 18, 19, 20, 21, 22, 23, 24, 25 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_delete_device_registration_state_by_param failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_delete_device_registration_state_by_param(sc, TEST_REGID, TEST_ETAG_STAR);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_NULL_prov)
{
    //arrange
    INDIVIDUAL_ENROLLMENT_HANDLE ie_arr[2] = {TEST_INDIVIDUAL_ENROLLMENT_HANDLE, TEST_INDIVIDUAL_ENROLLMENT_HANDLE2};
    PROVISIONING_BULK_OPERATION bulkop;
    bulkop.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulkop.enrollments.ie = ie_arr;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.type = BULK_OP_INDIVIDUAL_ENROLLMENT;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res;

    //act
    int res = prov_sc_run_individual_enrollment_bulk_operation(NULL, &bulkop, &bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);

    //cleanup
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_NULL_bulkop)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_run_individual_enrollment_bulk_operation(sc, NULL, &bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_NULL_bulk_res)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie_arr[2] = { TEST_INDIVIDUAL_ENROLLMENT_HANDLE, TEST_INDIVIDUAL_ENROLLMENT_HANDLE2 };
    PROVISIONING_BULK_OPERATION bulkop;
    bulkop.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulkop.enrollments.ie = ie_arr;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.type = BULK_OP_INDIVIDUAL_ENROLLMENT;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_run_individual_enrollment_bulk_operation(sc, &bulkop, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_invalid_bulkop_version)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie_arr[2] = { TEST_INDIVIDUAL_ENROLLMENT_HANDLE, TEST_INDIVIDUAL_ENROLLMENT_HANDLE2 };
    PROVISIONING_BULK_OPERATION bulkop;
    bulkop.version = 47474747;
    bulkop.enrollments.ie = ie_arr;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.type = BULK_OP_INDIVIDUAL_ENROLLMENT;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_run_individual_enrollment_bulk_operation(sc, &bulkop, &bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_SUCCESS)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie_arr[2] = { TEST_INDIVIDUAL_ENROLLMENT_HANDLE, TEST_INDIVIDUAL_ENROLLMENT_HANDLE2 };
    PROVISIONING_BULK_OPERATION bulkop;
    bulkop.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulkop.enrollments.ie = ie_arr;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.type = BULK_OP_INDIVIDUAL_ENROLLMENT;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(bulkOperation_serializeToJson(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    STRICT_EXPECTED_CALL(bulkOperationResult_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_run_individual_enrollment_bulk_operation(sc, &bulkop, &bulk_res);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NOT_NULL(bulk_res);

    //cleanup
    prov_sc_destroy(sc);
    bulkOperationResult_free(bulk_res);
}

TEST_FUNCTION(prov_sc_run_individual_enrollment_bulk_operation_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    INDIVIDUAL_ENROLLMENT_HANDLE ie_arr[2] = { TEST_INDIVIDUAL_ENROLLMENT_HANDLE, TEST_INDIVIDUAL_ENROLLMENT_HANDLE2 };
    PROVISIONING_BULK_OPERATION bulkop;
    bulkop.version = PROVISIONING_BULK_OPERATION_VERSION_1;
    bulkop.enrollments.ie = ie_arr;
    bulkop.mode = BULK_OP_CREATE;
    bulkop.type = BULK_OP_INDIVIDUAL_ENROLLMENT;
    PROVISIONING_BULK_OPERATION_RESULT* bulk_res = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(bulkOperation_serializeToJson(IGNORED_PTR_ARG));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //does not fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE); //----------------->20?
    STRICT_EXPECTED_CALL(bulkOperationResult_deserializeFromJson(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 8, 10, 11, 15, 17, 18, 20, 21, 23, 24, 25, 26, 27 };
    size_t count = umock_c_negative_tests_call_count();
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);

    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_run_individual_enrollment_bulk_op_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_run_individual_enrollment_bulk_operation(sc, &bulkop, &bulk_res);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    prov_sc_destroy(sc);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_NULL_prov_client)
{
    //arrange
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_individual_enrollment(NULL, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_NULL_query_spec)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_individual_enrollment(sc, NULL, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_NULL_cont_token_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, NULL, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_NULL_query_resp_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_invalid_version)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = 47474;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_success_full_results)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(false, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_success_paging_no_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NOT_NULL(cont_token);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_success_paging_given_token_no_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_individual_enrollment_success_paging_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN2, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

/*---Note that this failure test covers all failures from other cases by virtue of making all possible calls---/*
TEST_FUNCTION(prov_sc_query_individual_enrollment_success_paging_given_token_w_token_return_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST); //10
    expected_calls_add_query_headers(true, true); //12
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT); //cannot "fail"
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_INDIVIDUAL_ENROLLMENT));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 8, 10, 13, 17, 19, 22, 23, 24, 26, 29, 30, 31, 32, 33, 34 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_query_individual_enrollment_success_paging_given_token_w_token_return_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_query_individual_enrollment(sc, &qs, &cont_token, &query_resp);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_NULL_prov_client)
{
    //arrange
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_enrollment_group(NULL, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
}

TEST_FUNCTION(prov_sc_query_enrollment_group_NULL_query_spec)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_enrollment_group(sc, NULL, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_NULL_cont_token_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, NULL, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_NULL_query_resp_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_invalid_version)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = 47474;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_success_full_results)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(false, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_success_paging_no_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NOT_NULL(cont_token);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_success_paging_given_token_no_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_enrollment_group_success_paging_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN2, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

/*---Note that this failure test covers all failures from other cases by virtue of making all possible calls---*/
TEST_FUNCTION(prov_sc_query_enrollment_group_success_paging_given_token_w_token_return_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.query_string = TEST_QUERY_STRING;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(querySpecification_serializeToJson(&qs));
    expected_calls_construct_registration_path(false);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST); //10
    expected_calls_add_query_headers(true, true); //12
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP); //cannot "fail"
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_ENROLLMENT_GROUP));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 3, 8, 10, 13, 17, 19, 22, 23, 24, 26, 29, 30, 31, 32, 33, 34};
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_query_enrollment_group_success_paging_given_token_w_token_return_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_query_enrollment_group(sc, &qs, &cont_token, &query_resp);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_NULL_prov_client)
{
    //arrange
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_device_registration_state(NULL, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
}

TEST_FUNCTION(prov_sc_query_device_registration_state_NULL_query_spec)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_device_registration_state(sc, NULL, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_NULL_cont_token_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, NULL, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_NULL_query_resp_ptr)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_invalid_version)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.registration_id = TEST_REGID;
    qs.version = 47474;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NULL(query_resp);

    //cleanup
    prov_sc_destroy(sc);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_success_full_results)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = NO_MAX_PAGE_SIZE;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(false, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_success_paging_no_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token = NULL;
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, false);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NOT_NULL(cont_token);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_success_paging_given_token_no_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(NULL); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_IS_NULL(cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

TEST_FUNCTION(prov_sc_query_device_registration_state_success_paging_given_token_w_token_return)
{
    //arrange
    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST);
    expected_calls_add_query_headers(true, true);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot fail
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE);
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE)); //cannot fail
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    //act
    int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, res);
    ASSERT_ARE_EQUAL(char_ptr, TEST_CONT_TOKEN2, cont_token);
    ASSERT_IS_NOT_NULL(query_resp);

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
    queryResponse_free(query_resp);
}

/*---Note that this failure test covers all failures from other cases by virtue of making all possible calls--*/
TEST_FUNCTION(prov_sc_query_device_registration_state_success_paging_given_token_w_token_return_ERROR)
{
    //arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    PROVISIONING_SERVICE_CLIENT_HANDLE sc = prov_sc_create_from_connection_string(TEST_CONNECTION_STRING);
    PROVISIONING_QUERY_SPECIFICATION qs = { 0 };
    qs.page_size = 5;
    qs.registration_id = TEST_REGID;
    qs.version = PROVISIONING_QUERY_SPECIFICATION_VERSION_1;

    char* cont_token;
    mallocAndStrcpy_s(&cont_token, TEST_CONT_TOKEN);
    PROVISIONING_QUERY_RESPONSE* query_resp = NULL;
    umock_c_reset_all_calls();

    expected_calls_construct_registration_path(true);
    expected_calls_construct_http_headers(NO_ETAG, HTTP_CLIENT_REQUEST_POST); //12
    expected_calls_add_query_headers(true, true); //14
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); //cannot fail
    expected_calls_rest_call(HTTP_CLIENT_REQUEST_POST, RESPONSE);
    //response headers
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(TEST_CONT_TOKEN2); //cannot "fail"
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE); //cannot "fail"
    //end response headers
    STRICT_EXPECTED_CALL(queryType_stringToEnum(QUERY_RESPONSE_HEADER_ITEM_TYPE_VALUE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(queryResponse_deserializeFromJson(IGNORED_PTR_ARG, QUERY_TYPE_DEVICE_REGISTRATION_STATE));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG)); //does not fail
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG)); //cannot fail
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG)); //cannot fail

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 1, 2, 4, 9, 11, 14, 18, 20, 23, 24, 25, 26, 27, 30, 31, 32, 33, 34, 35 };
    size_t num_cannot_fail = sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0]);
    size_t count = umock_c_negative_tests_call_count();
    size_t test_num = 0;
    size_t test_max = count - num_cannot_fail;

    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, num_cannot_fail) != 0)
            continue;
        test_num++;

        char tmp_msg[128];
        sprintf(tmp_msg, "prov_sc_query_device_registration_state_success_paging_given_token_w_token_return_ERROR failure in test %zu/%zu", test_num, test_max);

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        //act
        int res = prov_sc_query_device_registration_state(sc, &qs, &cont_token, &query_resp);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, res, 0, tmp_msg);

        g_uhttp_client_dowork_call_count = 0;
    }

    //cleanup
    free(cont_token);
    prov_sc_destroy(sc);
}

END_TEST_SUITE(provisioning_service_client_ut);