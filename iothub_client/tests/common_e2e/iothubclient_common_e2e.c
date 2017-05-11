// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdbool>
#include <cstring>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "umock_c.h"
#endif

#ifdef AZIOT_LINUX
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "testrunnerswitcher.h"

#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothub_messaging.h"

#include "iothub_account.h"
#include "iothubtest.h"

#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "../../../certs/certs.h"

#include "iothubclient_common_e2e.h"

static bool g_callbackRecv = false;

const char* TEST_EVENT_DATA_FMT = "{\"data\":\"%.24s\",\"id\":\"%d\"}";

const char* MSG_ID = "MessageIdForE2E";
const char* MSG_CORRELATION_ID = "MessageCorrelationIdForE2E";
const char* MSG_CONTENT = "Message content for E2E";

#define MSG_PROP_COUNT 3
const char* MSG_PROP_KEYS[MSG_PROP_COUNT] = { "Key1", "Key2", "Key3" };
const char* MSG_PROP_VALS[MSG_PROP_COUNT] = { "Val1", "Val2", "Val3" };

static size_t g_iotHubTestId = 0;
IOTHUB_ACCOUNT_INFO_HANDLE g_iothubAcctInfo = NULL;
E2E_TEST_OPTIONS g_e2e_test_options;

#define IOTHUB_COUNTER_MAX           10
#define MAX_CLOUD_TRAVEL_TIME        60.0
// Wait for 60 seconds for the service to tell us that an event was received.
#define MAX_SERVICE_EVENT_WAIT_TIME_SECONDS 60
// When waiting for events, start listening for events that happened up to 60 seconds in the past.
#define SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS 60

