// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include <stdbool.h>

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/vector_types_internal.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/agenttime.h"

#include "iothub_client_options.h"
#include "iothub_client_version.h"
#include "internal/iothub_client_private.h"
#include "iothub_client_core_common.h"
#include "internal/iothubtransport.h"
#include "internal/iothub_message_private.h"

#include "internal/iothub_transport_ll_private.h"

MOCKABLE_FUNCTION(, bool, Transport_MessageCallbackFromInput, IOTHUB_MESSAGE_HANDLE, messageHandle, void*, ctx);
MOCKABLE_FUNCTION(, bool, Transport_MessageCallback, IOTHUB_MESSAGE_HANDLE, messageHandle, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_ConnectionStatusCallBack, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_SendComplete_Callback, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result, void*, ctx);
MOCKABLE_FUNCTION(, const char*, Transport_GetOption_Product_Info_Callback, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_ReportedStateComplete_Callback, uint32_t, item_id, int, status_code, void*, ctx);
MOCKABLE_FUNCTION(, void, Transport_Twin_RetrievePropertyComplete_Callback, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size, void*, ctx);
MOCKABLE_FUNCTION(, int, Transport_DeviceMethod_Complete_Callback, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id, void*, ctx);

#undef ENABLE_MOCKS

#include "iothubtransporthttp.h"

#include "real_strings.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern BUFFER_HANDLE real_BUFFER_new(void);
    extern void real_BUFFER_delete(BUFFER_HANDLE handle);
    extern unsigned char* real_BUFFER_u_char(BUFFER_HANDLE handle);
    extern size_t real_BUFFER_length(BUFFER_HANDLE handle);
    extern int real_BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern int real_BUFFER_append_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
    extern BUFFER_HANDLE real_BUFFER_clone(BUFFER_HANDLE handle);
    extern BUFFER_HANDLE real_BUFFER_create(const unsigned char* source, size_t size);

    extern int real_mallocAndStrcpy_s(char** destination, const char* source);
    extern int real_size_tToString(char* destination, size_t destinationSize, size_t value);

    extern VECTOR_HANDLE real_VECTOR_create(size_t elementSize);
    extern VECTOR_HANDLE real_VECTOR_move(VECTOR_HANDLE handle);
    extern void real_VECTOR_destroy(VECTOR_HANDLE handle);
    extern int real_VECTOR_push_back(VECTOR_HANDLE handle, const void* elements, size_t numElements);
    extern void real_VECTOR_erase(VECTOR_HANDLE handle, void* elements, size_t numElements);
    extern void real_VECTOR_clear(VECTOR_HANDLE handle);
    extern void* real_VECTOR_element(VECTOR_HANDLE handle, size_t index);
    extern void* real_VECTOR_front(VECTOR_HANDLE handle);
    extern void* real_VECTOR_back(VECTOR_HANDLE handle);
    extern void* real_VECTOR_find_if(VECTOR_HANDLE handle, PREDICATE_FUNCTION pred, const void* value);
    extern size_t real_VECTOR_size(VECTOR_HANDLE handle);

    extern void real_DList_InitializeListHead(PDLIST_ENTRY listHead);
    extern int real_DList_IsListEmpty(const PDLIST_ENTRY listHead);
    extern void real_DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    extern void real_DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
    extern void real_DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
    extern int real_DList_RemoveEntryList(PDLIST_ENTRY listEntry);
    extern PDLIST_ENTRY real_DList_RemoveHeadList(PDLIST_ENTRY listHead);

#ifdef __cplusplus
}
#endif

#define IOTHUB_ACK "iothub-ack"
#define IOTHUB_ACK_NONE "none"
#define IOTHUB_ACK_FULL "full"
#define IOTHUB_ACK_POSITIVE "positive"
#define IOTHUB_ACK_NEGATIVE "negative"
static const char* TEST_MESSAGE_ID = "FC365F0A-D432-4AB9-8307-59C0EB8F0447";
static const char* TEST_CONTENT_TYPE = "application/json";
static const char* TEST_CONTENT_ENCODING = "utf8";
static const char* TEST_PRODUCT_INFO = "product_info";

#define EVENT_ENDPOINT  "/messages/events"
#define API_VERSION     "?api-version=2016-11-14"
#define MESSAGE_ENDPOINT_HTTP "/messages/devicebound"
#define MESSAGE_ENDPOINT_HTTP_ETAG "/messages/devicebound/"

//DEFINE_MICROMOCK_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
//DEFINE_MICROMOCK_ENUM_TO_STRING(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(PLATFORM_INFO_OPTION, PLATFORM_INFO_OPTION_VALUES);

#define TEST_DEVICE_ID "thisIsDeviceID"
#define TEST_DEVICE_ID2 "aSecondDeviceID"
#define TEST_DEVICE_KEY "thisIsDeviceKey"
#define TEST_DEVICE_KEY2 "aSecondDeviceKey"
#define TEST_DEVICE_TOKEN "thisIsDeviceSasToken"
#define TEST_IOTHUB_NAME "thisIsIotBuhName"
#define TEST_IOTHUB_SUFFIX "thisIsIotHubSuffix"
#define TEST_IOTHUB_GWHOSTNAME "thisIsGWHostname"
#define TEST_BLANK_SAS_TOKEN " "
#define TEST_IOTHUB_CLIENT_CORE_LL_HANDLE (IOTHUB_CLIENT_CORE_LL_HANDLE)0x34333
#define TEST_IOTHUB_CLIENT_CORE_LL_HANDLE2 (IOTHUB_CLIENT_CORE_LL_HANDLE)0x34344
#define TEST_ETAG_VALUE_UNQUOTED "thisIsSomeETAGValueSomeGUIDMaybe"
#define TEST_ETAG_VALUE MU_TOSTRING(TEST_ETAG_VALUE_UNQUOTED)
#define TEST_PROPERTY_A_NAME "a"
#define TEST_PROPERTY_A_VALUE "value_of_a"

#define TEST_HTTPAPIEX_HANDLE (HTTPAPIEX_HANDLE)0x343

//static const bool thisIsTrue = true;
//static const bool thisIsFalse = false;
//#define ENABLE_BATCHING() do{(void)IoTHubTransportHttp_SetOption(handle, "Batching", &thisIsTrue);} while(BASEIMPLEMENTATION::gballocState-BASEIMPLEMENTATION::gballocState)
//#define DISABLE_BATCHING() do{(void)IoTHubTransportHttp_SetOption(handle, "Batching", &thisIsFalse);} while(BASEIMPLEMENTATION::gballocState-BASEIMPLEMENTATION::gballocState)

static unsigned char contains3[1] = { '3' };

static const IOTHUB_DEVICE_CONFIG TEST_DEVICE_1 =
{
    TEST_DEVICE_ID,
    TEST_DEVICE_KEY,
    NULL
};

static const IOTHUB_DEVICE_CONFIG TEST_DEVICE_2 =
{
    TEST_DEVICE_ID2,
    TEST_DEVICE_KEY2,
    NULL
};