TEST_DEFINE_ENUM_TYPE(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

typedef struct EXPECTED_SEND_DATA_TAG
{
    const char* expectedString;
    bool wasFound;
    bool dataWasRecv;
    LOCK_HANDLE lock;
    IOTHUB_MESSAGE_HANDLE msgHandle;
} EXPECTED_SEND_DATA;

typedef struct EXPECTED_RECEIVE_DATA_TAG
{
    bool wasFound;
    LOCK_HANDLE lock; /*needed to protect this structure*/
    IOTHUB_MESSAGE_HANDLE msgHandle;
} EXPECTED_RECEIVE_DATA;

typedef struct CONNECTION_STATUS_DATA_TAG
{
    bool connFaultHappened;
    bool connRestored;
    LOCK_HANDLE lock;
    IOTHUB_CLIENT_CONNECTION_STATUS currentStatus;
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON currentStatusReason;
} CONNECTION_STATUS_INFO;

static CONNECTION_STATUS_INFO g_connection_status_info;

static void openCompleteCallback(void* context)
{
    (void)printf("Open completed, context: %s\n", (char*)context);
}

static void sendCompleteCallback(void* context, IOTHUB_MESSAGING_RESULT messagingResult)
{
    (void)context;
    if (messagingResult == IOTHUB_MESSAGING_OK)
    {
        (void)printf("Message has been sent successfully\n");
    }
    else
    {
        (void)printf("Send failed\n");
    }
}

static int IoTHubCallback(void* context, const char* data, size_t size)
{
    (void)size;
    int result = 0; // 0 means "keep processing"
    EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)context;
    if (expectedData != NULL)
    {
        if (Lock(expectedData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (
                (strlen(expectedData->expectedString) == size) &&
                (memcmp(expectedData->expectedString, data, size) == 0)
                )
            {
                expectedData->wasFound = true;
                result = 1;
            }
            (void)Unlock(expectedData->lock);
        }
    }
    return result;
}

static void ReceiveConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)result;
    EXPECTED_SEND_DATA* expectedData = (EXPECTED_SEND_DATA*)userContextCallback;
    if (expectedData != NULL)
    {
        if (Lock(expectedData->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            expectedData->dataWasRecv = true;
            (void)Unlock(expectedData->lock);
        }
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE messageHandle, void* userContextCallback)
{
    EXPECTED_RECEIVE_DATA* receiveUserContext = (EXPECTED_RECEIVE_DATA*)userContextCallback;
    if (receiveUserContext == NULL)
    {
        ASSERT_FAIL("User context is NULL");
    }
    else
    {
        if (Lock(receiveUserContext->lock) != LOCK_OK)
        {
            ASSERT_FAIL("Unable to lock"); /*because the test must absolutely be terminated*/
        }
        else
        {
            const char* messageId;
            const char* correlationId;
            const unsigned char* content;
            size_t contentSize;

            if ((messageId = IoTHubMessage_GetMessageId(messageHandle)) == NULL)
            {
                messageId = "<null>";
            }
            else
            {
                if (strcmp(messageId, MSG_ID) != 0)
                {
                    ASSERT_FAIL("Message ID mismatch.");
                }
            }

            if ((correlationId = IoTHubMessage_GetCorrelationId(messageHandle)) == NULL)
            {
                correlationId = "<null>";
            }
            else
            {
                if (strcmp(correlationId, MSG_CORRELATION_ID) != 0)
                {
                    ASSERT_FAIL("Message correlation ID mismatch.");
                }
            }

            IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(messageHandle);
            if (contentType != IOTHUBMESSAGE_BYTEARRAY)
            {
                ASSERT_FAIL("Message content type mismatch.");
            }
            else
            {
                if (IoTHubMessage_GetByteArray(messageHandle, &content, &contentSize) != IOTHUB_MESSAGE_OK)
                {
                    ASSERT_FAIL("Unable to get the content of the message.");
                }
                else
                {
                    (void)printf("Received new message from IoT Hub :\r\nMessage-id: %s\r\nCorrelation-id: %s\r\n", messageId, correlationId);
                    (void)printf("\r\n");
                }

                receiveUserContext->wasFound = true;
                MAP_HANDLE mapHandle = IoTHubMessage_Properties(messageHandle);
                if (mapHandle != NULL)
                {
                    const char*const* keys;
                    const char*const* values;
                    size_t propertyCount = 0;
                    if (Map_GetInternals(mapHandle, &keys, &values, &propertyCount) == MAP_OK)
                    {
                        receiveUserContext->wasFound = true;
                        if (propertyCount == MSG_PROP_COUNT)
                        {
                            (void)printf("Message Properties:\r\n");
                            for (size_t index = 0; index < propertyCount; index++)
                            {
                                (void)printf("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                                if (strcmp(keys[index], MSG_PROP_KEYS[index]) != 0)
                                {
                                    receiveUserContext->wasFound = false;
                                }
                                if (strcmp(values[index], MSG_PROP_VALS[index]) != 0)
                                {
                                    receiveUserContext->wasFound = false;
                                }
                            }
                            (void)printf("\r\n");
                        }
                        else
                        {
                            receiveUserContext->wasFound = false;
                        }
                    }
                }
            }
            Unlock(receiveUserContext->lock);
        }
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

static EXPECTED_RECEIVE_DATA* ReceiveUserContext_Create(void)
{
    EXPECTED_RECEIVE_DATA* result = (EXPECTED_RECEIVE_DATA*)malloc(sizeof(EXPECTED_RECEIVE_DATA));
    if (result != NULL)
    {
        memset(result, 0, sizeof(*result));
        if ((result->lock = Lock_Init()) == NULL)
        {
            free(result);
            result = NULL;
        }
    }
    return result;
}

static void ReceiveUserContext_Destroy(EXPECTED_RECEIVE_DATA* data)
{
    if (data != NULL)
    {
        (void)Lock_Deinit(data->lock);

        if (data->msgHandle != NULL)
        {
            IoTHubMessage_Destroy(data->msgHandle);
        }

        free(data);
    }
}

static EXPECTED_SEND_DATA* EventData_Create(void)
{
    EXPECTED_SEND_DATA* result = (EXPECTED_SEND_DATA*)malloc(sizeof(EXPECTED_SEND_DATA));
    memset(result, 0, sizeof(*result));
    if (result != NULL)
    {
        if ((result->lock = Lock_Init()) == NULL)
        {
            ASSERT_FAIL("unable to Lock_Init");
        }
        else
        {
            char temp[1000];
            char* tempString;
            time_t t = time(NULL);
            size_t string_length;
            string_length = sprintf(temp, TEST_EVENT_DATA_FMT, ctime(&t), g_iotHubTestId);
            if ((tempString = (char*)malloc(string_length + 1)) == NULL)
            {
                Lock_Deinit(result->lock);
                free(result);
                result = NULL;
            }
            else
            {
                strcpy(tempString, temp);
                result->expectedString = tempString;
                result->wasFound = false;
                result->dataWasRecv = false;
            }
        }
    }
    return result;
}

static void EventData_Destroy(EXPECTED_SEND_DATA* data)
{
    if (data != NULL)
    {
        (void)Lock_Deinit(data->lock);
        if (data->expectedString != NULL)
        {
            free((void*)data->expectedString);
        }
        if (data->msgHandle != NULL)
        {
            IoTHubMessage_Destroy(data->msgHandle);
        }
        free(data);
    }
}

#ifdef AZIOT_LINUX
static char* get_target_mac_address()
{
    char* result;
    int s;
    
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        LogError("Failure: socket create failure %d.", s);
        result = NULL;
    }
    else
    {
        struct ifreq ifr;
        struct ifconf ifc;
        char buf[1024];

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;

        if (ioctl(s, SIOCGIFCONF, &ifc) == -1)
        {
            LogError("ioctl failed querying socket (SIOCGIFCONF)");
            result = NULL;
        }
        else 
        {
            struct ifreq* it = ifc.ifc_req;
            const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
            
            result = NULL;
            
            for (; it != end; ++it)
            {
                strcpy(ifr.ifr_name, it->ifr_name);

                if (ioctl(s, SIOCGIFFLAGS, &ifr) != 0)
                {
                    LogError("ioctl failed querying socket (SIOCGIFFLAGS)");
                    break;
                }
                else if (ioctl(s, SIOCGIFHWADDR, &ifr) != 0)
                {
                    LogError("ioctl failed querying socket (SIOCGIFHWADDR)");
                    break;
                }
                else if (ioctl(s, SIOCGIFADDR, &ifr) != 0)
                {
                    LogError("ioctl failed querying socket (SIOCGIFADDR)");
                    break;
                }
                else if (strcmp(ifr.ifr_name, "eth0") == 0)
                {
                    unsigned char* mac = (unsigned char*)ifr.ifr_hwaddr.sa_data;
                    
                    if ((result = (char*)malloc(sizeof(char) * 18)) == NULL)
                    {
                        LogError("failed formatting mac address (malloc failed)");
                    }
                    else if (sprintf(result, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]) <= 0)
                    {
                        LogError("failed formatting mac address (sprintf failed)");
                        free(result);
                        result = NULL;
                    }
                    
                    break;
                }
            }
        }
        
        close(s);
    }
    
    return result;
}
#endif //AZIOT_LINUX


void e2e_init(void)
{
    int result = platform_init();
    ASSERT_ARE_EQUAL_WITH_MSG(int, 0, result, "Platform init failed");
    g_iothubAcctInfo = IoTHubAccount_Init();
    ASSERT_IS_NOT_NULL_WITH_MSG(g_iothubAcctInfo, "Could not initialize IoTHubAccount");
    platform_init();

    memset(&g_e2e_test_options, 0, sizeof(E2E_TEST_OPTIONS));
    g_e2e_test_options.set_mac_address = false;

    g_connection_status_info.lock = Lock_Init();
}

void e2e_deinit(void)
{
    IoTHubAccount_deinit(g_iothubAcctInfo);
    // Need a double deinit
    platform_deinit();
    platform_deinit();

    Lock_Deinit(g_connection_status_info.lock);
}

IOTHUB_CLIENT_HANDLE client_connect_to_hub(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    IOTHUB_CLIENT_RESULT result;

    iotHubClientHandle = IoTHubClient_CreateFromConnectionString(deviceToUse->connectionString, protocol);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubClientHandle, "Could not create IoTHubClient");

    if (deviceToUse->howToCreate == IOTHUB_ACCOUNT_AUTH_X509) {
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, deviceToUse->certificate);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 certificate");
        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, deviceToUse->primaryAuthentication);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Could not set the device x509 privateKey");
    }

    // Turn on Log 
    bool trace = true;
    (void)IoTHubClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &trace);
    (void)IoTHubClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates);

    (void)IoTHubClient_SetOption(iotHubClientHandle, OPTION_PRODUCT_INFO, "MQTT_E2E/1.1.12");