static const IOTHUB_DEVICE_CONFIG TEST_DEVICE_3 =
{
    TEST_DEVICE_ID,
    NULL,
    TEST_DEVICE_TOKEN
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_CONFIG =
{
    HTTP_Protocol,                                  /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,                                 /* const char* deviceId;                        */
    TEST_DEVICE_KEY,                                /* const char* deviceKey;                       */
    NULL,                                           /* const char* deviceSasToken;                  */
    TEST_IOTHUB_NAME,                               /* const char* iotHubName;                      */
    TEST_IOTHUB_SUFFIX,                              /* const char* iotHubSuffix;                    */
    NULL                                       /* const char* protocolGatewayHostName;                  */
};

static DLIST_ENTRY waitingToSend;

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG =
{
    &TEST_CONFIG_IOTHUBCLIENT_CONFIG,
    &waitingToSend
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_GW_CONFIG =
{
    HTTP_Protocol,                                  /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,                                 /* const char* deviceId;                        */
    TEST_DEVICE_KEY,                                /* const char* deviceKey;                       */
    NULL,                                           /* const char* deviceSasToken;                  */
    TEST_IOTHUB_NAME,                               /* const char* iotHubName;                      */
    TEST_IOTHUB_SUFFIX,                              /* const char* iotHubSuffix;                    */
    TEST_IOTHUB_GWHOSTNAME                        /* const char* protocolGatewayHostName;                  */
};

static IOTHUBTRANSPORT_CONFIG TEST_GW_CONFIG =
{
    &TEST_CONFIG_IOTHUBCLIENT_GW_CONFIG,
    &waitingToSend
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_CONFIG2 =
{
    HTTP_Protocol,                                  /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID2,                                 /* const char* deviceId;                        */
    TEST_DEVICE_KEY2,                                /* const char* deviceKey;                       */
    NULL,                                           /* const char* deviceSasToken;                  */
    TEST_IOTHUB_NAME,                               /* const char* iotHubName;                      */
    TEST_IOTHUB_SUFFIX,                              /* const char* iotHubSuffix;                    */
    NULL                                       /* const char* protocolGatewayHostName;                  */
};

static DLIST_ENTRY waitingToSend2;

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG2 =
{
    &TEST_CONFIG_IOTHUBCLIENT_CONFIG2,
    &waitingToSend2
};

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG_NULL_CONFIG =
{
    NULL,
    (PDLIST_ENTRY)0x1
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_PROTOCOL =
{
    NULL,                                       /*IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;  */
    TEST_DEVICE_ID,                             /*const char* deviceId;                       */
    TEST_DEVICE_KEY,                            /*const char* deviceKey;                      */
    NULL,                                       /* const char* deviceSasToken;                  */
    TEST_IOTHUB_NAME,                           /*const char* iotHubName;                     */
    TEST_IOTHUB_SUFFIX,                          /* const char* iotHubSuffix;                    */
    NULL                                       /* const char* protocolGatewayHostName;                  */
};

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG_NULL_PROTOCOL =
{
    &TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_PROTOCOL,
    (PDLIST_ENTRY)0x1
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_IOTHUB_NAME =
{
    HTTP_Protocol,                              /*IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;  */
    TEST_DEVICE_ID,                             /*const char* deviceId;                       */
    TEST_DEVICE_KEY,                            /*const char* deviceKey;                      */
    NULL,                                       /* const char* deviceSasToken;                  */
    NULL,                                       /*const char* iotHubName;                     */
    TEST_IOTHUB_SUFFIX,                          /* const char* iotHubSuffix;                    */
    NULL                                       /* const char* protocolGatewayHostName;                  */
};

static const IOTHUB_CLIENT_CONFIG TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_IOTHUB_SUFFIX =
{
    HTTP_Protocol,                                  /* IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;   */
    TEST_DEVICE_ID,                                 /* const char* deviceId;                        */
    TEST_DEVICE_KEY,                                /* const char* deviceKey;                       */
    NULL,                                           /* const char* deviceSasToken;                  */
    TEST_IOTHUB_NAME,                               /* const char* iotHubName;                      */
    NULL,                                            /* const char* iotHubSuffix;                    */
    NULL                                       /* const char* protocolGatewayHostName;                  */
};

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG_NULL_IOTHUB_NAME =
{
    &TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_IOTHUB_NAME,
    (PDLIST_ENTRY)0x1
};

static IOTHUBTRANSPORT_CONFIG TEST_CONFIG_NULL_IOTHUB_SUFFIX =
{
    &TEST_CONFIG_IOTHUBCLIENT_CONFIG_NULL_IOTHUB_SUFFIX,
    (PDLIST_ENTRY)0x1
};

#define TEST_IOTHUB_MESSAGE_HANDLE_1 ((IOTHUB_MESSAGE_HANDLE)0x01d1)
#define TEST_IOTHUB_MESSAGE_HANDLE_2 ((IOTHUB_MESSAGE_HANDLE)0x01d2)
#define TEST_IOTHUB_MESSAGE_HANDLE_3 ((IOTHUB_MESSAGE_HANDLE)0x01d3)
#define TEST_IOTHUB_MESSAGE_HANDLE_4 ((IOTHUB_MESSAGE_HANDLE)0x01d4)
#define TEST_IOTHUB_MESSAGE_HANDLE_5 ((IOTHUB_MESSAGE_HANDLE)0x01d5)
#define TEST_IOTHUB_MESSAGE_HANDLE_6 ((IOTHUB_MESSAGE_HANDLE)0x01d6)
#define TEST_IOTHUB_MESSAGE_HANDLE_7 ((IOTHUB_MESSAGE_HANDLE)0x01d7)
#define TEST_IOTHUB_MESSAGE_HANDLE_8 ((IOTHUB_MESSAGE_HANDLE)0x01d8)
#define TEST_IOTHUB_MESSAGE_HANDLE_9 ((IOTHUB_MESSAGE_HANDLE)0x01d9)
#define TEST_IOTHUB_MESSAGE_HANDLE_10 ((IOTHUB_MESSAGE_HANDLE)0x01da)
#define TEST_IOTHUB_MESSAGE_HANDLE_11 ((IOTHUB_MESSAGE_HANDLE)0x01db)
#define TEST_IOTHUB_MESSAGE_HANDLE_12 ((IOTHUB_MESSAGE_HANDLE)0x01dc)

static IOTHUB_MESSAGE_LIST message1 =  /*this is the oldest message, always the first to be processed, send etc*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_1,                    /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message2 = /*this is the message next to the oldest message, it is processed after the oldest message*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_2,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message3 = /*this is the newest message, always last to be processed*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_3,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message4 = /*this is outof bounds message (>256K)*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_4,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message5 = /*this is almost out of bounds message (<256K)*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_5,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message6 = /*this has properties (1)*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_6,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message7 = /*this has properties (2)*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_7,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message9 = /*this has 256*1024 bytes - must fail @ send unbatched*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_9,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message10 = /*this is a simple string message*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_10,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message11 = /*this is message of 255*1024 bytes - 384 - 16 -2 bytes length, so that adding a property "a":"b" brings it to the maximum size*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_11,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

static IOTHUB_MESSAGE_LIST message12 = /*this is message of 255*1024 bytes - 384 - 16 - 2 bytes length, so that adding a property "aa":"b" brings it over the maximum size*/
{
    TEST_IOTHUB_MESSAGE_HANDLE_12,                  /*IOTHUB_MESSAGE_HANDLE messageHandle;                        */
    NULL,                                           /*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;     */
    NULL,                                           /*void* context;                                              */
    { NULL, NULL }                                  /*DLIST_ENTRY entry;                                          */
};

#define TEST_MAP_EMPTY (MAP_HANDLE) 0xe0
#define TEST_MAP_1_PROPERTY (MAP_HANDLE) 0xe1
#define TEST_MAP_2_PROPERTY (MAP_HANDLE) 0xe2
#define TEST_MAP_3_PROPERTY (MAP_HANDLE) 0xe3
#define TEST_MAP_1_PROPERTY_A_B (MAP_HANDLE) 0xe4
#define TEST_MAP_1_PROPERTY_AA_B (MAP_HANDLE) 0xe5

#define TEST_RED_KEY "redkey"
#define TEST_RED_KEY_STRING TEST_RED_KEY
#define TEST_RED_KEY_STRING_WITH_IOTHUBAPP "\"" "iothub-app-" TEST_RED_KEY "\""

#define TEST_RED_VALUE "redvalue"
#define TEST_RED_VALUE_STRING MU_TOSTRING(TEST_RED_VALUE)

#define TEST_BLUE_KEY "bluekey"
#define TEST_BLUE_KEY_STRING TEST_BLUE_KEY
#define TEST_BLUE_KEY_STRING_WITH_IOTHUBAPP "\"" "iothub-app-" TEST_BLUE_KEY "\""

#define TEST_BLUE_VALUE "bluevalue"
#define TEST_BLUE_VALUE_STRING MU_TOSTRING(TEST_BLUE_VALUE)

#define TEST_YELLOW_KEY "yellowkey"
#define TEST_YELLOW_KEY_STRING TEST_YELLOW_KEY
#define TEST_YELLOW_KEY_STRING_WITH_IOTHUBAPP "\"" "iothub-app-" TEST_YELLOW_KEY "\""

#define TEST_YELLOW_VALUE "yellowvaluekey"
#define TEST_YELLOW_VALUE_STRING MU_TOSTRING(TEST_YELLOW_VALUE)

static const char* TEST_KEYS1[] = { TEST_RED_KEY_STRING };
static const char* TEST_VALUES1[] = { TEST_RED_VALUE };

static const char* TEST_KEYS2[] = { TEST_BLUE_KEY_STRING, TEST_YELLOW_KEY_STRING };
static const char* TEST_VALUES2[] = { TEST_BLUE_VALUE,  TEST_YELLOW_VALUE };

static const char* TEST_KEYS1_A_B[] = { "a" };
static const char* TEST_VALUES1_A_B[] = { "b" };

static const char* TEST_KEYS1_AA_B[] = { "aa" };
static const char* TEST_VALUES1_AA_B[] = { "b" };

static IOTHUBMESSAGE_DISPOSITION_RESULT currentDisposition;

#define MAXIMUM_MESSAGE_SIZE (255*1024-1)
#define PAYLOAD_OVERHEAD (384)
#define PROPERTY_OVERHEAD (16)

#define TEST_MINIMAL_PAYLOAD [{"body":""}]
#define TEST_MINIMAL_PAYLOAD_STRING TOSTRING(TEST_MINIMAL_PAYLOAD)

#define TEST_BIG_BUFFER_1_SIZE          (MAXIMUM_MESSAGE_SIZE - PAYLOAD_OVERHEAD)
#define TEST_BIG_BUFFER_1_OVERFLOW_SIZE (TEST_BIG_BUFFER_1_SIZE +1 )
#define TEST_BIG_BUFFER_1_FIT_SIZE      (TEST_BIG_BUFFER_1_SIZE)

#define TEST_BIG_BUFFER_9_OVERFLOW_SIZE (256*1024)

static const unsigned char buffer1[1] = { '1' };
static const size_t buffer1_size = sizeof(buffer1);

static const unsigned char buffer2[2] = { '2', '2' };
static const size_t buffer2_size = sizeof(buffer2);

static const unsigned char buffer3[3] = { '3', '3', '3' };
static const size_t buffer3_size = sizeof(buffer3);

static const unsigned char buffer6[6] = { '1', '2', '3', '4', '5', '6' };
static const size_t buffer6_size = sizeof(buffer6);

static const unsigned char buffer7[7] = { '1', '2', '3', '4', '5', '6', '7' };
static const size_t buffer7_size = sizeof(buffer7);

static const unsigned char* buffer9;
static const size_t buffer9_size = TEST_BIG_BUFFER_9_OVERFLOW_SIZE;

static const unsigned char* buffer11;
static const size_t buffer11_size = MAXIMUM_MESSAGE_SIZE - PAYLOAD_OVERHEAD - 2 - PROPERTY_OVERHEAD;

static unsigned char* bigBufferOverflow; /*this is a buffer that contains just enough characters to go over the limit of 256K as a single message*/
static unsigned char* bigBufferFit; /*this is a buffer that contains just enough characters to NOT go over the limit of 256K as a single message*/

static const char* string10 = "thisgoestoJ\\s//on\"ToBeEn\r\n\bcoded";

const unsigned int httpStatus200 = 200;
const unsigned int httpStatus201 = 201;
const unsigned int httpStatus204 = 204;
const unsigned int httpStatus404 = 404;

static BUFFER_HANDLE last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = NULL;

static bool HTTPHeaders_GetHeaderCount_writes_to_its_outputs = true;

static const char*   TEST_STRING_DATA = "Hello Test World";
static STRING_HANDLE TEST_STRING_HANDLE = NULL;

static TRANSPORT_CALLBACKS_INFO transport_cb_info;
static void* transport_cb_ctx = (void*)0x499922;

#define TEST_HEADER_1 "iothub-app-NAME1: VALUE1"
#define TEST_HEADER_1_5 "not-iothub-app-NAME1: VALUE1"
#define TEST_HEADER_2 "iothub-app-NAME2: VALUE2"
#define TEST_HEADER_3 "iothub-messageid: VALUE3"
#define TEST_HEADER_4 "ContentType: VALUE4"
#define TEST_HEADER_5 "ContentEncoding: VALUE5"
/*value returned by time() function*/
/*for the purpose of this implementation, time_t represents the number of seconds since 1970, 1st jan, 0:0:0*/
#define TEST_GET_TIME_VALUE 384739233
#define TEST_DEFAULT_GETMINIMUMPOLLINGTIME 1500

static pfIotHubTransport_SendMessageDisposition         IoTHubTransportHttp_SendMessageDisposition;
static pfIoTHubTransport_Subscribe_DeviceTwin           IoTHubTransportHttp_Subscribe_DeviceTwin;
static pfIoTHubTransport_Unsubscribe_DeviceTwin         IoTHubTransportHttp_Unsubscribe_DeviceTwin;
static pfIoTHubTransport_GetTwinAsync                   IoTHubTransportHttp_GetTwinAsync;
static pfIoTHubTransport_GetHostname                    IoTHubTransportHttp_GetHostname;
static pfIoTHubTransport_SetOption                      IoTHubTransportHttp_SetOption;
static pfIoTHubTransport_Create                         IoTHubTransportHttp_Create;
static pfIoTHubTransport_Destroy                        IoTHubTransportHttp_Destroy;
static pfIotHubTransport_Register                       IoTHubTransportHttp_Register;
static pfIotHubTransport_Unregister                     IoTHubTransportHttp_Unregister;
static pfIoTHubTransport_Subscribe                      IoTHubTransportHttp_Subscribe;
static pfIoTHubTransport_Unsubscribe                    IoTHubTransportHttp_Unsubscribe;
static pfIoTHubTransport_DoWork                         IoTHubTransportHttp_DoWork;
static pfIoTHubTransport_GetSendStatus                  IoTHubTransportHttp_GetSendStatus;
static pfIoTHubTransport_SetCallbackContext             IoTHubTransportHttp_SetCallbackContext;
static pfIoTHubTransport_GetSupportedPlatformInfo       IoTHubTransportHttp_GetSupportedPlatformInfo;

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

STRING_HANDLE my_URL_EncodeString(const char* textEncode)
{
    (void)textEncode;
    return real_STRING_construct(TEST_DEVICE_ID);
}

HTTPAPIEX_HANDLE my_HTTPAPIEX_Create(const char* hostName)
{
    (void)hostName;
    return (HTTPAPIEX_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPAPIEX_Destroy(HTTPAPIEX_HANDLE handle)
{
    my_gballoc_free(handle);
}

static IOTHUB_CLIENT_RESULT my_IoTHubClientCore_LL_GetOption(IOTHUB_CLIENT_CORE_LL_HANDLE handle, const char* option, void** value)
{
    (void)handle;
    (void)option;
    *value = TEST_STRING_HANDLE;
    return IOTHUB_CLIENT_OK;
}

static IOTHUB_MESSAGE_HANDLE my_IoTHubClientCore_LL_MessageCallback_messageHandle;
static bool my_IoTHubClientCore_LL_MessageCallback_return_value;
static bool my_Transport_MessageCallback(IOTHUB_MESSAGE_HANDLE message_handle, void* ctx)
{
    (void)ctx;
    my_IoTHubClientCore_LL_MessageCallback_messageHandle = message_handle;
    return my_IoTHubClientCore_LL_MessageCallback_return_value;
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE httpHeadersHandle)
{
    my_gballoc_free(httpHeadersHandle);
}

static HTTPAPIEX_SAS_HANDLE my_HTTPAPIEX_SAS_Create(STRING_HANDLE key, STRING_HANDLE uriResource, STRING_HANDLE keyName)
{
    (void)key;
    (void)uriResource;
    (void)keyName;
    return (HTTPAPIEX_SAS_HANDLE)my_gballoc_malloc(1);
}

static void my_HTTPAPIEX_SAS_Destroy(HTTPAPIEX_SAS_HANDLE handle)
{
    my_gballoc_free(handle);
}

static HTTPAPIEX_RESULT my_HTTPAPIEX_ExecuteRequest(HTTPAPIEX_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath, HTTP_HEADERS_HANDLE requestHttpHeadersHandle, BUFFER_HANDLE requestContent, unsigned int* statusCode, HTTP_HEADERS_HANDLE responseHttpHeadersHandle, BUFFER_HANDLE responseContent)
{
    (void)handle;
    (void)requestType;
    (void)relativePath;
    (void)requestHttpHeadersHandle;
    (void)requestContent;
    (void)responseHttpHeadersHandle;
    (void)responseContent;
    *statusCode = 204;
    if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
    {
        real_BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
    }
    last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = real_BUFFER_clone(requestContent);
    return HTTPAPIEX_OK;
}

static HTTPAPIEX_RESULT my_HTTPAPIEX_SAS_ExecuteRequest(HTTPAPIEX_SAS_HANDLE sasHandle, HTTPAPIEX_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath, HTTP_HEADERS_HANDLE requestHttpHeadersHandle, BUFFER_HANDLE requestContent, unsigned int* statusCode, HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    (void)sasHandle;
    (void)handle;
    (void)requestType;
    (void)relativePath;
    (void)requestHttpHeadersHandle;
    (void)responseHeadersHandle;
    (void)requestContent;
    (void)responseContent;
    *statusCode = 204;
    if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
    {
        real_BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
    }
    last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = real_BUFFER_clone(requestContent);
    return HTTPAPIEX_OK;
}

static IOTHUB_MESSAGE_HANDLE my_IoTHubMessage_CreateFromByteArray(const unsigned char* byteArray, size_t size)
{
    (void)byteArray;
    (void)size;
    return (IOTHUB_MESSAGE_HANDLE)my_gballoc_malloc(1);
}

static void my_IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    if (iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_1 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_2 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_3 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_4 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_5 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_6 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_7 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_8 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_9 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_10 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_11 &&
        iotHubMessageHandle != TEST_IOTHUB_MESSAGE_HANDLE_12)
    {
        my_gballoc_free(iotHubMessageHandle);
    }
}

static HTTP_HEADERS_RESULT my_HTTPHeaders_GetHeaderCount(HTTP_HEADERS_HANDLE handle, size_t* headerCount)
{
    (void)handle;
    *headerCount = 0;
    return HTTP_HEADERS_OK;
}

static HTTP_HEADERS_RESULT my_HTTPHeaders_GetHeader(HTTP_HEADERS_HANDLE handle, size_t index, char** destination)
{
    (void)handle;
    if (index == 0)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_1) + 1);
        strcpy(*destination, TEST_HEADER_1);
    }
    else if (index == 1)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_1_5) + 1);
        strcpy(*destination, TEST_HEADER_1_5);

    }
    else if (index == 2)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_2) + 1);
        strcpy(*destination, TEST_HEADER_2);

    }
    else if (index == 3)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_3) + 1);
        strcpy(*destination, TEST_HEADER_3);
    }
    else if (index == 4)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_4) + 1);
        strcpy(*destination, TEST_HEADER_4);
    }
    else if (index == 5)
    {
        *destination = (char*)my_gballoc_malloc(strlen(TEST_HEADER_5) + 1);
        strcpy(*destination, TEST_HEADER_5);
    }
    else
    {
        *destination = NULL; /*never to be reached*/
    }
    return HTTP_HEADERS_OK;
}

static IOTHUB_MESSAGE_RESULT my_IoTHubMessage_GetByteArray(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const unsigned char** buffer, size_t* size)
{
    if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_1)
    {
        *buffer = buffer1;
        *size = buffer1_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_2)
    {
        *buffer = buffer2;
        *size = buffer2_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_3)
    {
        *buffer = buffer3;
        *size = buffer3_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_4) /*this is out of bounds message (>256K)*/
    {
        *buffer = bigBufferOverflow;
        *size = TEST_BIG_BUFFER_1_OVERFLOW_SIZE;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_5) /*this is a message that just fits*/
    {
        *buffer = bigBufferFit;
        *size = TEST_BIG_BUFFER_1_FIT_SIZE;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_6) /*this is a message that just fits*/
    {
        *buffer = buffer6;
        *size = buffer6_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_7) /*this is a message that just fits*/
    {
        *buffer = buffer7;
        *size = buffer7_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_9) /*this is a message that is +1 byte over the unbtached send limit*/
    {
        *buffer = buffer9;
        *size = buffer9_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_11) /*this is a message that has a property and together with that property fit at maximum*/
    {
        *buffer = buffer11;
        *size = buffer11_size;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_12) /*this is a message that has a property and together with that property does NOT fit at maximum*/
    {
        *buffer = buffer11; /*this is not a copy&paste mistake, it is intended to use the same "to the limit" buffer as 11*/
        *size = buffer11_size;
    }
    else
    {
        /*not expected really*/
        *buffer = (const unsigned char*)"333";
        *size = 3;
    }
    return IOTHUB_MESSAGE_OK;
}

static MAP_HANDLE my_IoTHubMessage_Properties(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    MAP_HANDLE result2;
    if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_1)
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_2)
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_3)
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_4)  /*this is out of bounds message (>256K)*/
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_5) /*this is a message that just fits*/
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_6)
    {
        result2 = TEST_MAP_1_PROPERTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_7)
    {
        result2 = TEST_MAP_2_PROPERTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_8)
    {
        result2 = TEST_MAP_3_PROPERTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_9)
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_10)
    {
        result2 = TEST_MAP_EMPTY;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_11)
    {
        result2 = TEST_MAP_1_PROPERTY_A_B;
    }
    else if (iotHubMessageHandle == TEST_IOTHUB_MESSAGE_HANDLE_12)
    {
        result2 = TEST_MAP_1_PROPERTY_AA_B;
    }
    else
    {
        /*not expected really*/
        result2 = NULL;
        ASSERT_FAIL("not expected");
    }
    return result2;
}

static HTTP_HEADERS_HANDLE my_HTTPHeaders_Clone(HTTP_HEADERS_HANDLE handle)
{
    (void)handle;
    return (HTTP_HEADERS_HANDLE)my_gballoc_malloc(1);
}

static MAP_RESULT my_Map_GetInternals(MAP_HANDLE handle, const char*const** keys, const char*const** values, size_t* count)
{
    if (handle == TEST_MAP_EMPTY)
    {
        *keys = NULL;
        *values = NULL;
        *count = 0;
    }
    else if (handle == TEST_MAP_1_PROPERTY)
    {
        *keys = (const char*const*)TEST_KEYS1;
        *values = (const char*const*)TEST_VALUES1;
        *count = 1;
    }
    else if (handle == TEST_MAP_2_PROPERTY)
    {
        *keys = (const char*const*)TEST_KEYS2;
        *values = (const char*const*)TEST_VALUES2;
        *count = 2;
    }
    else if (handle == TEST_MAP_1_PROPERTY_A_B)
    {
        *keys = (const char*const*)TEST_KEYS1_A_B;
        *values = (const char*const*)TEST_VALUES1_A_B;
        *count = 1;
    }
    else if (handle == TEST_MAP_1_PROPERTY_AA_B)
    {
        *keys = (const char*const*)TEST_KEYS1_AA_B;
        *values = (const char*const*)TEST_VALUES1_AA_B;
        *count = 1;
    }
    else
    {
        ASSERT_FAIL("unexpected value");
    }
    return MAP_OK;
}

static void setupCreateHappyPathAlloc(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(IoTHub_Transport_ValidateCallbacks(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(HTTPAPIEX_Init());
}

static void setupCreateHappyPathHostname(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUB_NAME));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "."));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_IOTHUB_SUFFIX));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setupCreateHappyPathGWHostname(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct(TEST_IOTHUB_GWHOSTNAME));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setupCreateHappyPathApiExHandle(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_Create(TEST_IOTHUB_NAME "." TEST_IOTHUB_SUFFIX)).IgnoreArgument(1);
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG));
    }
}

static void setupCreateHappyPathPerDeviceList(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(VECTOR_create(IGNORED_NUM_ARG));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    }
}

static void setupCreateHappyPath(bool deallocateCreated)
{
    setupCreateHappyPathAlloc(deallocateCreated);
    setupCreateHappyPathHostname(deallocateCreated);
    setupCreateHappyPathApiExHandle(deallocateCreated);
    setupCreateHappyPathPerDeviceList(deallocateCreated);
}

static void setupUnregisterOneDevice()
{
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
}
static void setupRegisterHappyPathAllocHandle(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
}

static void setupRegisterHappyPathcreate_deviceId(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_ID));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setupRegisterHappyPathcreate_deviceKey(bool deallocateCreated, bool is_x509_used)
{
    if (is_x509_used == false)
    {
        STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_KEY));
        if (deallocateCreated == true)
        {
            STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        }
    }
}

static void setupRegisterHappyPathcreate_deviceSasToken(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_TOKEN));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setupRegisterHappyPatheventHTTPrelativePath(bool deallocateCreated)
{
    /*creating eventHTTPrelativePath*/
    STRICT_EXPECTED_CALL(STRING_construct("/devices/"));
    STRICT_EXPECTED_CALL(URL_EncodeString(TEST_DEVICE_ID));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, EVENT_ENDPOINT API_VERSION));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    /*the url encoded device id*/
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setupRegisterHappyPathmessageHTTPrelativePath(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct("/devices/"));
    STRICT_EXPECTED_CALL(URL_EncodeString(TEST_DEVICE_ID));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, MESSAGE_ENDPOINT_HTTP API_VERSION));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    /*the url encoded device id*/
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setupRegisterHappyPatheventHTTPrequestHeaders(bool deallocateCreated, bool is_x509_used)
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(STRING_construct("/devices/"));
    STRICT_EXPECTED_CALL(URL_EncodeString(TEST_DEVICE_ID));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, EVENT_ENDPOINT));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-to", "/devices/"  TEST_DEVICE_ID  EVENT_ENDPOINT));
    if (is_x509_used == false)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN));
    }
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Accept", "application/json"));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Connection", "Keep-Alive"));
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_PRODUCT_INFO));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setupRegisterHappyPathmessageHTTPrequestHeaders(bool deallocateCreated, bool is_x509_used)
{
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_PRODUCT_INFO));
    if (is_x509_used == false)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN));
    }
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    }
}

static void setupRegisterHappyPathabandonHTTPrelativePathBegin(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(STRING_construct("/devices/"));
    STRICT_EXPECTED_CALL(URL_EncodeString(TEST_DEVICE_ID));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, MESSAGE_ENDPOINT_HTTP_ETAG));
    if (deallocateCreated == true)
    {
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
}

static void setupRegisterHappyPathsasObject(bool deallocateCreated, bool is_x509_used)
{
    if (is_x509_used == false)
    {
        STRICT_EXPECTED_CALL(URL_EncodeString(TEST_DEVICE_ID));
        STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/devices/"));
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_construct(TEST_DEVICE_KEY));
        STRICT_EXPECTED_CALL(STRING_empty(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Create(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
        if (deallocateCreated)
        {
            STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_Destroy(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    }
}

static void setupRegisterHappyPatheventConfirmations()
{
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)).IgnoreAllArguments();
}

static void setupRegisterHappyPathDeviceListAdd()
{
    STRICT_EXPECTED_CALL(VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
}

static void setupRegisterHappyPathWithSasToken(bool deallocateCreated)
{
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setupRegisterHappyPathAllocHandle(deallocateCreated);
    setupRegisterHappyPathcreate_deviceId(deallocateCreated);
    setupRegisterHappyPathcreate_deviceSasToken(deallocateCreated);
    setupRegisterHappyPatheventHTTPrelativePath(deallocateCreated);
    setupRegisterHappyPathmessageHTTPrelativePath(deallocateCreated);
    setupRegisterHappyPatheventHTTPrequestHeaders(deallocateCreated, false);
    setupRegisterHappyPathmessageHTTPrequestHeaders(deallocateCreated, false);
    setupRegisterHappyPathabandonHTTPrelativePathBegin(deallocateCreated);
    setupRegisterHappyPathDeviceListAdd();
    setupRegisterHappyPatheventConfirmations();
}

static void setupRegisterHappyPath(bool deallocateCreated, bool is_x509_used)
{
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    setupRegisterHappyPathAllocHandle(deallocateCreated);
    setupRegisterHappyPathcreate_deviceId(deallocateCreated);
    setupRegisterHappyPathcreate_deviceKey(deallocateCreated, is_x509_used);
    setupRegisterHappyPatheventHTTPrelativePath(deallocateCreated);
    setupRegisterHappyPathmessageHTTPrelativePath(deallocateCreated);
    setupRegisterHappyPatheventHTTPrequestHeaders(deallocateCreated, is_x509_used);
    setupRegisterHappyPathmessageHTTPrequestHeaders(deallocateCreated, is_x509_used);
    setupRegisterHappyPathabandonHTTPrelativePathBegin(deallocateCreated);
    setupRegisterHappyPathsasObject(deallocateCreated, is_x509_used);
    setupRegisterHappyPathDeviceListAdd();
    setupRegisterHappyPatheventConfirmations();
}

static void setupDoWorkLoopOnceForOneDevice()
{
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
}

static void setupDoWorkLoopForNextDevice(size_t next)
{
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, next));
}

BEGIN_TEST_SUITE(iothubtransporthttp_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    (void)umock_c_init(on_umock_c_error);

    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.twin_retrieve_prop_complete_cb = Transport_Twin_RetrievePropertyComplete_Callback;
    transport_cb_info.twin_rpt_state_complete_cb = Transport_Twin_ReportedStateComplete_Callback;
    transport_cb_info.send_complete_cb = Transport_SendComplete_Callback;
    transport_cb_info.connection_status_cb = Transport_ConnectionStatusCallBack;
    transport_cb_info.prod_info_cb = Transport_GetOption_Product_Info_Callback;
    transport_cb_info.msg_input_cb = Transport_MessageCallbackFromInput;
    transport_cb_info.msg_cb = Transport_MessageCallback;
    transport_cb_info.method_complete_cb = Transport_DeviceMethod_Complete_Callback;

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(VECTOR_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PREDICATE_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CORE_LL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_SAS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTPAPI_REQUEST_TYPE, int);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION, void*);

    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_CONFIRMATION_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_MESSAGE_RESULT, int);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_calloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_RETURN(Transport_GetOption_Product_Info_Callback, TEST_PRODUCT_INFO);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Transport_GetOption_Product_Info_Callback, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_new, real_BUFFER_new);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_new, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_create, real_BUFFER_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_delete, real_BUFFER_delete);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_build, real_BUFFER_build);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_build, __LINE__);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, real_BUFFER_u_char);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, real_BUFFER_length);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_clone, real_BUFFER_clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(BUFFER_clone, NULL);

    REGISTER_STRING_GLOBAL_MOCK_HOOK;
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_clone, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_construct_n, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new_with_memory, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new_quoted, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_new_JSON, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat_with_STRING, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_quote, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_copy, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_copy_n, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_empty, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Create, my_HTTPAPIEX_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Init, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_Destroy, my_HTTPAPIEX_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_create, real_VECTOR_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_destroy, real_VECTOR_destroy);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_push_back, real_VECTOR_push_back);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_push_back, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_erase, real_VECTOR_erase);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_clear, real_VECTOR_clear);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_element, real_VECTOR_element);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_front, real_VECTOR_front);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_back, real_VECTOR_back);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_find_if, real_VECTOR_find_if);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(VECTOR_find_if, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(VECTOR_size, real_VECTOR_size);

    REGISTER_GLOBAL_MOCK_HOOK(URL_EncodeString, my_URL_EncodeString);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(URL_EncodeString, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_BYTEARRAY);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetContentType, IOTHUBMESSAGE_UNKNOWN);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_CreateFromByteArray, my_IoTHubMessage_CreateFromByteArray);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_CreateFromByteArray, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_GetByteArray, my_IoTHubMessage_GetByteArray);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetContentEncodingSystemProperty, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetContentEncodingSystemProperty, IOTHUB_MESSAGE_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetMessageId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetMessageId, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_SetCorrelationId, IOTHUB_MESSAGE_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_SetCorrelationId, IOTHUB_MESSAGE_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_Destroy, my_IoTHubMessage_Destroy);
    REGISTER_GLOBAL_MOCK_HOOK(IoTHubMessage_Properties, my_IoTHubMessage_Properties);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHubMessage_Properties, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Alloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_AddHeaderNameValuePair, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_GetHeader, my_HTTPHeaders_GetHeader);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_GetHeader, HTTP_HEADERS_ERROR);

    //REGISTER_GLOBAL_MOCK_HOOK(IoTHubClientCore_LL_MessageCallback, my_IoTHubClientCore_LL_MessageCallback);
    REGISTER_GLOBAL_MOCK_HOOK(Transport_MessageCallback, my_Transport_MessageCallback);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Create, my_HTTPAPIEX_SAS_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SAS_Create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_ExecuteRequest, my_HTTPAPIEX_ExecuteRequest);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_ExecuteRequest, HTTPAPIEX_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_ExecuteRequest, my_HTTPAPIEX_SAS_ExecuteRequest);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPAPIEX_SAS_ExecuteRequest, HTTPAPIEX_ERROR);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHub_Transport_ValidateCallbacks, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(IoTHub_Transport_ValidateCallbacks, __LINE__);

    REGISTER_GLOBAL_MOCK_RETURN(IoTHubMessage_IsSecurityMessage, false);

    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_FindHeaderValue, TEST_ETAG_VALUE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_FindHeaderValue, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_GetHeaderCount, my_HTTPHeaders_GetHeaderCount);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_GetHeaderCount, HTTP_HEADERS_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Clone, my_HTTPHeaders_Clone);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_Clone, NULL);
    REGISTER_GLOBAL_MOCK_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(HTTPHeaders_ReplaceHeaderNameValuePair, HTTP_HEADERS_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(Map_GetInternals, my_Map_GetInternals);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_GetInternals, MAP_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(Map_AddOrUpdate, MAP_OK);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(Map_AddOrUpdate, MAP_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPIEX_SAS_Destroy, my_HTTPAPIEX_SAS_Destroy);

    REGISTER_GLOBAL_MOCK_HOOK(DList_InitializeListHead, real_DList_InitializeListHead);
    REGISTER_GLOBAL_MOCK_HOOK(DList_IsListEmpty, real_DList_IsListEmpty);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertTailList, real_DList_InsertTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_InsertHeadList, real_DList_InsertHeadList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_AppendTailList, real_DList_AppendTailList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveEntryList, real_DList_RemoveEntryList);
    REGISTER_GLOBAL_MOCK_HOOK(DList_RemoveHeadList, real_DList_RemoveHeadList);

    bigBufferOverflow = (unsigned char*)my_gballoc_malloc(TEST_BIG_BUFFER_1_OVERFLOW_SIZE);
    memset(bigBufferOverflow, '3', TEST_BIG_BUFFER_1_OVERFLOW_SIZE);
    bigBufferFit = (unsigned char*)my_gballoc_malloc(TEST_BIG_BUFFER_1_FIT_SIZE);
    memset(bigBufferFit, '3', TEST_BIG_BUFFER_1_FIT_SIZE);

    unsigned char* temp = (unsigned char*)my_gballoc_malloc(buffer9_size);
    memset(temp, '3', TEST_BIG_BUFFER_9_OVERFLOW_SIZE);
    buffer9 = temp;

    temp = (unsigned char*)my_gballoc_malloc(buffer11_size);
    memset(temp, '3', buffer11_size);
    buffer11 = temp;

    IoTHubTransportHttp_SendMessageDisposition = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_SendMessageDisposition;
    IoTHubTransportHttp_Unsubscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Unsubscribe_DeviceTwin;
    IoTHubTransportHttp_Subscribe_DeviceTwin = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Subscribe_DeviceTwin;
    IoTHubTransportHttp_GetTwinAsync = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_GetTwinAsync;
    IoTHubTransportHttp_GetHostname = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_GetHostname;
    IoTHubTransportHttp_SetOption = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_SetOption;
    IoTHubTransportHttp_Create = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Create;
    IoTHubTransportHttp_Destroy = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Destroy;
    IoTHubTransportHttp_Register = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Register;
    IoTHubTransportHttp_Unregister = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Unregister;
    IoTHubTransportHttp_Subscribe = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Subscribe;
    IoTHubTransportHttp_Unsubscribe = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_Unsubscribe;
    IoTHubTransportHttp_DoWork = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_DoWork;
    IoTHubTransportHttp_GetSendStatus = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_GetSendStatus;
    IoTHubTransportHttp_SetCallbackContext = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_SetCallbackContext;
    IoTHubTransportHttp_GetSupportedPlatformInfo = ((TRANSPORT_PROVIDER*)HTTP_Protocol())->IoTHubTransport_GetSupportedPlatformInfo;

    TEST_STRING_HANDLE = real_STRING_construct(TEST_STRING_DATA);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);

    my_gballoc_free((void*)buffer9);
    my_gballoc_free((void*)buffer11);
    my_gballoc_free((void*)bigBufferFit);
    my_gballoc_free((void*)bigBufferOverflow);
    real_STRING_delete(TEST_STRING_HANDLE);
}