#ifdef AZIOT_LINUX
    if (g_e2e_test_options.set_mac_address)
    {
        char* mac_address = get_target_mac_address();
        ASSERT_IS_NOT_NULL_WITH_MSG(mac_address, "failed getting the target MAC ADDRESS");

        result = IoTHubClient_SetOption(iotHubClientHandle, OPTION_NET_INT_MAC_ADDRESS, mac_address);
        ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "failed to set the target network interface");

        LogInfo("Target MAC ADDRESS: %s", mac_address);
        free(mac_address);
    }
#endif //AZIOT_LINUX

    return iotHubClientHandle;
}

D2C_MESSAGE_HANDLE client_create_and_send_d2c(IOTHUB_CLIENT_HANDLE iotHubClientHandle)
{
    IOTHUB_MESSAGE_HANDLE msgHandle;
    IOTHUB_CLIENT_RESULT result;

    EXPECTED_SEND_DATA* sendData = EventData_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(sendData, "Could not create the EventData associated with the event to be sent");

    msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
    ASSERT_IS_NOT_NULL_WITH_MSG(msgHandle, "Could not create the D2C message to be sent");

    MAP_HANDLE mapHandle = IoTHubMessage_Properties(msgHandle);
    for (size_t i = 0; i < MSG_PROP_COUNT; i++)
    {
        if (Map_AddOrUpdate(mapHandle, MSG_PROP_KEYS[i], MSG_PROP_VALS[i]) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate failed for property %zu!\r\n", i);
        }
    }

    sendData->msgHandle = msgHandle;

    // act
    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "SendEventAsync failed");

    return (D2C_MESSAGE_HANDLE)sendData;
}