static void reset_test_data()
{
    last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = NULL;
    my_IoTHubClientCore_LL_MessageCallback_messageHandle = NULL;
}

typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG
{
    char* etagValue;
} MESSAGE_DISPOSITION_CONTEXT;

static MESSAGE_DISPOSITION_CONTEXT* make_transport_context_data(void* thd, void* dd)
{
    MESSAGE_DISPOSITION_CONTEXT* tc;

    if (thd == NULL && dd == NULL)
    {
        tc = NULL;
    }
    else
    {
        tc = (MESSAGE_DISPOSITION_CONTEXT*)malloc(sizeof(MESSAGE_DISPOSITION_CONTEXT));
        tc->etagValue = (char*)malloc(20);
        sprintf(tc->etagValue, "Hello World");
    }

    return tc;
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    umock_c_reset_all_calls();

    real_DList_InitializeListHead(&waitingToSend);
    real_DList_InitializeListHead(&waitingToSend2);
    reset_test_data();
    my_IoTHubClientCore_LL_MessageCallback_return_value = true;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
    {
        real_BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
    }

    reset_test_data();
    TEST_MUTEX_RELEASE(g_testByTest);
}

static int should_skip_index(size_t current_index, const size_t skip_array[], size_t length)
{
    int result = 0;
    for (size_t index = 0; index < length; index++)
    {
        if (current_index == skip_array[index])
        {
            result = __LINE__;
            break;
        }
    }
    return result;
}

// Testing the values of constants allows me to use them throughout the unit tests (e.g., when setting expectations on mocks) without fear of missing a bug.

TEST_FUNCTION(IotHubTransportHttp_EVENT_ENDPOINT_constant_is_expected_value)
{
    ASSERT_ARE_EQUAL(char_ptr, "/messages/events", EVENT_ENDPOINT);
}

TEST_FUNCTION(IotHubTransportHttp_MESSAGE_ENDPOINT_HTTP_constant_is_expected_value)
{
    ASSERT_ARE_EQUAL(char_ptr, "/messages/devicebound", MESSAGE_ENDPOINT_HTTP);
}

TEST_FUNCTION(IotHubTransportHttp_MESSAGE_ENDPOINT_HTTP_ETAG_constant_is_expected_value)
{
    ASSERT_ARE_EQUAL(char_ptr, "/messages/devicebound/", MESSAGE_ENDPOINT_HTTP_ETAG);
}

TEST_FUNCTION(IotHubTransportHttp_API_VERSION_constant_is_expected_value)
{
    ASSERT_ARE_EQUAL(char_ptr, "?api-version=2016-11-14", API_VERSION);
}


TEST_FUNCTION(IoTHubTransportHttp_Create_with_NULL_parameter_fails)
{
    // arrange

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(NULL, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);

}

TEST_FUNCTION(IoTHubTransportHttp_Create_with_NULL_config_parameter_fails)
{
    // arrange

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG_NULL_CONFIG, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_with_NULL_protocol_parameter_fails)
{
    // arrange

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG_NULL_PROTOCOL, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_with_NULL_iothub_name_fails)
{
    // arrange

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG_NULL_IOTHUB_NAME, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_with_NULL_iothub_suffix_fails)
{
    // arrange

    // act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG_NULL_IOTHUB_SUFFIX, &transport_cb_info, transport_cb_ctx);

    // assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_happy_path)
{
    //arrange
    setupCreateHappyPath(false);

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_happy_path_with_gwhostname)
{
    //arrange
    setupCreateHappyPathAlloc(false);
    setupCreateHappyPathGWHostname(false);
    setupCreateHappyPathApiExHandle(false);
    setupCreateHappyPathPerDeviceList(false);

    //act
    TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_GW_CONFIG, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(result);
}

TEST_FUNCTION(IoTHubTransportHttp_Create_fails)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    setupCreateHappyPathAlloc(false);
    setupCreateHappyPathHostname(false);
    setupCreateHappyPathApiExHandle(false);
    setupCreateHappyPathPerDeviceList(false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 6 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubTransportHttp_Create failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        TRANSPORT_LL_HANDLE result = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

        //assert
        ASSERT_IS_NULL(result, tmp_msg);
    }

    //cleanup
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransportHttp_Destroy_with_NULL_handle_does_nothing)
{
    //arrange

    //act
    IoTHubTransportHttp_Destroy(NULL);

    //assert
}

TEST_FUNCTION(IoTHubTransportHttp_Destroy_succeeds_NoRegistered_devices)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));                                             //STRING_HANDLE hostName;
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG));                                             //HTTPAPIEX_HANDLE httpApiExHandle;
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(handle));

    //act
    IoTHubTransportHttp_Destroy(handle);

    //assert
}

TEST_FUNCTION(IoTHubTransportHttp_Destroy_succeeds_One_Registered_device)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(VECTOR_element(IGNORED_PTR_ARG, 0));
    setupUnregisterOneDevice();
    STRICT_EXPECTED_CALL(gballoc_free(devHandle));

    STRICT_EXPECTED_CALL(VECTOR_destroy(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(handle));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_Destroy(handle);

    //assert
}

TEST_FUNCTION(IoTHubTransportHttp_Register_HappyPath_with_deviceKey_success_fun_time)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setupRegisterHappyPath(false, false);

    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_NOT_EQUAL(void_ptr, handle, devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_HappyPath_with_deviceSas_success_fun_time)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setupRegisterHappyPathWithSasToken(false);

    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_3, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_NOT_EQUAL(void_ptr, handle, devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_2nd_device_HappyPath_success_fun_time)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    umock_c_reset_all_calls();

    // find in vector..
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    setupRegisterHappyPathAllocHandle(false);
    setupRegisterHappyPathcreate_deviceId(false);
    setupRegisterHappyPathcreate_deviceKey(false, false);
    setupRegisterHappyPatheventHTTPrelativePath(false);
    setupRegisterHappyPathmessageHTTPrelativePath(false);
    setupRegisterHappyPatheventHTTPrequestHeaders(false, false);
    setupRegisterHappyPathmessageHTTPrequestHeaders(false, false);
    setupRegisterHappyPathabandonHTTPrelativePathBegin(false);
    setupRegisterHappyPathsasObject(false, false);
    setupRegisterHappyPathDeviceListAdd();
    setupRegisterHappyPatheventConfirmations();

    // actual register
    //setupRegisterHappyPath(false, false);

    //act
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NOT_NULL(devHandle2);
    ASSERT_ARE_NOT_EQUAL(void_ptr, handle, devHandle1);
    ASSERT_ARE_NOT_EQUAL(void_ptr, devHandle1, devHandle2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_sameDevice_twice_returns_null)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    // find in vector.. 2 and 1a
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));

    //act
    IOTHUB_DEVICE_HANDLE devHandle1b = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NULL(devHandle1b);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_HappyPath_with_deviceKey_fail)
{
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setupRegisterHappyPath(false, false);

    umock_c_negative_tests_snapshot();

    size_t calls_cannot_fail[] = { 0, 8, 13, 19, 24, 26, 27, 29, 36, 44, 45, 46, 47, 48, 51 };

    //act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (should_skip_index(index, calls_cannot_fail, sizeof(calls_cannot_fail) / sizeof(calls_cannot_fail[0])) != 0)
        {
            continue;
        }

        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(index);

        char tmp_msg[64];
        sprintf(tmp_msg, "IoTHubMessage_CreateFromByteArray failure in test %lu/%lu", (unsigned long)index, (unsigned long)count);

        IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

        //assert
        ASSERT_IS_NULL(devHandle, tmp_msg);
    }

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION(IoTHubTransportHttp_Register_deviceFoundInList_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG)).SetReturn((void_ptr)0x1);

    //act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_deviceId_null_returns_null)
{
    //arrange
    IOTHUB_DEVICE_CONFIG myDevice;
    myDevice.deviceId = NULL;
    myDevice.deviceKey = TEST_DEVICE_KEY;
    myDevice.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &myDevice, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);

}

TEST_FUNCTION(IoTHubTransportHttp_Register_deviceKey_null_and_deviceSasToken_succeeds)
{
    //arrange
    IOTHUB_DEVICE_CONFIG myDevice;
    myDevice.deviceId = TEST_DEVICE_ID;
    myDevice.deviceKey = NULL;
    myDevice.deviceSasToken = NULL;

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    setupRegisterHappyPath(false, true);

    //act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &myDevice, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NOT_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Register_wts_null_returns_null)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, NULL);

    //assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);

}

TEST_FUNCTION(IoTHubTransportHttp_Register_handle_null_returns_null)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(NULL, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    //assert
    ASSERT_IS_NULL(devHandle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);

}

TEST_FUNCTION(IoTHubTransportHttp_Unregister_null_does_nothing)
{
    //arrange


    //act
    IoTHubTransportHttp_Unregister(NULL);
}