bool client_wait_for_d2c_confirmation(D2C_MESSAGE_HANDLE d2cMessage)
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (client_received_confirmation(d2cMessage))
        {
            break;
        }
        ThreadAPI_Sleep(100);
    }
    return (client_received_confirmation(d2cMessage));
}

bool client_received_confirmation(D2C_MESSAGE_HANDLE d2cMessage)
{
    bool result = false;
    EXPECTED_SEND_DATA* sendData = (EXPECTED_SEND_DATA*)d2cMessage;

    if (Lock(sendData->lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        result = sendData->dataWasRecv;
        (void)Unlock(sendData->lock);
    }

    return result;
}

D2C_MESSAGE_HANDLE client_create_with_properies_and_send_d2c(IOTHUB_CLIENT_HANDLE iotHubClientHandle, MAP_HANDLE mapHandle)
{
    IOTHUB_MESSAGE_HANDLE msgHandle;
    IOTHUB_CLIENT_RESULT result;

    EXPECTED_SEND_DATA* sendData = EventData_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(sendData, "Could not create the EventData associated with the event to be sent");

    msgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)sendData->expectedString, strlen(sendData->expectedString));
    ASSERT_IS_NOT_NULL_WITH_MSG(msgHandle, "Could not create the D2C message to be sent");

    MAP_HANDLE msgMapHandle = IoTHubMessage_Properties(msgHandle);

    const char*const* keys;
    const char*const* values;
    size_t propCount;

    MAP_RESULT mapResult = Map_GetInternals(mapHandle, &keys, &values, &propCount);
    if (mapResult == MAP_OK)
    {
        for (size_t i = 0; i < propCount; i++)
        {
            if (Map_AddOrUpdate(msgMapHandle, keys[i], values[i]) != MAP_OK)
            {
                ASSERT_FAIL("Map_AddOrUpdate failed!");
            }
        }
    }

    sendData->msgHandle = msgHandle;

    // act
    result = IoTHubClient_SendEventAsync(iotHubClientHandle, msgHandle, ReceiveConfirmationCallback, sendData);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "SendEventAsync failed");

    return (D2C_MESSAGE_HANDLE)sendData;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    if (reason == IOTHUB_CLIENT_CONNECTION_OK)
    {
        CONNECTION_STATUS_INFO* connection_status_info = (CONNECTION_STATUS_INFO*)userContextCallback;
        if (Lock(connection_status_info->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if ((connection_status_info->currentStatus == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) &&
                (status == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED))
            {
                connection_status_info->connFaultHappened = true;
            }
            if ((connection_status_info->currentStatus == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED) &&
                (status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED))
            {
                connection_status_info->connRestored = true;
            }
            connection_status_info->currentStatus = status;
            connection_status_info->currentStatusReason = reason;

            (void)Unlock(connection_status_info->lock);
        }
    }
}

bool client_wait_for_connection_fault()
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (client_status_fault_happened())
        {
            break;
        }
        ThreadAPI_Sleep(100);
    }
    return (client_status_fault_happened());
}

bool client_status_fault_happened()
{
    bool result = false;

    if (Lock(g_connection_status_info.lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        result = g_connection_status_info.connFaultHappened;
        (void)Unlock(g_connection_status_info.lock);
    }

    return result;
}

bool client_wait_for_connection_restored()
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)),
        (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) // time box
        )
    {
        if (client_status_restored())
        {
            break;
        }
        ThreadAPI_Sleep(100);
    }
    return (client_status_restored());
}

bool client_status_restored()
{
    bool result = false;

    if (Lock(g_connection_status_info.lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        result = g_connection_status_info.connRestored;
        (void)Unlock(g_connection_status_info.lock);
    }

    return result;
}

void clear_connection_status_info_flags()
{
    if (Lock(g_connection_status_info.lock) != LOCK_OK)
    {
        ASSERT_FAIL("unable to lock");
    }
    else
    {
        g_connection_status_info.connFaultHappened = false;
        g_connection_status_info.connRestored = false;
        (void)Unlock(g_connection_status_info.lock);
    }
}

void service_wait_for_d2c_event_arrival(IOTHUB_PROVISIONED_DEVICE* deviceToUse, D2C_MESSAGE_HANDLE d2cMessage)
{
    EXPECTED_SEND_DATA* sendData = (EXPECTED_SEND_DATA*)d2cMessage;
    
    IOTHUB_TEST_HANDLE iotHubTestHandle = IoTHubTest_Initialize(IoTHubAccount_GetEventHubConnectionString(g_iothubAcctInfo), IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo), deviceToUse->deviceId, IoTHubAccount_GetEventhubListenName(g_iothubAcctInfo), IoTHubAccount_GetEventhubAccessKey(g_iothubAcctInfo), IoTHubAccount_GetSharedAccessSignature(g_iothubAcctInfo), IoTHubAccount_GetEventhubConsumerGroup(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubTestHandle, "Could not initialize IoTHubTest in order to listen for events");

    IOTHUB_TEST_CLIENT_RESULT result = IoTHubTest_ListenForEvent(iotHubTestHandle, IoTHubCallback, IoTHubAccount_GetIoTHubPartitionCount(g_iothubAcctInfo), sendData, time(NULL)-SERVICE_EVENT_WAIT_TIME_DELTA_SECONDS, MAX_SERVICE_EVENT_WAIT_TIME_SECONDS);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_OK, result, "Listening for the event failed");
    
    ASSERT_IS_TRUE_WITH_MSG(sendData->wasFound, "Failure retrieving data that was sent to eventhub"); // was found is written by the callback...

    IoTHubTest_Deinit(iotHubTestHandle);
}

bool service_received_the_message(D2C_MESSAGE_HANDLE d2cMessage)
{
    return ((EXPECTED_SEND_DATA*)d2cMessage)->wasFound;
}

void destroy_d2c_message_handle(D2C_MESSAGE_HANDLE d2cMessage)
{
    EventData_Destroy((EXPECTED_SEND_DATA*)d2cMessage);
}

static void send_event_test(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{

    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage;

    // Create the IoT Hub Data
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Send the Event from the client
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // Wait for confirmation that the event was recevied
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

    // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    /* guess who */
    (void)platform_init();

    // Waigt for the message to arrive
    service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage);

    // cleanup
    destroy_d2c_message_handle(d2cMessage);


}

void e2e_send_event_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    send_event_test(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol);
}

void e2e_send_event_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    send_event_test(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol);
}

void e2e_d2c_with_svc_fault_ctrl(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* faultOperationType, const char* faultOperationCloseReason, const char* faultOperationDelayInSecs)
{
    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage;

    MAP_HANDLE propMap = Map_Create(NULL);

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", faultOperationType) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", faultOperationCloseReason) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", faultOperationDelayInSecs) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }

    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);

    // Create the IoT Hub Data
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Send the Event from the client
    (void)printf("Send message and wait for confirmation...\r\n");
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // Wait for confirmation that the event was recevied
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

    (void)printf("Send server fault control message and wait for confirmation...\r\n");
    clear_connection_status_info_flags();
    d2cMessage = client_create_with_properies_and_send_d2c(iotHubClientHandle, propMap);

    ThreadAPI_Sleep(3000);

    size_t i;
    for (i = 0; i < 10; i++)
    {
        // Send the Event from the client
        (void)printf("Send message after the server fault and wait for confirmation...\r\n");
        d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

        // Wait for confirmation that the event was recevied
        dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
        if (dataWasRecv)
            break;

        ThreadAPI_Sleep(2000);
    }
    ASSERT_ARE_NOT_EQUAL_WITH_MSG(size_t, 10, i, "Don't recover after the fault..."); // was received by the callback...

                                                                                      // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    /* guess who */
    (void)platform_init();

    // Waigt for the message to arrive
    service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage);

    // cleanup
    destroy_d2c_message_handle(d2cMessage);
}

void e2e_d2c_with_svc_fault_ctrl_with_transport_status(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* faultOperationType, const char* faultOperationCloseReason, const char* faultOperationDelayInSecs)
{
    MAP_HANDLE propMap = Map_Create(NULL);

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", faultOperationType) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", faultOperationCloseReason) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", faultOperationDelayInSecs) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }

    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);

    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage;

    // Create the IoT Hub Data
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Set connection status change callback
    IOTHUB_CLIENT_RESULT setConnResult = IoTHubClient_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, &g_connection_status_info);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, setConnResult, "Failure setting connection status callback");

    // Send the Event from the client
    (void)printf("Send message and wait for confirmation...\r\n");
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // Wait for confirmation that the event was recevied
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

    // Send the Fault Control Event from the client
    (void)printf("Send server fault control message and wait for confirmation...\r\n");
    clear_connection_status_info_flags();
    d2cMessage = client_create_with_properies_and_send_d2c(iotHubClientHandle, propMap);

    // Wait for confirmation that the fault control was received
    dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending fault control message to IotHub"); // was received by the callback...

    // Wait for connection status change (restored)
    bool connStatus = client_wait_for_connection_restored();
    ASSERT_IS_TRUE_WITH_MSG(connStatus, "Fault injection failed - connection has not been restored");

    // Wait for connection status change (fault)
    connStatus = client_wait_for_connection_fault();
    ASSERT_IS_TRUE_WITH_MSG(connStatus, "Fault injection failed - no fault happened");

    // Wait for connection status change (restored)
    connStatus = client_wait_for_connection_restored();
    ASSERT_IS_TRUE_WITH_MSG(connStatus, "Fault injection failed - connection has not been restored");

    // Wait for confirmation that the event was recevied
    dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

                                                                            // Send the Event from the client
    (void)printf("Send message after the server fault and wait for confirmation...\r\n");
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // Wait for confirmation that the event was recevied
    dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

                                                                            // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    /* guess who */
    (void)platform_init();

    // Waigt for the message to arrive
    service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage);

    // cleanup
    destroy_d2c_message_handle(d2cMessage);
}