TEST_FUNCTION(IoTHubTransportHttp_Unregister_superHappyFunPath)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    setupUnregisterOneDevice();
    STRICT_EXPECTED_CALL(VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(gballoc_free(devHandle));

    //act
    IoTHubTransportHttp_Unregister(devHandle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unregister_2nd_device_superHappyFunPath)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle1));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    setupUnregisterOneDevice();
    STRICT_EXPECTED_CALL(VECTOR_erase(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1));
    STRICT_EXPECTED_CALL(gballoc_free(devHandle1));

    //act
    IoTHubTransportHttp_Unregister(devHandle1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unregister_DeviceNotFound_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle)).SetReturn((void_ptr)NULL);

    //act
    IoTHubTransportHttp_Unregister(devHandle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Subscribe_with_NULL_parameter_fails)
{
    //arrange

    //act
    int result = IoTHubTransportHttp_Subscribe(NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

TEST_FUNCTION(IoTHubTransportHttp_Subscribe_with_non_NULL_parameter_succeeds)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle));

    //act
    int result = IoTHubTransportHttp_Subscribe(devHandle);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Subscribe_2devices_succeeds)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle1));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle2));

    //act
    int result1 = IoTHubTransportHttp_Subscribe(devHandle1);
    int result2 = IoTHubTransportHttp_Subscribe(devHandle2);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result1);
    ASSERT_ARE_EQUAL(int, 0, result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Subscribe_with_list_find_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle))
        .SetReturn((void_ptr)NULL);

    //act
    int result = IoTHubTransportHttp_Subscribe(devHandle);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unsubscribe_with_NULL_parameter_succeeds) /*does nothing*/
{
    //arrange

    //act
    IoTHubTransportHttp_Unsubscribe(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubTransportHttp_Unsubscribe_with_non_NULL_parameter_succeeds)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle));

    //act
    IoTHubTransportHttp_Unsubscribe(devHandle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unsubscribe_with_2devices_succeeds)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle));
    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle2));

    //act
    IoTHubTransportHttp_Unsubscribe(devHandle);
    IoTHubTransportHttp_Unsubscribe(devHandle2);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unsubscribe_with_device_not_found_does_nothing)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle))
        .SetFailReturn((void_ptr)NULL);

    //act
    IoTHubTransportHttp_Unsubscribe(devHandle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_with_NULL_handle_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_with_NULL_handle_data_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_with_NULL_message_data_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_with_NULL_message_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, NULL, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_get_disposition_invalid_arg_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_MESSAGE_HANDLE test_message = (IOTHUB_MESSAGE_HANDLE)my_gballoc_malloc(1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_MESSAGE_INVALID_ARG);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, test_message, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SendMessageDisposition_with_NULL_disposition_info_fails)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_MESSAGE_HANDLE test_message = (IOTHUB_MESSAGE_HANDLE)my_gballoc_malloc(100);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(IoTHubMessage_GetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    //act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_SendMessageDisposition(devHandle1, test_message, IOTHUBMESSAGE_ACCEPTED);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_NULL_handle_does_nothing)
{
    //arrange

    //act
    IoTHubTransportHttp_DoWork(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_no_devices)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));

    /*no message work because DoWork_PullMessage is set to false by create*/

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_all_devices_unregistered)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    (void)IoTHubTransportHttp_Unregister(devHandle1);
    (void)IoTHubTransportHttp_Unregister(devHandle2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_size(IGNORED_PTR_ARG));

    /*no message work because DoWork_PullMessage is set to false by create*/

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_no_service_messages)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    /*no message work because DoWork_PullMessage is set to false by create*/

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_no_work_check_unsubscribe)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle2 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);
    (void)IoTHubTransportHttp_Subscribe(devHandle2);
    (void)IoTHubTransportHttp_Subscribe(devHandle1);
    IoTHubTransportHttp_Unsubscribe(devHandle2);
    IoTHubTransportHttp_Unsubscribe(devHandle1);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    setupDoWorkLoopForNextDevice(1);
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend2));

    /*no message work because DoWork_PullMessage is unset due to unsubscribe*/

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_2_registered_devices_empty_wts_no_service_msgs)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    setupDoWorkLoopForNextDevice(1);
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend2));

    /*no message work because DoWork_PullMessage is set to false by create*/

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

//requestType: GET
//    relativePath : the message HTTP relative path
//    requestHttpHeadersHandle : message HTTP request headers created by _Create
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which shall be later examined
//    responseHeadearsHandle : a new instance of HTTP headers
//    responseContent : a new instance of buffer ]
//
//requestType: DELETE
//    relativePath : abandon relative path begin + value of ETag + APIVERSION
//    requestHttpHeadersHandle : an HTTP headers instance containing the following
//    Authorization : " "
//    If - Match : value of ETag
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which might be used by logging
//    responseHeadearsHandle : NULL
//    responseContent : NULL ]
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fail_set_disposition_fails)
{
    //arrange
    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;

    char* real_ETAG = (char*)malloc(sizeof(TEST_ETAG_VALUE) + 1);
    (void)sprintf(real_ETAG, "%s", TEST_ETAG_VALUE);

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"));

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // This is where it fails.
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .CopyOutArgumentBuffer_destination(&real_ETAG, sizeof(&real_ETAG));;
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .SetReturn(IOTHUB_MESSAGE_INVALID_ARG);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));


    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_PRODUCT_INFO));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE));

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,*/
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/**/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_async_and_1_service_malloc_fails)
{
    //arrange
    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_PRODUCT_INFO));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_async_and_2nd_service_malloc_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    // DoEvents
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    // DoMessages
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    // This is where it fails.
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG));

    // And then the message is just abandoned.
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_PRODUCT_INFO));
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/**/
#if 0
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_async_and_1_service_MessageClone_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .SetReturn((IOTHUB_MESSAGE_HANDLE)NULL);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

//requestType: GET
//    relativePath : the message HTTP relative path
//    requestHttpHeadersHandle : message HTTP request headers created by _Create
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which shall be later examined
//    responseHeadearsHandle : a new instance of HTTP headers
//    responseContent : a new instance of buffer ]
//
//requestType: DELETE
//    relativePath : abandon relative path begin + value of ETag + APIVERSION
//    requestHttpHeadersHandle : an HTTP headers instance containing the following
//    Authorization : " "
//    If - Match : value of ETag
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which might be used by logging
//    responseHeadearsHandle : NULL
//    responseContent : NULL ]
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_2device_with_2nd_empty_waitingToSend_and_1_service_message_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle2);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    setupDoWorkLoopForNextDevice(1);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /* DoWork DoEvent for device 1*/

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend2)); /* DoWork DoEvent for device 1*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID2 MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);


    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID2 MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_2devices_2nd_has_1_event_item_as_string_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend2), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    setupDoWorkLoopForNextDevice(1);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend2));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"base64Encoded\":false")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message10.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message10.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID2 EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_2devices_2subscriptions_happy_path_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    auto devHandle1 = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_2, TEST_CONFIG2.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle2);
    (void)IoTHubTransportHttp_Subscribe(devHandle1);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();
    setupDoWorkLoopForNextDevice(1);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /* DoWork DoEvent for device 1*/

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend2)); /* DoWork DoEvent for device 1*/

    // Device 1
    {
        STRICT_EXPECTED_CALL(get_time(NULL));
        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
            .IgnoreArgument(1)
            .SetReturn(TEST_ETAG_VALUE);

        STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            ;
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*this returns "0" so the message needs to be "accepted"*/
        /*this is "accepting"*/
        STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
            .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            NULL                                                /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    //device 2
    {
        STRICT_EXPECTED_CALL(get_time(NULL));
        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID2 MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
            .IgnoreArgument(1)
            .SetReturn(TEST_ETAG_VALUE);

        STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            ;
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*this returns "0" so the message needs to be "accepted"*/
        /*this is "accepting"*/
        STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
            .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID2 MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            NULL                                                /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);

}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_when_time_is_minus_one_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn((time_t)(-1));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_2_service_message_when_time_is_minus_one_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    for (int i = 0; i < 2; i++)
    {
        setupDoWorkLoopOnceForOneDevice();

        STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

        STRICT_EXPECTED_CALL(get_time(NULL))
            .SetReturn((time_t)(-1));
        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

        STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
            .IgnoreArgument(1)
            .SetReturn(TEST_ETAG_VALUE);

        STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            ;
        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*this returns "0" so the message needs to be "accepted"*/
        /*this is "accepting"*/
        STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreAllArguments();

        STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
            .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
        STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
        STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
            IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
            IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
            HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
            "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
            IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
            NULL,                                               /*BUFFER_HANDLE requestContent,                                */
            IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
            NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
            NULL                                                /*BUFFER_HANDLE responseContent))                              */
            ))
            .IgnoreArgument_requestType()
            .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

        STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    //act
    for (int i = 0; i < 2; i++)
    {
        IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_non_default_minimumPollingTime_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    unsigned int thisIs20Seconds = 20;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransportHttp_SetOption(handle, OPTION_MIN_POLLING_TIME, &thisIs20Seconds);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();

    /*everything below is for the second time this is called*/

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_later_than_minimumPollingTime_does_nothing_succeeds)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    /*everything below is for the second time _DoWork this is called*/

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn(TEST_GET_TIME_VALUE + TEST_DEFAULT_GETMINIMUMPOLLINGTIME); /*right on the verge of the time*/
    STRICT_EXPECTED_CALL(get_difftime(TEST_GET_TIME_VALUE + TEST_DEFAULT_GETMINIMUMPOLLINGTIME, TEST_GET_TIME_VALUE));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_immediately_after_minimumPollingTime_polls_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    /*everything below is for the second time _DoWork this is called*/

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL))
        .SetReturn(TEST_GET_TIME_VALUE + TEST_DEFAULT_GETMINIMUMPOLLINGTIME + 1); /*right on the verge of the time*/
    STRICT_EXPECTED_CALL(get_difftime(TEST_GET_TIME_VALUE + TEST_DEFAULT_GETMINIMUMPOLLINGTIME + 1, TEST_GET_TIME_VALUE));

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
/*purpose of this test is to see that gremlins don't emerge when the http return code is 404 from the service*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_accept_code_404_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode404 = 404;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode404, sizeof(statusCode404));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_httpapi_execute_request_failed_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .SetReturn(HTTPAPIEX_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_HTTPHeaders_AddHeaderNameValuePair_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        ;
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_HTTPHeaders_AddHeaderNameValuePair_succeeds_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_HTTPHeaders_Alloc_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    whenShallHTTPHeaders_Alloc_fail = currentHTTPHeaders_Alloc_call + 2;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_STRING_concat_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    whenShallSTRING_concat_fail = currentSTRING_concat_call + 1;
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_STRING_concat_with_STRING_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallSTRING_concat_with_STRING_fail = currentSTRING_concat_with_STRING_call + 1;
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_STRING_construct_n_succeeds_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    whenShallSTRING_construct_n_fail = currentSTRING_construct_n_call + 1;
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*undefined behavior*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_acceptmessage_fails_at_STRING_clone_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    whenShallSTRING_clone_fail = currentSTRING_clone_call + 1;
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

//
//requestType: POST
//    relativePath : abandon relative path begin(as created by _Create) + value of ETag + "/abandon" + APIVERSION
//    requestHttpHeadersHandle : an HTTP headers instance containing the following
//    Authorization : " "
//    If - Match : value of ETag
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which might be examined for logging
//    responseHeadearsHandle : NULL
//    responseContent : NULL ]
//
//requestType: DELETE
//    relativePath : abandon relative path begin + value of ETag + APIVERSION
//    requestHttpHeadersHandle : an HTTP headers instance containing the following
//    Authorization : " "
//    If - Match : value of ETag
//    requestContent : NULL
//    statusCode : a pointer to unsigned int which might be used by logging
//    responseHeadearsHandle : NULL
//    responseContent : NULL ]

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_reject_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION REJECT_QUERY_PARAMETER,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_abandon_statusCode404_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode404 = 404;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION REJECT_QUERY_PARAMETER,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode404, sizeof(statusCode404));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_HTTPAPIEX_SAS_ExecuteRequest2_fails_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode404 = 404;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION REJECT_QUERY_PARAMETER,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode404, sizeof(statusCode404))
        .SetReturn(HTTPAPIEX_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_HTTPHeaders_AddHeaderNameValuePair_fails_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_HTTPHeaders_AddHeaderNameValuePair_fails_succeeds_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

                                            /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_HTTPHeaders_Alloc_fails_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    whenShallHTTPHeaders_Alloc_fail = currentHTTPHeaders_Alloc_call + 2;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_STRING_concat_fails_succeeds_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_REJECTED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    whenShallSTRING_concat_fail = currentSTRING_concat_call + 1;
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION REJECT_QUERY_PARAMETER))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_STRING_concat_with_STRING_fails_succeeds_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    whenShallSTRING_concat_with_STRING_fail = currentSTRING_concat_with_STRING_call + 1;
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_STRING_construct_n_fails_succeeds_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    whenShallSTRING_construct_n_fail = currentSTRING_construct_n_call + 1;
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_STRING_clone_fails_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "reject"*/
    whenShallSTRING_clone_fail = currentSTRING_clone_call + 1;
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_abandons_when_IoTHubMessage_Create_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn((IOTHUB_MESSAGE_HANDLE)NULL);

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_no_ETag_header_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn((const char*)NULL);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_ETag_zero_characters_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn("");

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_ETag_1_characters_not_quote_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn("a");

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_ETag_2_characters_last_not_quote_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn("\"a");

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_ETag_1_characters_quote_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn("\"");

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_fails_when_ETag_many_last_not_quote_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn("\"abcd");

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_goes_top_next_action_when_httpstatus_is_500_fails)
{
    //arrange

    unsigned int statusCode500 = 500;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode500, sizeof(statusCode500));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_goes_to_next_action_when_HTTPAPIEX_SAS_ExecuteRequest2_fails)
{
    //arrange

    unsigned int statusCode200 = 200;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200))
        .SetReturn(HTTPAPIEX_ERROR);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_goes_to_next_action_when_BUFFER_new_fails)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    whenShallBUFFER_new_fail = currentBUFFER_new_call + 1;
    STRICT_EXPECTED_CALL(BUFFER_new());

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_goes_to_next_action_when_HTTPHeaders_Alloc_fails)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    whenShallHTTPHeaders_Alloc_fail = currentHTTPHeaders_Alloc_call + 1;
    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

                                                      //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(HTTP_Protocol_succeeds)
{
    //arrange

    //act
    auto result = HTTP_Protocol();

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_SendMessageDisposition, (void*)IoTHubTransportHttp_SendMessageDisposition);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Subscribe_DeviceMethod, (void*)IoTHubTransportHttp_Subscribe_DeviceMethod);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Unsubscribe_DeviceMethod, (void*)IoTHubTransportHttp_Unsubscribe_DeviceMethod);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_DeviceMethod_Response, (void*)IoTHubTransportHttp_DeviceMethod_Response);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Subscribe_DeviceTwin, (void*)IoTHubTransportHttp_Subscribe_DeviceTwin);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Unsubscribe_DeviceTwin, (void*)IoTHubTransportHttp_Unsubscribe_DeviceTwin);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_ProcessItem, (void*)IoTHubTransportHttp_ProcessItem);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_GetHostname, (void*)IoTHubTransportHttp_GetHostname);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_SetOption, (void*)IoTHubTransportHttp_SetOption);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Create, (void*)IoTHubTransportHttp_Create);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Destroy, (void*)IoTHubTransportHttp_Destroy);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Register, (void*)IoTHubTransportHttp_Register);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Unregister, (void*)IoTHubTransportHttp_Unregister);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Subscribe, (void*)IoTHubTransportHttp_Subscribe);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Unsubscribe, (void*)IoTHubTransportHttp_Unsubscribe);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_DoWork, (void*)IoTHubTransportHttp_DoWork);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_SetRetryPolicy, (void*)IoTHubTransportHttp_SetRetryPolicy);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_GetSendStatus, (void*)IoTHubTransportHttp_GetSendStatus);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Subscribe_InputQueue, (void*)IotHubTransportHttp_Subscribe_InputQueue);
    ASSERT_ARE_EQUAL(void_ptr, (void*)((TRANSPORT_PROVIDER*)result)->IoTHubTransport_Unsubscribe_InputQueue, (void*)IotHubTransportHttp_Unsubscribe_InputQueue);

    //cleanup
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();


    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_puts_it_back_when_http_status_is_404)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus404, sizeof(httpStatus404));

    STRICT_EXPECTED_CALL(DList_AppendTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)).IgnoreAllArguments();

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_puts_it_back_when_HTTPAPIEX_SAS_ExecuteRequest2_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200))
        .SetReturn(HTTPAPIEX_ERROR);

    STRICT_EXPECTED_CALL(DList_AppendTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)).IgnoreAllArguments();

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_puts_it_back_when_BUFFER_build_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        whenShallBUFFER_build_fail = 1;
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .SetReturn(MU_FAILURE);
    }

    STRICT_EXPECTED_CALL(DList_AppendTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)).IgnoreAllArguments();

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_puts_it_back_when_BUFFER_new_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        whenShallBUFFER_new_fail = 1;
        STRICT_EXPECTED_CALL(BUFFER_new());
    }

    STRICT_EXPECTED_CALL(DList_AppendTailList(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_PTR_ARG)).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_PTR_ARG)).IgnoreAllArguments();

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_concat_with_STRING_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        whenShallSTRING_concat_with_STRING_fail = 7;
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_concat_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);


        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        whenShallSTRING_concat_fail = currentSTRING_concat_call + 2;
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_concat_it_fails_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);


        whenShallSTRING_concat_fail = currentSTRING_concat_call + 1;
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_concat_with_STRING_it_fails_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        whenShallSTRING_concat_with_STRING_fail = 6;
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_Azure_Base64_Encode_Bytes_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        whenShallAzure_Base64_Encode_Bytes_fail = 1;
        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_IoTHubMessage_GetByteArray_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .SetReturn(IOTHUB_MESSAGE_ERROR);

        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_construct_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        whenShallSTRING_construct_fail = currentSTRING_construct_call + 2;
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_STRING_construct_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    whenShallSTRING_construct_fail = currentSTRING_construct_call + 1;
    STRICT_EXPECTED_CALL(STRING_construct("["));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_when_HTTP_headers_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_bigger_than_256K_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message4.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message4.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message4.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(bigBufferOverflow, TEST_BIG_BUFFER_1_OVERFLOW_SIZE));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message4.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        /*building the list of messages to be notified because this is 100% fail (>256K)*/
        STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message4.entry)))
            .IgnoreArgument(1);
    }

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