void e2e_d2c_with_svc_fault_ctrl_no_answer(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* faultOperationType, const char* faultOperationCloseReason, const char* faultOperationDelayInSecs, bool setTimeoutOption)
{
    MAP_HANDLE propMap = Map_Create(NULL);

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", faultOperationType) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", faultOperationCloseReason) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", faultOperationDelayInSecs) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }

    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);

    // arrange
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage;

    // Create the IoT Hub Data
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Set connection status change callback
    IOTHUB_CLIENT_RESULT setConnResult = IoTHubClient_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, &g_connection_status_info);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, setConnResult, "Failure setting connection status callback");

    if (setTimeoutOption)
    {
        if ((strcmp(faultOperationType, "InvokeThrottling") == 0) ||
            (strcmp(faultOperationType, "InvokeMaxMessageQuota") == 0) ||
            (strcmp(faultOperationType, "InvokeAuthError") == 0)
            )
        {
            IOTHUB_CLIENT_RESULT setOptionResult = IoTHubClient_SetOption(iotHubClientHandle, "event_send_timeout_secs", "30");
            ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, setOptionResult, "Failure setting transport timeout");
        }
    }

    // Send the Event from the client
    (void)printf("Send message and wait for confirmation...\r\n");
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // Wait for confirmation that the event was recevied
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

                                                                            // Send the Fault Control Event from the client
    (void)printf("Send server fault control message and wait for confirmation...\r\n");
    clear_connection_status_info_flags();
    d2cMessage = client_create_with_properies_and_send_d2c(iotHubClientHandle, propMap);

    // Wait for confirmation that the fault control was received
    dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending fault control message to IotHub"); // was received by the callback...

    if ((strcmp(faultOperationType, "InvokeThrottling") == 0) ||
        (strcmp(faultOperationType, "InvokeMaxMessageQuota") == 0) ||
        (strcmp(faultOperationType, "InvokeAuthError") == 0)
        )
    {
        ThreadAPI_Sleep(3000);
        // Send the Event from the client and expect no answer (after 60 sec wait)
        (void)printf("Sending message and expect no confirmation...\r\n");
        d2cMessage = client_create_and_send_d2c(iotHubClientHandle);
        dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
        ASSERT_IS_FALSE_WITH_MSG(dataWasRecv, "Service still answering..."); // was received by the callback...

                                                                             // Send the Event from the client
        (void)printf("Send message after the server fault and wait for confirmation...\r\n");
        d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

        // Wait for confirmation that the event was recevied
        dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
        ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...
    }
    else if ((strcmp(faultOperationType, "ShutDownAmqp") == 0) ||
        (strcmp(faultOperationType, "ShutDownMqtt") == 0))
    {
        ThreadAPI_Sleep(3000);
        // Send the Event from the client and expect no answer and no recovery (after 60 sec wait)
        (void)printf("ShutDownAmqp - Sending message and expect no confirmation...\r\n");
        d2cMessage = client_create_and_send_d2c(iotHubClientHandle);
        dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
        ASSERT_IS_FALSE_WITH_MSG(dataWasRecv, "Service still answering..."); // was received by the callback...
    }

    // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    /* guess who */
    (void)platform_init();

    // Waigt for the message to arrive
    service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage);

    // cleanup
    destroy_d2c_message_handle(d2cMessage);
}


EXPECTED_RECEIVE_DATA *service_create_c2d(const char *content)
{
    IOTHUB_MESSAGE_RESULT iotHubMessageResult;
    IOTHUB_MESSAGE_HANDLE messageHandle;

    EXPECTED_RECEIVE_DATA *receiveUserContext = ReceiveUserContext_Create();
    ASSERT_IS_NOT_NULL_WITH_MSG(receiveUserContext, "Could not create receive user context");

    messageHandle = IoTHubMessage_CreateFromString(content);
    ASSERT_IS_NOT_NULL_WITH_MSG(messageHandle, "Could not create IoTHubMessage to send C2D messages to the device");

    iotHubMessageResult = IoTHubMessage_SetMessageId(messageHandle, MSG_ID);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    iotHubMessageResult = IoTHubMessage_SetCorrelationId(messageHandle, MSG_CORRELATION_ID);
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessageResult);

    MAP_HANDLE mapHandle = IoTHubMessage_Properties(messageHandle);
    for (size_t i = 0; i < MSG_PROP_COUNT; i++)
    {
        if (Map_AddOrUpdate(mapHandle, MSG_PROP_KEYS[i], MSG_PROP_VALS[i]) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate failed for property %zu!\r\n", i);
        }
    }

    receiveUserContext->msgHandle = messageHandle;

    return receiveUserContext;
}

static void service_send_c2d(IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle, EXPECTED_RECEIVE_DATA* receiveUserContext, IOTHUB_PROVISIONED_DEVICE* deviceToUse)
{
    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;

    iotHubMessagingResult = IoTHubMessaging_SendAsync(iotHubMessagingHandle, deviceToUse->deviceId, receiveUserContext->msgHandle, sendCompleteCallback, receiveUserContext);
    ASSERT_ARE_EQUAL_WITH_MSG(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult, "IoTHubMessaging_SendAsync failed, could not send C2D message to the device");
}

void client_wait_for_c2d_event_arrival(EXPECTED_RECEIVE_DATA* receiveUserContext)
{
    time_t beginOperation, nowTime;

    beginOperation = time(NULL);
    while (
        (nowTime = time(NULL)), (difftime(nowTime, beginOperation) < MAX_CLOUD_TRAVEL_TIME) //time box
        )
    {
        if (Lock(receiveUserContext->lock) != LOCK_OK)
        {
            ASSERT_FAIL("unable to lock");
        }
        else
        {
            if (receiveUserContext->wasFound)
            {
                (void)Unlock(receiveUserContext->lock);
                break;
            }
            (void)Unlock(receiveUserContext->lock);
        }
        ThreadAPI_Sleep(100);
    }

}

static void recv_message_test(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    // arrange
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;

    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
    IOTHUB_CLIENT_RESULT result;

    EXPECTED_RECEIVE_DATA* receiveUserContext;

    // Create device client
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Create receive context
    receiveUserContext = service_create_c2d(MSG_CONTENT);

    // Set callback
    result = IoTHubClient_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, receiveUserContext);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Setting message callback failed");

    // Create Service Client
    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubServiceClientHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL (int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Send message
    service_send_c2d(iotHubMessagingHandle, receiveUserContext, deviceToUse);

    // wait for message to arrive on client
    client_wait_for_c2d_event_arrival(receiveUserContext);

    // assert
    ASSERT_IS_TRUE_WITH_MSG(receiveUserContext->wasFound, "Failure retrieving data from C2D"); // was found is written by the callback...

    // cleanup
    IoTHubMessaging_Close(iotHubMessagingHandle);
    IoTHubMessaging_Destroy(iotHubMessagingHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);

    IoTHubClient_Destroy(iotHubClientHandle);
    ReceiveUserContext_Destroy(receiveUserContext);
}

void e2e_recv_message_test_sas(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    recv_message_test(IoTHubAccount_GetSASDevice(g_iothubAcctInfo), protocol);
}

void e2e_recv_message_test_x509(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    recv_message_test(IoTHubAccount_GetX509Device(g_iothubAcctInfo), protocol);
}