/*this is a test that wants to see that "almost" 255KB message still fits*/
TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_almost255_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message5.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message5.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message5.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(bigBufferFit, TEST_BIG_BUFFER_1_FIT_SIZE));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message5.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message5.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_makes_1_batch_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    DList_InsertTailList(&(waitingToSend), &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG))
        .ExpectedAtLeastTimes(2);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    /*this is second batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message2.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message2.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer2, buffer2_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message2.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the second payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    {
        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message2.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_when_the_second_items_fails_the_first_one_still_makes_1_batch_succeeds_1)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    DList_InsertTailList(&(waitingToSend), &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG))
        .ExpectedAtLeastTimes(2);
    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    /*this is second batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message2.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message2.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer2, buffer2_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message2.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        whenShallSTRING_concat_fail = currentSTRING_concat_call + 4;
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the second batched payload*/
    }

    {
        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_when_the_second_items_fails_the_first_one_still_makes_1_batch_succeeds_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    DList_InsertTailList(&(waitingToSend), &(message2.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG))
        .ExpectedAtLeastTimes(2);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);;
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    /*this is second batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message2.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message2.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer2, buffer2_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message2.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the second batched payload*/
    }

    {
        /*adding the second payload to the "big" payload*/
        whenShallSTRING_concat_with_STRING_fail = currentSTRING_concat_with_STRING_call + 4;
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    {
        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_the_second_one_does_not_fit_256K_makes_1_batch_of_the_first_item_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    DList_InsertTailList(&(waitingToSend), &(message5.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG))
        .ExpectedAtLeastTimes(2);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message1.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message1.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(buffer1, buffer1_size));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message1.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    /*this is second batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message5.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(message5.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL(Azure_Base64_Encode_Bytes(bigBufferFit, TEST_BIG_BUFFER_1_FIT_SIZE));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message5.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the second payload to the "big" payload*/
    }

    {
        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);
    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}


/*** IoTHubTransportHttp_GetSendStatus ***/