void e2e_c2d_with_svc_fault_ctrl(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, const char* faultOperationType, const char* faultOperationCloseReason, const char* faultOperationDelayInSecs)
{
    // arrange
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle;
    IOTHUB_MESSAGING_CLIENT_HANDLE iotHubMessagingHandle;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;

    IOTHUB_MESSAGING_RESULT iotHubMessagingResult;
    IOTHUB_CLIENT_RESULT result;

    EXPECTED_RECEIVE_DATA* receiveUserContext;

    D2C_MESSAGE_HANDLE d2cMessage;
    IOTHUB_PROVISIONED_DEVICE* deviceToUse = IoTHubAccount_GetSASDevice(g_iothubAcctInfo);

    MAP_HANDLE propMap = Map_Create(NULL);

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationType", faultOperationType) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationType!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationCloseReason", faultOperationCloseReason) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationCloseReason!");
    }

    if (Map_AddOrUpdate(propMap, "AzIoTHub_FaultOperationDelayInSecs", faultOperationDelayInSecs) != MAP_OK)
    {
        ASSERT_FAIL("Map_AddOrUpdate failed for AzIoTHub_FaultOperationDelayInSecs!");
    }

    // Create device client
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Create receive context
    receiveUserContext = service_create_c2d(MSG_CONTENT);

    // Set callback
    result = IoTHubClient_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, receiveUserContext);
    ASSERT_ARE_EQUAL_WITH_MSG(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result, "Setting message callback failed");

    // Create Service Client
    iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(IoTHubAccount_GetIoTHubConnString(g_iothubAcctInfo));
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubServiceClientHandle, "Could not initialize IoTHubServiceClient to send C2D messages to the device");

    iotHubMessagingHandle = IoTHubMessaging_Create(iotHubServiceClientHandle);
    ASSERT_IS_NOT_NULL_WITH_MSG(iotHubMessagingHandle, "Could not initialize IoTHubMessaging to send C2D messages to the device");

    iotHubMessagingResult = IoTHubMessaging_Open(iotHubMessagingHandle, openCompleteCallback, (void*)"Context string for open");
    ASSERT_ARE_EQUAL(int, IOTHUB_MESSAGING_OK, iotHubMessagingResult);

    // Send message
    service_send_c2d(iotHubMessagingHandle, receiveUserContext, deviceToUse);
    // wait for message to arrive on client
    client_wait_for_c2d_event_arrival(receiveUserContext);
    // assert
    ASSERT_IS_TRUE_WITH_MSG(receiveUserContext->wasFound, "Failure retrieving data from C2D"); // was found is written by the callback...

    (void)printf("Send server fault control message and wait for confirmation...\r\n");
    clear_connection_status_info_flags();
    d2cMessage = client_create_with_properies_and_send_d2c(iotHubClientHandle, propMap);

    (void)printf("Send server fault control message and wait for confirmation...\r\n");
    clear_connection_status_info_flags();
    d2cMessage = client_create_with_properies_and_send_d2c(iotHubClientHandle, propMap);

    // Wait for confirmation that the fault control was received
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending fault control message to IotHub"); // was received by the callback...

    ThreadAPI_Sleep(10000);

    // Send message
    receiveUserContext->wasFound = false;
    service_send_c2d(iotHubMessagingHandle, receiveUserContext, deviceToUse);
    // wait for message to arrive on client
    client_wait_for_c2d_event_arrival(receiveUserContext);
    // assert
    ASSERT_IS_TRUE_WITH_MSG(receiveUserContext->wasFound, "Failure retrieving data from C2D"); // was found is written by the callback...

                                                                                               // cleanup
    IoTHubMessaging_Close(iotHubMessagingHandle);
    IoTHubMessaging_Destroy(iotHubMessagingHandle);
    IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);

    IoTHubClient_Destroy(iotHubClientHandle);
    ReceiveUserContext_Destroy(receiveUserContext);
}

//***********************************************************
// D2C
//***********************************************************
void e2e_d2c_svc_fault_ctrl_kill_TCP_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl(protocol, "KillTcp", "boom", "1");
}

void e2e_d2c_svc_fault_ctrl_kill_TCP_connection_with_transport_status_check(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillTcp", "boom", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpConnection", "Connection fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_session(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpSession", "Session fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_request_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpCBSLinkReq", "CBS request link fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_CBS_response_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpCBSLinkResp", "CBS response link fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_D2C_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpD2CLink", "D2C link fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_kill_C2D_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_with_transport_status(protocol, "KillAmqpC2DLink", "C2D link fault", "1");
}

void e2e_d2c_svc_fault_ctrl_AMQP_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeThrottling", "20", "1", true);
}

void e2e_d2c_svc_fault_ctrl_AMQP_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeMaxMessageQuota", "20", "1", true);
}

void e2e_d2c_svc_fault_ctrl_AMQP_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeAuthError", "20", "1", true);
}

void e2e_d2c_svc_fault_ctrl_AMQP_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "ShutDownAmqp", "byebye", "1", true);
}

void e2e_d2c_svc_fault_ctrl_MQTT_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeThrottling", "20", "1", false);
}

void e2e_d2c_svc_fault_ctrl_MQTT_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeMaxMessageQuota", "20", "1", false);
}

void e2e_d2c_svc_fault_ctrl_MQTT_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "InvokeAuthError", "20", "1", false);
}

void e2e_d2c_svc_fault_ctrl_MQTT_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_d2c_with_svc_fault_ctrl_no_answer(protocol, "ShutDownMqtt", "byebye", "1", false);
}

//***********************************************************
// C2D
//***********************************************************
void e2e_c2d_svc_fault_ctrl_kill_TCP_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillTcp", "boom", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_connection(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpConnection", "Connection fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_session(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpSession", "Session fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_request_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpCBSLinkReq", "CBS request link fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_CBS_response_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpCBSLinkResp", "CBS response link fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_D2C_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpc2dLink", "c2d link fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_kill_C2D_link(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "KillAmqpC2DLink", "C2D link fault", "1");
}

void e2e_c2d_svc_fault_ctrl_AMQP_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "ShutDownAmqp", "byebye", "1");
}

void e2e_c2d_svc_fault_ctrl_throttling_reconnect(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "InvokeThrottling", "20", "1");
}

void e2e_c2d_svc_fault_ctrl_message_quota_exceeded(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "InvokeMaxMessageQuota", "20", "1");
}

void e2e_c2d_svc_fault_ctrl_auth_error(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "InvokeAuthError", "20", "1");
}

void e2e_c2d_svc_fault_ctrl_MQTT_shut_down(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    e2e_c2d_with_svc_fault_ctrl(protocol, "ShutDownMqtt", "byebye", "1");
}