TEST_FUNCTION(IoTHubTransportHttp_GetSendStatus_InvalidHandleArgument_fail)
{
    // arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_GetSendStatus(NULL, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetSendStatus_InvalidStatusArgument_fail)
{
    // arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_GetSendStatus(devHandle, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetSendStatus_empty_waitingToSend_and_empty_eventConfirmations_success)
{
    // arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_GetSendStatus(devHandle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, status, IOTHUB_CLIENT_SEND_STATUS_IDLE);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetSendStatus_waitingToSend_not_empty_success)
{
    // arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    IOTHUB_MESSAGE_HANDLE eventMessageHandle = IoTHubMessage_CreateFromByteArray(contains3, 1);
    IOTHUB_MESSAGE_LIST newEntry;
    newEntry.messageHandle = eventMessageHandle;
    DList_InsertTailList(&(waitingToSend), &(newEntry.entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_GetSendStatus(devHandle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_OK);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_STATUS, status, IOTHUB_CLIENT_SEND_STATUS_BUSY);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
    IoTHubMessage_Destroy(eventMessageHandle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetSendStatus_deviceData_is_not_found_fails)
{
    // arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    IOTHUB_MESSAGE_HANDLE eventMessageHandle = IoTHubMessage_CreateFromByteArray(contains3, 1);
    IOTHUB_MESSAGE_LIST newEntry;
    newEntry.messageHandle = eventMessageHandle;
    DList_InsertTailList(&(waitingToSend), &(newEntry.entry));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(VECTOR_find_if(IGNORED_PTR_ARG, IGNORED_PTR_ARG, devHandle))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetFailReturn((void_ptr)NULL);

    IOTHUB_CLIENT_STATUS status;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubTransportHttp_GetSendStatus(devHandle, &status);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, result, IOTHUB_CLIENT_INVALID_ARG);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
    IoTHubMessage_Destroy(eventMessageHandle);
}

void setupIrrelevantMocksForProperties(CIoTHubTransportHttpMocks *IOTHUB_MESSAGE_HANDLE messageHandle) /*these are copy pasted from TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items))*/
{
    (void)(*mocks);
    STRICT_EXPECTED_CALL((*mocks), DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL((*mocks), HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL((*mocks), STRING_construct("["));
    STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetContentType(messageHandle));
        STRICT_EXPECTED_CALL((*mocks), STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetByteArray(messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        if(messageHandle == TEST_IOTHUB_MESSAGE_HANDLE_11)
        {
            STRICT_EXPECTED_CALL((*mocks), Azure_Base64_Encode_Bytes(buffer11, buffer11_size));

        }
        else
        {
            STRICT_EXPECTED_CALL((*mocks), Azure_Base64_Encode_Bytes(buffer6, buffer6_size));
        }

        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL((*mocks), DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    if(messageHandle == TEST_IOTHUB_MESSAGE_HANDLE_11)
    {
        STRICT_EXPECTED_CALL((*mocks), DList_InsertTailList(IGNORED_PTR_ARG, &(message11.entry)))
            .IgnoreArgument(1);
    }
    else
    {
        STRICT_EXPECTED_CALL((*mocks), DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)))
            .IgnoreArgument(1);
    }


    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL((*mocks), BUFFER_new());
        STRICT_EXPECTED_CALL((*mocks), BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL((*mocks), HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL((*mocks), Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_with_properties_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    setupIrrelevantMocksForProperties(&message6.messageHandle);

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message6.messageHandle));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"properties\":"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "{"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"iothub-app-"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_VALUES1[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\":\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS1[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "}"))
        .IgnoreArgument(1);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_with_1_properties_at_maximum_message_size_succeeds)
{
    /*this test shall use a message that has a payload of MAXIMUM_MESSAGE_SIZE - PAYLOAD_OVERHEAD - PROPERTY_OVERHEAD - 2*/
    /*it will also have a property of "a":"b", therefore reaching the MAXIMUM_MESSAGE_SIZE*/
    /*the next test will increase either the propertyname or value by 1 character as the message is expected to fail*/
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message11.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    setupIrrelevantMocksForProperties(&message11.messageHandle);

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message11.messageHandle));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY_A_B, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"properties\":"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "{"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"iothub-app-"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_VALUES1_A_B[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\":\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS1_A_B[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "}"))
        .IgnoreArgument(1);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_with_1_properties_past_maximum_message_size_fails)
{
    /*this test shall use a message that has a payload of MAXIMUM_MESSAGE_SIZE - PAYLOAD_OVERHEAD - PROPERTY_OVERHEAD - 2*/
    /*it will also have a property of "aa":"b", therefore reaching 1 byte past the MAXIMUM_MESSAGE_SIZE*/
    /*this is done in a very e2e manner*/
    //arrange
    CNiceCallComparer<CIoTHubTransportHttpMocks>mocks;
    DList_InsertTailList(&(waitingToSend), &(message12.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    ENABLE_BATCHING();


    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items_fails_when_Map_GetInternals_fails)
{
    //arrange
    CNiceCallComparer<CIoTHubTransportHttpMocks> mocks;
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4)
        .SetReturn(MAP_ERROR);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_IS_NULL(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());


    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

void setupIrrelevantMocksForProperties2(CIoTHubTransportHttpMocks *IOTHUB_MESSAGE_HANDLE h1, IOTHUB_MESSAGE_HANDLE h2) /*these are copy pasted from TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_items))*/
{
    (void)(*mocks);
    STRICT_EXPECTED_CALL((*mocks), DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL((*mocks), HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL((*mocks), STRING_construct("["));
    STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetContentType(h1));
        STRICT_EXPECTED_CALL((*mocks), STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetByteArray(message6.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL((*mocks), Azure_Base64_Encode_Bytes(buffer6, buffer6_size));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
    }

    /*this is second batched payload*/
    {
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetContentType(h2));
        STRICT_EXPECTED_CALL((*mocks), STRING_construct("{\"body\":\""));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), IoTHubMessage_GetByteArray(message7.messageHandle, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3);

        STRICT_EXPECTED_CALL((*mocks), Azure_Base64_Encode_Bytes(buffer7, buffer7_size));
        STRICT_EXPECTED_CALL((*mocks), STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "\"")) /*closing the value of the body*/
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the second payload to the "big" payload*/
        STRICT_EXPECTED_CALL((*mocks), STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL((*mocks), DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL((*mocks), DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL((*mocks), DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL((*mocks), DList_InsertTailList(IGNORED_PTR_ARG, &(message7.entry)))
        .IgnoreArgument(1);


    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL((*mocks), BUFFER_new());
        STRICT_EXPECTED_CALL((*mocks), BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL((*mocks), STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL((*mocks), BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL((*mocks), STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL((*mocks), HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL((*mocks), Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_with_properties_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    DList_InsertTailList(&(waitingToSend), &(message7.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    setupIrrelevantMocksForProperties2(&message6.messageHandle, message7.messageHandle);

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message6.messageHandle));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    EXPECTED_CALL(STRING_new_with_memory(IGNORED_PTR_ARG))
        .ExpectedAtLeastTimes(2);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"properties\":"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "{")) /*opening of the properties*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"iothub-app-"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_VALUES1[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\":\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS1[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\""))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "}"))/*closing of the properties*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message7.messageHandle));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_2_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"properties\":"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "{")) /*opening of the properties*/
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\"iothub-app-"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_VALUES2[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\":\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS2[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\""))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"iothub-app-"))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_VALUES2[1]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\":\""))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS2[1]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "\""))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "}"))/*closing of the properties*/
        .IgnoreArgument(1);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}


#define TEST_2_ITEM_STRING "[{\"body\":\"MTIzNDU2\",\"properties\":{" TEST_RED_KEY_STRING_WITH_IOTHUBAPP ":" TEST_RED_VALUE_STRING "}},{\"body\":\"MTIzNDU2Nw==\",\"properties\":{" TEST_BLUE_KEY_STRING_WITH_IOTHUBAPP ":" TEST_BLUE_VALUE_STRING "," TEST_YELLOW_KEY_STRING_WITH_IOTHUBAPP ":" TEST_YELLOW_VALUE_STRING "}}]"
#define TEST_1_ITEM_STRING "[{\"body\":\"MTIzNDU2\",\"properties\":{" TEST_RED_KEY_STRING_WITH_IOTHUBAPP ":" TEST_RED_VALUE_STRING "}}]"

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items)
{
    //arrange
    CNiceCallComparer<CIoTHubTransportHttpMocks> mocks; /*a very e2e test... */
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    DList_InsertTailList(&(waitingToSend), &(message7.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), TEST_2_ITEM_STRING, sizeof(TEST_2_ITEM_STRING) - 1));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

#define THRESHOLD1 32 /*between THRESHOLD1 and THRESHOLD2 of STRING_concat fails there still is produced a payload, consisting of only REDKEY, REDVALUE*/
#define THRESHOLD2 17
#define THRESHOLD3 7 /*below 7, STRING_concat would fail not in producing properties*/

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_THRESHOLD1_succeeds)
{
    for (size_t i = THRESHOLD1 + 1; i > THRESHOLD1; i--)
    {
        //arrange
        currentSTRING_concat_call = 0;
        whenShallSTRING_concat_fail = 0;
        BASEIMPLEMENTATION::DList_InitializeListHead(&waitingToSend);
        CNiceCallComparer<CIoTHubTransportHttpMocks> mocks; /*a very e2e test... */
        DList_InsertTailList(&(waitingToSend), &(message6.entry));
        DList_InsertTailList(&(waitingToSend), &(message7.entry));
        TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
        (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

        umock_c_reset_all_calls();

        setupDoWorkLoopOnceForOneDevice();

        whenShallSTRING_concat_fail = i;

        ENABLE_BATCHING();

        //act
        IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

        //assert
        ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), TEST_2_ITEM_STRING, sizeof(TEST_2_ITEM_STRING) - 1));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubTransportHttp_Destroy(handle);
        if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
        {
            BASEIMPLEMENTATION::BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
            last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = NULL;
        }
    }
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_send_only_1_when_properties_for_second_fail)
{
    for (size_t i = THRESHOLD1; i > THRESHOLD2; i--)
    {
        //arrange
        currentSTRING_concat_call = 0;
        whenShallSTRING_concat_fail = 0;
        BASEIMPLEMENTATION::DList_InitializeListHead(&waitingToSend);
        CNiceCallComparer<CIoTHubTransportHttpMocks> mocks; /*a very e2e test... */
        DList_InsertTailList(&(waitingToSend), &(message6.entry));
        DList_InsertTailList(&(waitingToSend), &(message7.entry));
        TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
        (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

        umock_c_reset_all_calls();

        setupDoWorkLoopOnceForOneDevice();

        whenShallSTRING_concat_fail = i;

        ENABLE_BATCHING();

        //act
        IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

        //assert
        ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), TEST_1_ITEM_STRING, sizeof(TEST_1_ITEM_STRING) - 1));
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubTransportHttp_Destroy(handle);
        if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
        {
            BASEIMPLEMENTATION::BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
            last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = NULL;
        }
    }
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_2_event_items_send_nothing)
{
    for (size_t i = THRESHOLD2; i > THRESHOLD3; i--)
    {
        //arrange
        currentSTRING_concat_call = 0;
        whenShallSTRING_concat_fail = 0;
        BASEIMPLEMENTATION::DList_InitializeListHead(&waitingToSend);
        CNiceCallComparer<CIoTHubTransportHttpMocks> mocks; /*a very e2e test... */
        DList_InsertTailList(&(waitingToSend), &(message6.entry));
        DList_InsertTailList(&(waitingToSend), &(message7.entry));
        TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
        umock_c_reset_all_calls();

        whenShallSTRING_concat_fail = i;

        ENABLE_BATCHING();

        //act
        IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

        //assert
        ASSERT_IS_NULL(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        //cleanup
        IoTHubTransportHttp_Destroy(handle);
        if (last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest != NULL)
        {
            BASEIMPLEMENTATION::BUFFER_delete(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
            last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest = NULL;
        }
    }
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_with_NULL_handle_fails)
{
    //arrange

    //act
    auto result = IoTHubTransportHttp_SetOption(NULL, "someOption", "someValue");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    //cleanup
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_with_NULL_optionName_fails)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, NULL, "someValue");

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_with_NULL_value_fails)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, "someOption", NULL);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_succeeds_when_HTTPAPIEX_succeeds)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, "someOption", (void*)42));

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, "someOption", (void*)42);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_fails_when_HTTPAPIEX_returns_HTTPAPIEX_INVALID_ARG)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, "someOption", (void*)42))
        .SetReturn(HTTPAPIEX_INVALID_ARG);

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, "someOption", (void*)42);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_fails_when_HTTPAPIEX_returns_HTTPAPIEX_ERROR)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, "someOption", (void*)42))
        .SetReturn(HTTPAPIEX_ERROR);

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, "someOption", (void*)42);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetOption_fails_when_HTTPAPIEX_returns_any_other_error)
{
    //arrange

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(HTTPAPIEX_SetOption(TEST_HTTPAPIEX_HANDLE, "someOption", (void*)42))
        .SetReturn(HTTPAPIEX_RECOVERYFAILED);

    //act
    auto result = IoTHubTransportHttp_SetOption(handle, "someOption", (void*)42);

    //assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_abandon_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    currentDisposition = IOTHUBMESSAGE_ABANDONED;
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_1_property_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 1;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));
    }

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_succeeds)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"));
    }

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_when_properties_fails_it_abandons_1)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"))
            .SetReturn(MAP_ERROR);
    }

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_when_properties_fails_it_abandons_2)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        whenShallHTTPHeaders_GetHeader_fail = currentHTTPHeaders_GetHeader_call + 3;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
    }

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);


    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_when_properties_fails_it_abandons_3)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));

        whenShallHTTPHeaders_GetHeader_fail = currentHTTPHeaders_GetHeader_call + 2;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
    }

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_when_properties_fails_it_abandons_4)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"))
            .SetReturn(MAP_ERROR);
    }

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_happy_path_with_empty_waitingToSend_and_1_service_message_with_2_property_when_properties_fails_it_abandons_5)
{
    //arrange

    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    size_t nHeaders = 3;
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders))
            .SetReturn(HTTP_HEADERS_ERROR);
    }

    /*this is "abandon"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_no_properties_unbatched_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message1.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_1));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_1, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_1));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message1.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer1, buffer1_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_no_properties_string_type_unbatched_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_10));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetString(TEST_IOTHUB_MESSAGE_HANDLE_10));

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_10));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message10.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), string10, strlen(string10)));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_unbatched_happy_path_at_the_message_limit_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message11.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_11));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_11, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*1 property*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_11));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY_A_B, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_KEYS1_A_B[0]))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-a", TEST_VALUES1_A_B[0]))
        .IgnoreArgument(1);


    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message11.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer11, buffer11_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_no_properties_string_type_unbatched_happy_path_at_the_message_limit_succeeds)
{
    //arrange
    CNiceCallComparer<CIoTHubTransportHttpMocks> mocks;
    DList_InsertTailList(&(waitingToSend), &(message12.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_no_properties_string_type_unbatched_fails_when_getString_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_10));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetString(TEST_IOTHUB_MESSAGE_HANDLE_10))
        .SetReturn((const char*)NULL);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();


    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}


TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_httpStatusCode_is_not_succeess)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus404, sizeof(httpStatus404));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_HTTPAPIEXSAS_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200))
        .SetReturn(HTTPAPIEX_ERROR);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(BASEIMPLEMENTATION::BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_buffer_fails_1)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(1);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_buffer_fails_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1);

    whenShallBUFFER_new_fail = currentBUFFER_new_call + 1;
    STRICT_EXPECTED_CALL(BUFFER_new());

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_http_headers_fail_1)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_http_headers_fail_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, TEST_RED_KEY))
        .IgnoreArgument(1)
        .SetReturn(1111); /*unpredictable value which is an error*/

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_http_headers_fail_3)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4);

    /*this is making http headers*/
    whenShallSTRING_construct_fail = currentSTRING_construct_call + 1;
    STRICT_EXPECTED_CALL(STRING_construct("iothub-app-"));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_map_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .IgnoreArgument(4)
        .SetReturn(MAP_ERROR);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_http_fails_4)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1)
        .SetReturn(HTTP_HEADERS_ERROR);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_http_fails_5)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    whenShallHTTPHeaders_Clone_fail = currentHTTPHeaders_Clone_call + 1;
    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_does_nothing_when_IoTHubMessage_GetByteArray_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3)
        .SetReturn(IOTHUB_MESSAGE_ERROR);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_1_property_unbatched_overlimit_calls_SendComplete_with_BATCHSTATE_FAILED)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message9.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();


    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_9));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_9, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    /*ooops - over 256K*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message9.entry)))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_ERROR, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    DISABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_happy_path_succeeds)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"base64Encoded\":false")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message10.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    {
        /*adding the first payload to the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        /*closing the "big" payload*/
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
    }

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message10.entry)))
        .IgnoreArgument(1);

    {
        /*this is building the HTTP payload... from a STRING_HANDLE (as it comes as "big payload"), into an array of bytes*/
        STRICT_EXPECTED_CALL(BUFFER_new());
        STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_length(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(BUFFER_build(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2)
            .IgnoreArgument(3);
    }

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG))
        .IgnoreArgument(2);

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_string_concat_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"base64Encoded\":false")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message10.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4);
        whenShallSTRING_concat_fail = currentSTRING_concat_call + 2;
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "},")) /* this extra "," is going to be harshly overwritten by a "]"*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_Map_GetInternals_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"base64Encoded\":false")) /*closing the value of the body*/
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(message10.messageHandle));
        STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_EMPTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(2)
            .IgnoreArgument(3)
            .IgnoreArgument(4)
            .SetReturn(MAP_ERROR);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_STRING_concat_fails_it_fails_2)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);

        whenShallSTRING_concat_fail = currentSTRING_concat_call + 1;
        STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, ",\"base64Encoded\":false")) /*closing the value of the body*/
            .IgnoreArgument(1);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_STRING_concat_with_STRING_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        whenShallSTRING_concat_with_STRING_fail = currentSTRING_concat_with_STRING_call + 1;
        STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(2);
        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_STRING_new_JSON_fails_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle));

        whenShallSTRING_new_JSON_fail = currentSTRING_new_JSON_call + 1;
        STRICT_EXPECTED_CALL(STRING_new_JSON(string10));

        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_IoTHubMessage_GetString_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));
        STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_GetString(message10.messageHandle))
            .SetReturn((const char*)NULL);

        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_with_1_event_item_as_string_when_STRING_construct_it_fails)
{
    //arrange

    DList_InsertTailList(&(waitingToSend), &(message10.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/vnd.microsoft.iothub.json"))
        .IgnoreArgument(1);

    /*starting to prepare the "big" payload*/
    STRICT_EXPECTED_CALL(STRING_construct("["));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    /*this is first batched payload*/
    {
        STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(message10.messageHandle));
        whenShallSTRING_construct_fail = currentSTRING_construct_call + 2;
        STRICT_EXPECTED_CALL(STRING_construct("{\"body\":"));

        /*end of the first batched payload*/
    }

    ENABLE_BATCHING();

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_SetMessageId_SUCCEED)
{
    //arrange

    size_t nHeaders = 4;
    unsigned int statusCode200 = 200;
    unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .IgnoreArgument(1)
        .SetReturn(TEST_ETAG_VALUE);

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2)
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 3, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageId(IGNORED_PTR_ARG, "VALUE3"))
            .IgnoreArgument(1);
    }

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(IoTHubMessage_Clone(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreAllArguments();

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_DELETE,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));

    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);

    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_SetMessageId_FAILED)
{
    //arrange
    size_t nHeaders = 4;
    unsigned int statusCode200 = 200;
    //unsigned int statusCode204 = 204;
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/
    STRICT_EXPECTED_CALL(BUFFER_new());
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));
    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .SetReturn(TEST_ETAG_VALUE);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(IGNORED_PTR_ARG)).SetReturn(TEST_MAP_3_PROPERTY);
        //STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        //    .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 3, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageId(IGNORED_PTR_ARG, "VALUE3"));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
    /*this is "abandon"*/

    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

    /*STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    //STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    //STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));
    */
    STRICT_EXPECTED_CALL(IoTHubMessage_Destroy(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 3, IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(3);
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
            .IgnoreArgument(1);
        STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageId(IGNORED_PTR_ARG, "VALUE3"))
            .IgnoreArgument(1)
            .SetReturn(IOTHUB_MESSAGE_ERROR);
    }
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_construct_n(TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1))
        .ValidateArgumentBuffer(1, TEST_ETAG_VALUE_UNQUOTED, sizeof(TEST_ETAG_VALUE_UNQUOTED) - 1);
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(STRING_concat_with_STRING(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(1)
        .IgnoreArgument(2);
    STRICT_EXPECTED_CALL(STRING_concat(IGNORED_PTR_ARG, "/abandon" API_VERSION))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(Transport_GetOption_Product_Info_Callback(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "User-Agent", TEST_STRING_DATA))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "Authorization", TEST_BLANK_SAS_TOKEN))
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "If-Match", TEST_ETAG_VALUE))
        .IgnoreArgument(1);

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
        .IgnoreArgument(1); /*because abandon relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_POST,                               /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP_ETAG TEST_ETAG_VALUE_UNQUOTED "/abandon" API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        NULL,                                               /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        NULL                                                /*BUFFER_HANDLE responseContent))                              */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode204, sizeof(statusCode204));
    //act
    IoTHubTransportHttp_Create(handle, &transport_cb_info, transport_cb_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}
#endif

TEST_FUNCTION(IoTHubTransportHttp_DoWork_SendSecurityMessage_SUCCEED)
{
    //arrange
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG)).SetReturn(TEST_MESSAGE_ID);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-messageid", TEST_MESSAGE_ID));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG)).SetReturn(true);

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-interface-id", "urn:azureiot:Security:SecurityAgent:1"));

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));
    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(real_BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_SetCustomContentType_SetContentEncoding_SUCCEED)
{
    //arrange

    size_t nHeaders = 6;
    unsigned int statusCode200 = 200;

    char* real_ETAG = (char*)malloc(sizeof(TEST_ETAG_VALUE) + 1);
    (void)sprintf(real_ETAG, "%s", TEST_ETAG_VALUE);

    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    IOTHUB_DEVICE_HANDLE devHandle = IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    (void)IoTHubTransportHttp_Subscribe(devHandle);
    umock_c_reset_all_calls();
    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend)); /*because DoWork for event*/

    STRICT_EXPECTED_CALL(get_time(NULL));
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()); /*because responseHeadearsHandle: a new instance of HTTP headers*/

    STRICT_EXPECTED_CALL(BUFFER_new());

    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because relativePath is a STRING_HANDLE*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,                                    /*HTTPAPIEX_HANDLE handle,                                     */
        HTTPAPI_REQUEST_GET,                                /*HTTPAPI_REQUEST_TYPE requestType,                            */
        "/devices/" TEST_DEVICE_ID MESSAGE_ENDPOINT_HTTP API_VERSION,    /*const char* relativePath,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,                */
        NULL,                                               /*BUFFER_HANDLE requestContent,                                */
        IGNORED_PTR_ARG,                                    /*unsigned int* statusCode,                                    */
        IGNORED_PTR_ARG,                                    /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,               */
        IGNORED_PTR_ARG                                     /*BUFFER_HANDLE responseContent))                              */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &statusCode200, sizeof(statusCode200));

    STRICT_EXPECTED_CALL(HTTPHeaders_FindHeaderValue(IGNORED_PTR_ARG, "ETag"))
        .SetReturn(real_ETAG);

    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(IoTHubMessage_CreateFromByteArray(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
        .SetReturn(TEST_IOTHUB_MESSAGE_HANDLE_8);

    {
        /*this scope is for is properties code*/

        HTTPHeaders_GetHeaderCount_writes_to_its_outputs = false;
        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeaderCount(IGNORED_PTR_ARG, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, &nHeaders, sizeof(nHeaders));

        STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_8));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 0, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME1", "VALUE1"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 1, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 2, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(Map_AddOrUpdate(TEST_MAP_3_PROPERTY, "NAME2", "VALUE2"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 3, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubMessage_SetMessageId(IGNORED_PTR_ARG, "VALUE3"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 4, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentTypeSystemProperty(IGNORED_PTR_ARG, "VALUE4"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

        STRICT_EXPECTED_CALL(HTTPHeaders_GetHeader(IGNORED_PTR_ARG, 5, IGNORED_PTR_ARG));
        STRICT_EXPECTED_CALL(IoTHubMessage_SetContentEncodingSystemProperty(IGNORED_PTR_ARG, "VALUE5"));
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }

    MESSAGE_DISPOSITION_CONTEXT_HANDLE dispositionContextHandle;
    MESSAGE_DISPOSITION_CONTEXT_DESTROY_FUNCTION dispositionContextDestroyFunction;

    /*this returns "0" so the message needs to be "accepted"*/
    /*this is "accepting"*/
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .CopyOutArgumentBuffer_destination(&real_ETAG, sizeof(&real_ETAG));
    STRICT_EXPECTED_CALL(IoTHubMessage_SetDispositionContext(IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CaptureArgumentValue_dispositionContext(&dispositionContextHandle)
        .CaptureArgumentValue_dispositionContextDestroyFunction(&dispositionContextDestroyFunction);
    STRICT_EXPECTED_CALL(Transport_MessageCallback(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(my_IoTHubClientCore_LL_MessageCallback_messageHandle);
    ASSERT_IS_NOT_NULL(dispositionContextHandle);
    ASSERT_IS_NOT_NULL(dispositionContextDestroyFunction);

    //cleanup
    IoTHubTransportHttp_SendMessageDisposition(devHandle, my_IoTHubClientCore_LL_MessageCallback_messageHandle, IOTHUBMESSAGE_ACCEPTED);
    dispositionContextDestroyFunction(dispositionContextHandle);
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_GetMessageId_succeeds)
{
    //arrange
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);

    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));

    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .IgnoreArgument(3);

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"))
        .IgnoreArgument(1);

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE));
    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG)).SetReturn(TEST_MESSAGE_ID);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-messageid", TEST_MESSAGE_ID));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));
    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_IS_NOT_NULL(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest);
    ASSERT_ARE_EQUAL(int, 0, memcmp(real_BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_GetCorrelationId_succeeds)
{
    //arrange
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"));

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE));

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG)).SetReturn(TEST_MESSAGE_ID);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-correlationid", TEST_MESSAGE_ID));
    EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
        ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(real_BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_GetCustomContentType_succeeds)
{
    //arrange
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"));

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE));

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(TEST_CONTENT_TYPE);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-contenttype", TEST_CONTENT_TYPE));
    EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)) /*because relativePath*/
        .IgnoreArgument(1);
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(real_BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_DoWork_GetContentEncoding_succeeds)
{
    //arrange
    DList_InsertTailList(&(waitingToSend), &(message6.entry));
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    (void)IoTHubTransportHttp_Register(handle, &TEST_DEVICE_1, TEST_CONFIG.waitingToSend);
    umock_c_reset_all_calls();

    setupDoWorkLoopOnceForOneDevice();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(&waitingToSend));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetContentType(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(IoTHubMessage_GetByteArray(TEST_IOTHUB_MESSAGE_HANDLE_6, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(HTTPHeaders_Clone(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "Content-Type", "application/octet-stream"));

    /*no properties, so no more headers*/
    STRICT_EXPECTED_CALL(IoTHubMessage_Properties(TEST_IOTHUB_MESSAGE_HANDLE_6));
    STRICT_EXPECTED_CALL(Map_GetInternals(TEST_MAP_1_PROPERTY, IGNORED_PTR_ARG, IGNORED_PTR_ARG, IGNORED_PTR_ARG));

    /*this is making http headers*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-app-" TEST_RED_KEY, TEST_RED_VALUE));

    STRICT_EXPECTED_CALL(STRING_delete(IGNORED_PTR_ARG));

    EXPECTED_CALL(IoTHubMessage_GetMessageId(IGNORED_PTR_ARG));
    EXPECTED_CALL(IoTHubMessage_GetCorrelationId(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentTypeSystemProperty(IGNORED_PTR_ARG)).SetReturn(NULL);
    EXPECTED_CALL(IoTHubMessage_GetContentEncodingSystemProperty(IGNORED_PTR_ARG)).SetReturn(TEST_CONTENT_ENCODING);
    STRICT_EXPECTED_CALL(HTTPHeaders_ReplaceHeaderNameValuePair(IGNORED_PTR_ARG, "iothub-contentencoding", TEST_CONTENT_ENCODING));

    STRICT_EXPECTED_CALL(IoTHubMessage_IsSecurityMessage(IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_create(IGNORED_PTR_ARG, IGNORED_NUM_ARG));

    /*executing HTTP goodies*/
    STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG)); /*because relativePath*/
    STRICT_EXPECTED_CALL(HTTPAPIEX_SAS_ExecuteRequest(
        IGNORED_PTR_ARG,                                    /*sasObject handle                                             */
        IGNORED_PTR_ARG,
        HTTPAPI_REQUEST_POST,                                                           /*HTTPAPI_REQUEST_TYPE requestType,                  */
        "/devices/" TEST_DEVICE_ID EVENT_ENDPOINT API_VERSION,                 /*const char* relativePath,                          */
        IGNORED_PTR_ARG,                                                                /*HTTP_HEADERS_HANDLE requestHttpHeadersHandle,      */
        IGNORED_PTR_ARG,                                                                /*BUFFER_HANDLE requestContent,                      */
        IGNORED_PTR_ARG,                                                                /*unsigned int* statusCode,                          */
        NULL,                                                                           /*HTTP_HEADERS_HANDLE responseHttpHeadersHandle,     */
        NULL                                                                            /*BUFFER_HANDLE responseContent)                     */
    ))
        .IgnoreArgument_requestType()
        .CopyOutArgumentBuffer(7, &httpStatus200, sizeof(httpStatus200));

    /*once the event has been succesfull...*/

    /*building the list of messages to be notified if HTTP is fine*/
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_PTR_ARG, &(message6.entry)));
    STRICT_EXPECTED_CALL(Transport_SendComplete_Callback(IGNORED_PTR_ARG, IOTHUB_CLIENT_CONFIRMATION_OK, IGNORED_PTR_ARG));

    STRICT_EXPECTED_CALL(BUFFER_delete(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(IGNORED_PTR_ARG));

    //act
    IoTHubTransportHttp_DoWork(handle);

    //assert
    ASSERT_ARE_EQUAL(int, 0, memcmp(real_BUFFER_u_char(last_BUFFER_HANDLE_to_HTTPAPIEX_ExecuteRequest), buffer6, buffer6_size));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetHostname_with_NULL_handle_fails)
{
    //arrange

    //act
    STRING_HANDLE hostname = IoTHubTransportHttp_GetHostname(NULL);

    //assert
    ASSERT_IS_NULL(hostname);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(IoTHubTransportHttp_GetHostname_with_non_NULL_handle_succeeds)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(STRING_clone(IGNORED_PTR_ARG));

    //act
    STRING_HANDLE hostname = IoTHubTransportHttp_GetHostname(handle);

    //assert
    ASSERT_IS_NOT_NULL(hostname);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, TEST_IOTHUB_NAME "." TEST_IOTHUB_SUFFIX, real_STRING_c_str(hostname));

    //cleanup
    real_STRING_delete(hostname);
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Unsubscribe_DeviceTwin_returns)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    IoTHubTransportHttp_Unsubscribe_DeviceTwin(handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_Subscribe_DeviceTwin_returns)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    //act
    int res = IoTHubTransportHttp_Subscribe_DeviceTwin(handle);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, res);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_GetTwinAsync_returns)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    //act
    IOTHUB_CLIENT_RESULT res = IoTHubTransportHttp_GetTwinAsync(handle, (IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)0x4444, (void*)0x4445);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, IOTHUB_CLIENT_ERROR, res);

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetCallbackContext_success)
{
    // arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);
    umock_c_reset_all_calls();

    // act
    int result = IoTHubTransportHttp_SetCallbackContext(handle, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    IoTHubTransportHttp_Destroy(handle);
}

TEST_FUNCTION(IoTHubTransportHttp_SetCallbackContext_fail)
{
    // arrange

    // act
    int result = IoTHubTransportHttp_SetCallbackContext(NULL, transport_cb_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

TEST_FUNCTION(IoTHubTransportHttp_GetSupportedPlatformInfo_returns)
{
    //arrange
    TRANSPORT_LL_HANDLE handle = IoTHubTransportHttp_Create(&TEST_CONFIG, &transport_cb_info, transport_cb_ctx);

    umock_c_reset_all_calls();

    //act
    PLATFORM_INFO_OPTION info;
    int result = IoTHubTransportHttp_GetSupportedPlatformInfo(handle, &info);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(PLATFORM_INFO_OPTION, info, PLATFORM_INFO_OPTION_DEFAULT);

    //cleanup
    IoTHubTransportHttp_Destroy(handle);
}

END_TEST_SUITE(iothubtransporthttp_ut)

