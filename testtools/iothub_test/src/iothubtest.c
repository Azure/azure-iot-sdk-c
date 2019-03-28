// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cctype>
#include <cmath>
#include <climits>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#endif

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/urlencode.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_c_shared_utility/threadapi.h"
#include "iothubtest.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/consolelogger.h"
#include "azure_uamqp_c/amqp_definitions.h"
#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/cbs.h"
#ifdef SET_TRUSTED_CERT
#include "certs.h"
#endif // SET_TRUSTED_CERT

const char* AMQP_RECV_ADDRESS_FMT = "%s/ConsumerGroups/%s/Partitions/%u";
const char* AMQP_ADDRESS_PATH_FMT = "/devices/%s/messages/deviceBound";
const char* AMQP_SEND_TARGET_ADDRESS_FMT = "amqps://%s/messages/deviceBound";
const char* AMQP_SEND_AUTHCID_FMT = "iothubowner@sas.root.%s";

#define THREAD_CONTINUE             0
#define THREAD_END                  1
#define MAX_DRAIN_TIME              1000.0
#define MAX_SHORT_VALUE             32767         /* maximum (signed) short value */
#define INDEFINITE_TIME             ((time_t)-1)

MU_DEFINE_ENUM_STRINGS(IOTHUB_TEST_CLIENT_RESULT, IOTHUB_TEST_CLIENT_RESULT_VALUES);

typedef enum MESSAGE_SEND_STATE_TAG
{
    MESSAGE_SEND_STATE_NOT_SENT,
    MESSAGE_SEND_STATE_SEND_IN_PROGRESS,
    MESSAGE_SEND_STATE_SENT_OK,
    MESSAGE_SEND_STATE_SEND_FAILED
} MESSAGE_SEND_STATE;

typedef struct AMQP_CONN_INFO_TAG
{
    XIO_HANDLE tls_io;
    SASL_MECHANISM_HANDLE sasl_mechanism;
    XIO_HANDLE sasl_io;
    CONNECTION_HANDLE connection;
    SESSION_HANDLE session;
    LINK_HANDLE receive_link;
    MESSAGE_RECEIVER_HANDLE message_receiver;
} AMQP_CONN_INFO;

typedef struct IOTHUB_VALIDATION_INFO_TAG
{
    char* iotHubName;
    char* hostName;
    char* partnerName;
    char* partnerHost;
    char* consumerGroup;
    char* deviceId;
    char* eventhubName;
    char* iotSharedSig;
    char* eventhubAccessKey;
    volatile sig_atomic_t messageThreadExit;
    AMQP_CONN_INFO *amqp_connection;
    pfIoTHubMessageCallback onMessageReceivedCallback;
    void* onMessageReceivedContext;
    THREAD_HANDLE asyncWorkThread;
    bool keepThreadAlive;
} IOTHUB_VALIDATION_INFO;

typedef struct MESSAGE_RECEIVER_CONTEXT_TAG
{
    pfIoTHubMessageCallback msgCallback;
    void* context;
    bool message_received;
} MESSAGE_RECEIVER_CONTEXT;

unsigned int ConvertToUnsignedInt(const unsigned char data[], int position)
{
    unsigned int result;
    if (data == NULL)
    {
        result = 0;
    }
    else
    {
        result = 0;
        for (int nIndex = 0; nIndex < 4; nIndex++)
        {
            int nTest = data[nIndex+position];
            nTest <<= (nIndex*8);
            result += nTest;
        }
    }
    return result;
}

static unsigned char* GetByteArray(const char* pszData, size_t* bufferLen)
{
    unsigned char* result;
    if (bufferLen != NULL)
    {
        *bufferLen = strlen(pszData);
        result = (unsigned char*)malloc(*bufferLen);

        size_t nIndex;
        for (nIndex = 0; nIndex < *bufferLen; nIndex++)
        {
            result[nIndex] = (unsigned char)toupper(pszData[nIndex]);
        }
    }
    else
    {
        result = NULL;
    }
    return result;
}

void ComputeHash(const unsigned char pszData[], size_t dataLen, unsigned int nSeed1, unsigned int nSeed2, unsigned int* hash1, unsigned int* hash2)
{
    unsigned int nVal1, nVal2, nVal3;

    nVal1 = nVal2 = nVal3 = (unsigned int)(0xdeadbeef + dataLen + nSeed1);
    nVal3 += nSeed2;

    int nIndex = 0;
    size_t nLen = dataLen;
    while (nLen > 12)
    {
        nVal1 += ConvertToUnsignedInt(pszData, nIndex);
        nVal2 += ConvertToUnsignedInt(pszData, nIndex+4);
        nVal3 += ConvertToUnsignedInt(pszData, nIndex+8);

        nVal1 -= nVal3;
        nVal1 ^= (nVal3 << 4) | (nVal3 >> 28);
        nVal3 += nVal2;

        nVal2 -= nVal1;
        nVal2 ^= (nVal1 << 6) | (nVal1 >> 26);
        nVal1 += nVal3;

        nVal3 -= nVal2;
        nVal3 ^= (nVal2 << 8) | (nVal2 >> 24);
        nVal2 += nVal1;

        nVal1 -= nVal3;
        nVal1 ^= (nVal3 << 16) | (nVal3 >> 16);
        nVal3 += nVal2;

        nVal2 -= nVal1;
        nVal2 ^= (nVal1 << 19) | (nVal1 >> 13);
        nVal1 += nVal3;

        nVal3 -= nVal2;
        nVal3 ^= (nVal2 << 4) | (nVal2 >> 28);
        nVal2 += nVal1;

        nIndex += 12;
        nLen -= 12;
    }
    switch (nLen)
    {
        case 12:
            nVal1 += ConvertToUnsignedInt(pszData, nIndex);
            nVal2 += ConvertToUnsignedInt(pszData, nIndex+4);
            nVal3 += ConvertToUnsignedInt(pszData, nIndex+8);
            break;
        case 11: // No break;
            nVal3 += ((unsigned int)pszData[nIndex + 10]) << 16;
        case 10: // No break;
            nVal3 += ((unsigned int)pszData[nIndex + 9]) << 8;
        case 9: // No break;
            nVal3 += (unsigned int)pszData[nIndex + 8];
        case 8:
            nVal2 += ConvertToUnsignedInt(pszData, nIndex+4);
            nVal1 += ConvertToUnsignedInt(pszData, nIndex);
            break;
        case 7:// No break
            nVal2 += ((unsigned int)pszData[nIndex + 6]) << 16;
        case 6:// No break
            nVal2 += ((unsigned int)pszData[nIndex + 5]) << 8;
        case 5:// No break
            nVal2 += ((unsigned int)pszData[nIndex + 4]);
        case 4:
            nVal1 += ConvertToUnsignedInt(pszData, nIndex);
            break;
        case 3:// No break
            nVal1 += ((unsigned int)pszData[nIndex + 2]) << 16;
        case 2:// No break
            nVal1 += ((unsigned int)pszData[nIndex + 1]) << 8;
        case 1:
            nVal1 += (unsigned int)pszData[nIndex];
            break;
        default:
        case 0:
            *hash1 = nVal3;
            *hash2 = nVal2;
            return;
    }

    nVal3 ^= nVal2;
    nVal3 -= (nVal2 << 14) | (nVal2 >> 18);

    nVal1 ^= nVal3;
    nVal1 -= (nVal3 << 11) | (nVal3 >> 21);

    nVal2 ^= nVal1;
    nVal2 -= (nVal1 << 25) | (nVal1 >> 7);

    nVal3 ^= nVal2;
    nVal3 -= (nVal2 << 16) | (nVal2 >> 16);

    nVal1 ^= nVal3;
    nVal1 -= (nVal3 << 4) | (nVal3 >> 28);

    nVal2 ^= nVal1;
    nVal2 -= (nVal1 << 14) | (nVal1 >> 18);

    nVal3 ^= nVal2;
    nVal3 -= (nVal2 << 24) | (nVal2 >> 8);

    *hash1 = nVal3;
    *hash2 = nVal2;
}

static size_t ResolvePartitionIndex(const char* partitionKey, size_t maxPartition)
{
    size_t result;

    // Normalize the Partition Key to be upper case
    size_t len = 0;
    unsigned char* byteArray = GetByteArray(partitionKey, &len);
    if (byteArray == NULL)
    {
        // On failure look at the zero partition
        LogError("Failure Getting Byte Array in ResolvePartitionIndex.");
        result = 0;
    }
    else
    {
        int defaultLogicalPartitionCount = MAX_SHORT_VALUE;
        unsigned int hash1 = 0, hash2 = 0;
        ComputeHash(byteArray, len, 0, 0, &hash1, &hash2);
        unsigned long hashedValue = hash1 ^ hash2;

        // Intended Value truncation from UINT to short
        short sTruncateVal = (short)hashedValue;
        short logicalPartition = (short)abs(sTruncateVal % defaultLogicalPartitionCount);

        double shortRangeWidth = floor((double)defaultLogicalPartitionCount/ (double)maxPartition);
        int remainingLogicalPartitions = defaultLogicalPartitionCount - ((int)maxPartition * (int)shortRangeWidth);
        int largeRangeWidth = ( (int)shortRangeWidth) + 1;
        int largeRangesLogicalPartitions = largeRangeWidth * remainingLogicalPartitions;
        result = logicalPartition < largeRangesLogicalPartitions ? logicalPartition / largeRangeWidth : remainingLogicalPartitions + ((logicalPartition - largeRangesLogicalPartitions) / (int)shortRangeWidth);

        free(byteArray);
    }
    return result;
}

static int RetrieveIotHubClientInfo(const char* pszIotConnString, IOTHUB_VALIDATION_INFO* dvhInfo)
{
    int result;
    int beginName, endName;
    int beginHost, endHost;

    if (sscanf(pszIotConnString, "HostName=%n%*[^.]%n.%n%*[^;];%nSharedAccessKeyName=*;SharedAccessKey=*", &beginName, &endName, &beginHost, &endHost) != 0)
    {
        LogError("Failure determinging string sizes in RetrieveIotHubClientInfo.");
        result = MU_FAILURE;
    }
    else
    {
        if ( (dvhInfo->iotHubName = (char*)malloc(endName-beginName+1) ) == NULL)
        {
            LogError("Failure allocating iothubName in RetrieveIotHubClientInfo endName: %d beginName: %d.", endName, beginName);
            result = MU_FAILURE;
        }
        else if ( (dvhInfo->hostName = (char*)malloc(endHost-beginName+1) ) == NULL)
        {
            LogError("Failure allocating hostName in RetrieveIotHubClientInfo endHost: %d beginHost: %d.", endHost, beginName);
            free(dvhInfo->iotHubName);
            result = MU_FAILURE;
        }
        else if (sscanf(pszIotConnString, "HostName=%[^.].%[^;];SharedAccessKeyName=*;SharedAccessKey=*", dvhInfo->iotHubName, dvhInfo->hostName + endName - beginName + 1) != 2)
        {
            LogError("Failure retrieving string values in RetrieveIotHubClientInfo.");
            free(dvhInfo->iotHubName);
            free(dvhInfo->hostName);
            result = MU_FAILURE;
        }
        else
        {
            (void)strcpy(dvhInfo->hostName, dvhInfo->iotHubName);
            dvhInfo->hostName[endName - beginName] = '.';
            result = 0;
        }
    }
    return result;
}

static int RetrieveEventHubClientInfo(const char* pszconnString, IOTHUB_VALIDATION_INFO* dvhInfo)
{
    int result;
    int beginName, endName;
    int beginHost, endHost;

    if (sscanf(pszconnString, "Endpoint=sb://%n%*[^.]%n.%n%*[^/]%n/;SharedAccessKeyName=owner;SharedAccessKey=%*s", &beginName, &endName, &beginHost, &endHost) != 0)
    {
        LogError("Failure determining string sizes in RetrieveEventHubClientInfo.");
        result = MU_FAILURE;
    }
    else
    {
        if ( (dvhInfo->partnerName = (char*)malloc(endName+beginName+1) ) == NULL)
        {
            LogError("Failure allocating partnerName in RetrieveEventHubClientInfo endName: %d beginName: %d.", endName, beginName);
            result = MU_FAILURE;
        }
        else if ( (dvhInfo->partnerHost = (char*)malloc(endHost+beginHost+1) ) == NULL)
        {
            LogError("Failure allocating partnerHost in RetrieveEventHubClientInfo endHost: %d beginHost: %d.", endHost, beginHost);
            free(dvhInfo->partnerName);
            result = MU_FAILURE;
        }
        else if (sscanf(pszconnString, "Endpoint=sb://%[^.].%[^/]/;SharedAccessKeyName=owner;SharedAccessKey=%*s", dvhInfo->partnerName, dvhInfo->partnerHost) != 2)
        {
            LogError("Failure retrieving string values in RetrieveEventHubClientInfo.");
            free(dvhInfo->partnerName);
            free(dvhInfo->partnerHost);
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

void IoTHubTest_Deinit(IOTHUB_TEST_HANDLE devhubHandle)
{
    if (devhubHandle != NULL)
    {
        IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)devhubHandle;

        if (devhubValInfo->consumerGroup != NULL)
        {
            free(devhubValInfo->consumerGroup);
        }

        if (devhubValInfo->eventhubAccessKey != NULL)
        {
            free(devhubValInfo->eventhubAccessKey);
        }

        if (devhubValInfo->deviceId != NULL)
        {
            free(devhubValInfo->deviceId);
        }

        if (devhubValInfo->eventhubName != NULL)
        {
            free(devhubValInfo->eventhubName);
        }

        if (devhubValInfo->iotSharedSig != NULL)
        {
            free(devhubValInfo->iotSharedSig);
        }

        if (devhubValInfo->iotHubName != NULL)
        {
            free(devhubValInfo->iotHubName);
        }

        if (devhubValInfo->partnerName != NULL)
        {
            free(devhubValInfo->partnerName);
        }

        if (devhubValInfo->partnerHost != NULL)
        {
            free(devhubValInfo->partnerHost);
        }

        if (devhubValInfo->hostName != NULL)
        {
            free(devhubValInfo->hostName);
        }

        free(devhubValInfo);
    }
}

IOTHUB_TEST_HANDLE IoTHubTest_Initialize(const char* eventhubConnString, const char* iothubConnString, const char* deviceId, const char* eventhubName, const char* eventhubAccessKey, const char* sharedSignature, const char* consumerGroup)
{
    IOTHUB_TEST_HANDLE result;
    IOTHUB_VALIDATION_INFO* devhubValInfo;

    if (eventhubConnString == NULL || iothubConnString == NULL || deviceId == NULL || eventhubName == NULL || sharedSignature == NULL || eventhubAccessKey == NULL)
    {
        LogError("Invalid parameter sent to Initialize Eventhub conn string: 0x%p\r\niothub Conn string 0x%p\r\ndeviceId 0x%p\r\n EventhubName 0x%p\r\nAccessKey 0x%p\r\nSharedSig 0x%p.", eventhubConnString, iothubConnString, deviceId, eventhubName, eventhubAccessKey, sharedSignature);
        result = NULL;
    }
    else if ( (devhubValInfo = malloc(sizeof(IOTHUB_VALIDATION_INFO) ) ) == NULL)
    {
        LogError("Failure allocating devicehub Validation Info.");
        result = NULL;
    }
    else
    {
        memset(devhubValInfo, 0, sizeof(IOTHUB_VALIDATION_INFO));

        if (mallocAndStrcpy_s(&devhubValInfo->consumerGroup, consumerGroup) != 0)
        {
            LogError("Failure allocating consumerGroup string.");
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&devhubValInfo->deviceId, deviceId) != 0)
        {
            LogError("Failure allocating deviceId string.");
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&devhubValInfo->eventhubAccessKey, eventhubAccessKey) != 0)
        {
            LogError("Failure allocating eventhubAccessKey string.");
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&devhubValInfo->eventhubName, eventhubName) != 0)
        {
            LogError("Failure allocating eventhubName string.");
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&devhubValInfo->iotSharedSig, sharedSignature) != 0)
        {
            LogError("Failure allocating sharedSig string.");
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (RetrieveIotHubClientInfo(iothubConnString, devhubValInfo) != 0)
        {
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else if (RetrieveEventHubClientInfo(eventhubConnString, devhubValInfo) != 0)
        {
            IoTHubTest_Deinit(devhubValInfo);
            result = NULL;
        }
        else
        {
            result = devhubValInfo;
        }
    }

    return result;
}

static char* CreateReceiveAddress(IOTHUB_VALIDATION_INFO* devhubValInfo, size_t partitionCount)
{
    char* result;
    size_t addressLen = strlen(AMQP_RECV_ADDRESS_FMT) + strlen(devhubValInfo->eventhubName) + strlen(devhubValInfo->consumerGroup) + 5;
    result = (char*)malloc(addressLen + 1);
    if (result != NULL)
    {
        size_t targetPartition = ResolvePartitionIndex(devhubValInfo->deviceId, partitionCount);
        sprintf_s(result, addressLen+1, AMQP_RECV_ADDRESS_FMT, devhubValInfo->eventhubName, devhubValInfo->consumerGroup, targetPartition);
    }
    else
    {
        LogError("Failure allocating recieving address string.");
        result = NULL;
    }
    return result;
}

static char* CreateReceiveHostName(IOTHUB_VALIDATION_INFO* devhubValInfo)
{
    char* result;
    size_t partner_host_len = strlen(devhubValInfo->partnerName) + strlen(devhubValInfo->partnerHost) + 2;
    result = (char*)malloc(partner_host_len + 1);
    if (result != NULL)
    {
        sprintf_s(result, partner_host_len + 1, "%s.%s", devhubValInfo->partnerName, devhubValInfo->partnerHost);
    }
    else
    {
        LogError("Failure allocating data in CreateReceiveHostName.");
        result = NULL;
    }
    return result;
}

static char* CreateSendTargetAddress(IOTHUB_VALIDATION_INFO* devhubValInfo)
{
    char* result;
    size_t addressLen = strlen(AMQP_SEND_TARGET_ADDRESS_FMT)+strlen(devhubValInfo->hostName);
    result = (char*)malloc(addressLen+1);
    if (result != NULL)
    {
        sprintf_s(result, addressLen+1, AMQP_SEND_TARGET_ADDRESS_FMT, devhubValInfo->hostName);
    }
    else
    {
        LogError("Failure allocating data in CreateSendTargetAddress.");
    }
    return result;
}

/* RFC SASL PLAIN, we construct the AuthCid here (http://tools.ietf.org/html/rfc4616) */
static char* CreateSendAuthCid(IOTHUB_VALIDATION_INFO* devhubValInfo)
{
    char* result;

    size_t authCidLen = strlen(AMQP_SEND_AUTHCID_FMT) + strlen(devhubValInfo->iotHubName);
    result = (char*)malloc(authCidLen + 1);
    if (result != NULL)
    {
        sprintf_s(result, authCidLen + 1, AMQP_SEND_AUTHCID_FMT, devhubValInfo->iotHubName);
    }
    else
    {
        LogError("Failure allocating data in CreateSendAuthCid.");
    }
    return result;
}

static AMQP_VALUE on_message_received(const void* context, MESSAGE_HANDLE message)
{
    MESSAGE_RECEIVER_CONTEXT* msg_received_context = (MESSAGE_RECEIVER_CONTEXT*)context;
    BINARY_DATA binary_data;

    if (message_get_body_amqp_data_in_place(message, 0, &binary_data) == 0)
    {
        if (msg_received_context->msgCallback != NULL)
        {
            if (msg_received_context->msgCallback(msg_received_context->context, (const char*)binary_data.bytes, binary_data.length) != 0)
            {
                msg_received_context->message_received = true;
            }
        }
    }

    return messaging_delivery_accepted();
}

// After this new code is seasoned and confirmed good, there shall be some consolidation of *Listen* APIs and their callbacks.
static AMQP_VALUE on_message_received_new(const void* context, MESSAGE_HANDLE message)
{
    AMQP_VALUE result;

    IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)context;

    if (devhubValInfo->onMessageReceivedCallback == NULL)
    {
        result = messaging_delivery_released();
    }
    else
    {
        BINARY_DATA binary_data;

        if (message_get_body_amqp_data_in_place(message, 0, &binary_data) != 0)
        {
            LogError("Failed getting incoming message body");
            result = messaging_delivery_rejected("failed parsing body", "failed parsing body");
        }
        else
        {
            if (devhubValInfo->onMessageReceivedCallback(devhubValInfo->onMessageReceivedContext, (const char*)binary_data.bytes, binary_data.length) == 0)
            {
                result = messaging_delivery_accepted();
            }
            else
            {
                result = messaging_delivery_released();
            }
        }
    }

    return result;
}

static void destroyAmqpConnection(AMQP_CONN_INFO* amqp_connection)
{
    if (amqp_connection->message_receiver != NULL)
    {
        messagereceiver_destroy(amqp_connection->message_receiver);
        amqp_connection->message_receiver = NULL;
    }

    if (amqp_connection->receive_link != NULL)
    {
        link_destroy(amqp_connection->receive_link);
        amqp_connection->receive_link = NULL;
    }

    if (amqp_connection->session != NULL)
    {
        session_destroy(amqp_connection->session);
        amqp_connection->session = NULL;
    }

    if (amqp_connection->connection != NULL)
    {
        connection_destroy(amqp_connection->connection);
        amqp_connection->connection = NULL;
    }

    if (amqp_connection->sasl_io != NULL)
    {
        xio_destroy(amqp_connection->sasl_io);
        amqp_connection->sasl_io = NULL;
    }

    if (amqp_connection->sasl_mechanism != NULL)
    {
        saslmechanism_destroy(amqp_connection->sasl_mechanism);
        amqp_connection->sasl_mechanism = NULL;
    }

    if (amqp_connection->tls_io != NULL)
    {
        xio_destroy(amqp_connection->tls_io);
        amqp_connection->tls_io = NULL;
    }
}

static int asyncWorkFunction(void* context)
{
    IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)context;

    while (devhubValInfo->keepThreadAlive)
    {
        if (devhubValInfo->amqp_connection != NULL)
        {
            connection_dowork(devhubValInfo->amqp_connection->connection);
        }
    }

    return 0;
}

static filter_set create_link_source_filter(time_t receiveTimeRangeStart)
{
    filter_set result;
    char tempBuffer[256];

    if ((sprintf(tempBuffer, "amqp.annotation.x-opt-enqueuedtimeutc > %llu", ((unsigned long long)receiveTimeRangeStart - 330) * 1000)) < 0)
    {
        LogError("Failed creating filter set with enqueuedtimeutc filter.");
        result = NULL;
    }
    else
    {
        if ((result = amqpvalue_create_map()) == NULL)
        {
            LogError("Failed creating map");
        }
        else
        {
            const char filter_name[] = "apache.org:selector-filter:string";
            AMQP_VALUE descriptor;
            AMQP_VALUE filter_value;
            AMQP_VALUE described_filter_value;

            if ((filter_value = amqpvalue_create_string(tempBuffer)) == NULL)
            {
                LogError("Failed creating filter value");
                amqpvalue_destroy(result);
                result = NULL;
            }
            else if ((descriptor = amqpvalue_create_symbol(filter_name)) == NULL)
            {
                LogError("Failed creating descriptor");
                amqpvalue_destroy(filter_value);
                amqpvalue_destroy(result);
                result = NULL;
            }
            else if ((described_filter_value = amqpvalue_create_described(descriptor, filter_value)) == NULL)
            {
                LogError("Failed creating described filter value");
                amqpvalue_destroy(descriptor);
                amqpvalue_destroy(filter_value);
                amqpvalue_destroy(result);
                result = NULL;
            }
            else
            {
                AMQP_VALUE filter_key;

                if ((filter_key = amqpvalue_create_symbol(filter_name)) == NULL)
                {
                    LogError("Failed creating filter key");
                    amqpvalue_destroy(described_filter_value);
                    amqpvalue_destroy(descriptor);
                    amqpvalue_destroy(filter_value);
                    amqpvalue_destroy(result);
                    result = NULL;
                }
                else
                {
                    if (amqpvalue_set_map_value(result, filter_key, described_filter_value) != 0)
                    {
                        LogError("Failed setting map value");
                        amqpvalue_destroy(described_filter_value);
                        amqpvalue_destroy(descriptor);
                        amqpvalue_destroy(filter_value);
                        amqpvalue_destroy(result);
                        result = NULL;
                    }

                    amqpvalue_destroy(filter_key);
                }
            }
        }
    }

    return result;
}

static AMQP_VALUE create_link_source(char* receive_address, filter_set filter_set)
{
    AMQP_VALUE result;
    SOURCE_HANDLE source_handle;

    if ((source_handle = source_create()) == NULL)
    {
        LogError("Failed creating source handle");
        result = NULL;
    }
    else
    {
        AMQP_VALUE address_value;

        if ((address_value = amqpvalue_create_string(receive_address)) == NULL)
        {
            LogError("Failed creating the AMQP value for the receive address");
            result = NULL;
        }
        else
        {
            if (source_set_address(source_handle, address_value) != 0)
            {
                LogError("Failed setting source address");
                result = NULL;
            }
            else if (source_set_filter(source_handle, filter_set) != 0)
            {
                LogError("Failed setting source filter");
                result = NULL;
            }
            else if ((result = amqpvalue_create_source(source_handle)) == NULL)
            {
                LogError("Failed creating the link source");
                result = NULL;
            }

            amqpvalue_destroy(address_value);
        }

        source_destroy(source_handle);
    }

    return result;
}

static AMQP_CONN_INFO* createAmqpConnection(IOTHUB_VALIDATION_INFO* devhubValInfo, size_t partitionCount, time_t receiveTimeRangeStart)
{
    AMQP_CONN_INFO* result;

    if ((result = (AMQP_CONN_INFO*)malloc(sizeof(AMQP_CONN_INFO))) == NULL)
    {
        LogError("Failed to allocate memory for AMQP connection elements");
    }
    else
    {
        char* eh_hostname;

        if ((eh_hostname = CreateReceiveHostName(devhubValInfo)) == NULL)
        {
            LogError("Failed creating receive host name");
            destroyAmqpConnection(result);
            result = NULL;
        }
        else
        {
            char* receive_address;

            if ((receive_address = CreateReceiveAddress(devhubValInfo, partitionCount)) == NULL)
            {
                LogError("Failed creating receive address");
                destroyAmqpConnection(result);
                result = NULL;
            }
            else
            {
                SASL_PLAIN_CONFIG sasl_plain_config;
                const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_plain_interface_description;
                TLSIO_CONFIG tls_io_config;
                const IO_INTERFACE_DESCRIPTION* tlsio_interface;

                sasl_plain_config.authcid = "iothubowner";
                sasl_plain_config.passwd = devhubValInfo->eventhubAccessKey;
                sasl_plain_config.authzid = NULL;

                tls_io_config.hostname = eh_hostname;
                tls_io_config.port = 5671;
                tls_io_config.underlying_io_interface = NULL;
                tls_io_config.underlying_io_parameters = NULL;

                if ((sasl_plain_interface_description = saslplain_get_interface()) == NULL)
                {
                    LogError("Failed getting saslplain_get_interface.");
                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                    result = NULL;
                }
                else if ((result->sasl_mechanism = saslmechanism_create(sasl_plain_interface_description, &sasl_plain_config)) == NULL)
                {
                    LogError("Failed creating sasl PLAN mechanism.");
                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                    result = NULL;
                }
                else if ((tlsio_interface = platform_get_default_tlsio()) == NULL)
                {
                    LogError("Failed getting default TLS IO interface.");
                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                    result = NULL;
                }
                else if ((result->tls_io = xio_create(tlsio_interface, &tls_io_config)) == NULL)
                {
                    LogError("Failed creating the TLS IO.");
                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                    result = NULL;
                }
                else
                {
                    /* create the SASL client IO using the TLS IO */
                    SASLCLIENTIO_CONFIG sasl_io_config;
                    sasl_io_config.underlying_io = result->tls_io;
                    sasl_io_config.sasl_mechanism = result->sasl_mechanism;
                    if ((result->sasl_io = xio_create(saslclientio_get_interface_description(), &sasl_io_config)) == NULL)
                    {
                        LogError("Failed creating the SASL IO.");
                        destroyAmqpConnection(devhubValInfo->amqp_connection);
                        result = NULL;
                    }
                    /* create the connection, session and link */
                    else if ((result->connection = connection_create(result->sasl_io, eh_hostname, "e2etest_link", NULL, NULL)) == NULL)
                    {
                        LogError("Failed creating the connection.");
                        destroyAmqpConnection(devhubValInfo->amqp_connection);
                        result = NULL;
                    }
                    else if ((result->session = session_create(result->connection, NULL, NULL)) == NULL)
                    {
                        LogError("Failed creating the session.");
                        destroyAmqpConnection(devhubValInfo->amqp_connection);
                        result = NULL;
                    }
                    else if (session_set_incoming_window(result->session, 100) != 0)
                    {
                        /* set incoming window to 100 for the session */
                        LogError("Failed setting the session incoming window.");
                        destroyAmqpConnection(devhubValInfo->amqp_connection);
                        result = NULL;
                    }
                    else
                    {
                        {
                            filter_set filter_set;

                            if ((filter_set = create_link_source_filter(receiveTimeRangeStart)) == NULL)
                            {
                                LogError("Failed creating filter set with enqueuedtimeutc filter.");
                                destroyAmqpConnection(devhubValInfo->amqp_connection);
                                result = NULL;
                            }
                            else
                            {
                                AMQP_VALUE source;

                                if ((source = create_link_source(receive_address, filter_set)) == NULL)
                                {
                                    LogError("Failed creating source for link.");
                                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                                    result = NULL;
                                }
                                else
                                {
                                    AMQP_VALUE target;

                                    if ((target = messaging_create_target(receive_address)) == NULL)
                                    {
                                        LogError("Failed creating target for link.");
                                        destroyAmqpConnection(devhubValInfo->amqp_connection);
                                        result = NULL;
                                    }
                                    else
                                    {
                                        if ((result->receive_link = link_create(result->session, "receiver-link", role_receiver, source, target)) == NULL)
                                        {
                                            LogError("Failed creating link.");
                                            destroyAmqpConnection(devhubValInfo->amqp_connection);
                                            result = NULL;
                                        }
                                        else if (link_set_rcv_settle_mode(result->receive_link, receiver_settle_mode_first) != 0)
                                        {
                                            LogError("Failed setting link receive settle mode.");
                                            destroyAmqpConnection(devhubValInfo->amqp_connection);
                                            result = NULL;
                                        }
                                        else if ((result->message_receiver = messagereceiver_create(result->receive_link, NULL, NULL)) == NULL)
                                        {
                                            LogError("Failed creating message receiver.");
                                            destroyAmqpConnection(devhubValInfo->amqp_connection);
                                            result = NULL;
                                        }
                                        else if (messagereceiver_open(result->message_receiver, on_message_received_new, devhubValInfo) != 0)
                                        {
                                            LogError("Failed opening message receiver.");
                                            destroyAmqpConnection(devhubValInfo->amqp_connection);
                                            result = NULL;
                                        }

                                        amqpvalue_destroy(target);
                                    }

                                    amqpvalue_destroy(source);
                                }

                                amqpvalue_destroy(filter_set);
                            }
                        }
                    }
                }

                free(receive_address);
            }

            free(eh_hostname);
        }
    }

    return result;
}

IOTHUB_TEST_CLIENT_RESULT IoTHubTest_ListenForEventAsync(IOTHUB_TEST_HANDLE devhubHandle, size_t partitionCount, time_t receiveTimeRangeStart, pfIoTHubMessageCallback msgCallback, void* context)
{
    IOTHUB_TEST_CLIENT_RESULT result;

    if (devhubHandle == NULL)
    {
        LogError("Invalid argument (devhubHandle=NULL)");
        result = IOTHUB_TEST_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)devhubHandle;

        if (msgCallback == NULL)
        {
            devhubValInfo->keepThreadAlive = false;

            if (devhubValInfo->asyncWorkThread != NULL)
            {
                int threadFunctionResult;
                if (ThreadAPI_Join(devhubValInfo->asyncWorkThread, &threadFunctionResult) != THREADAPI_OK)
                {
                    LogError("Failed to join thread");
                }

                devhubValInfo->asyncWorkThread = NULL;
            }

            if (devhubValInfo->amqp_connection != NULL)
            {
                destroyAmqpConnection(devhubValInfo->amqp_connection);
                devhubValInfo->amqp_connection = NULL;
            }

            devhubValInfo->onMessageReceivedCallback = NULL;
            devhubValInfo->onMessageReceivedContext = NULL;

            result = IOTHUB_TEST_CLIENT_OK;
        }
        else
        {
            if (devhubValInfo->amqp_connection != NULL)
            {
                LogError("Already listening for messages");
                result = IOTHUB_TEST_CLIENT_ERROR;
            }
            else if ((devhubValInfo->amqp_connection = createAmqpConnection(devhubValInfo, partitionCount, receiveTimeRangeStart)) == NULL)
            {
                LogError("Failed creating amqp components to listen for messages");
                result = IOTHUB_TEST_CLIENT_ERROR;
            }
            else
            {
                devhubValInfo->keepThreadAlive = true;
                devhubValInfo->onMessageReceivedCallback = msgCallback;
                devhubValInfo->onMessageReceivedContext = context;

                if (ThreadAPI_Create(&devhubValInfo->asyncWorkThread, asyncWorkFunction, devhubValInfo) != THREADAPI_OK)
                {
                    LogError("Failed creating a thread for amqp DoWork");
                    devhubValInfo->keepThreadAlive = false;
                    destroyAmqpConnection(devhubValInfo->amqp_connection);
                    devhubValInfo->onMessageReceivedCallback = NULL;
                    devhubValInfo->onMessageReceivedContext = NULL;
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else
                {
                    result = IOTHUB_TEST_CLIENT_OK;
                }
            }
        }
    }

    return result;
}


IOTHUB_TEST_CLIENT_RESULT IoTHubTest_ListenForEvent(IOTHUB_TEST_HANDLE devhubHandle, pfIoTHubMessageCallback msgCallback, size_t partitionCount, void* context, time_t receiveTimeRangeStart, double maxDrainTimeInSeconds)
{
    IOTHUB_TEST_CLIENT_RESULT result = 0;
    if (devhubHandle == NULL || msgCallback == NULL)
    {
        LogError("Invalid parameter given in IoTHubTest_ListenForEvent DevhubHandle: 0x%p\r\nMessage Callback: 0x%p.", devhubHandle, msgCallback);
        result = IOTHUB_TEST_CLIENT_ERROR;
    }
    else
    {
        XIO_HANDLE sasl_io = NULL;
        CONNECTION_HANDLE connection = NULL;
        SESSION_HANDLE session = NULL;
        LINK_HANDLE link = NULL;
        IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)devhubHandle;

        char* eh_hostname = CreateReceiveHostName(devhubValInfo);
        if (eh_hostname == NULL)
        {
            result = IOTHUB_TEST_CLIENT_ERROR;
        }
        else
        {
            char* receive_address = CreateReceiveAddress(devhubValInfo, partitionCount);
            if (receive_address == NULL)
            {
                result = IOTHUB_TEST_CLIENT_ERROR;
            }
            else
            {
                /* create SASL plain handler */
                SASL_PLAIN_CONFIG sasl_plain_config;
                sasl_plain_config.authcid = "iothubowner";
                sasl_plain_config.passwd = devhubValInfo->eventhubAccessKey;
                sasl_plain_config.authzid = NULL;
                const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_plain_interface_description;
                SASL_MECHANISM_HANDLE sasl_mechanism_handle = NULL;
                XIO_HANDLE tls_io = NULL;
                TLSIO_CONFIG tls_io_config;
                tls_io_config.hostname = eh_hostname;
                tls_io_config.port = 5671;
                tls_io_config.underlying_io_interface = NULL;
                tls_io_config.underlying_io_parameters = NULL;
                const IO_INTERFACE_DESCRIPTION* tlsio_interface = NULL;

                if ((sasl_plain_interface_description = saslplain_get_interface()) == NULL)
                {
                    LogError("Failed getting saslplain_get_interface.");
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else if ((sasl_mechanism_handle = saslmechanism_create(sasl_plain_interface_description, &sasl_plain_config)) == NULL)
                {
                    LogError("Failed creating sasl PLAN mechanism.");
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else if ((tlsio_interface = platform_get_default_tlsio()) == NULL)
                {
                    LogError("Failed getting default TLS IO interface.");
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else if ((tls_io = xio_create(tlsio_interface, &tls_io_config)) == NULL)
                {
                    LogError("Failed creating the TLS IO.");
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else
                {
#ifdef SET_TRUSTED_CERT
                    xio_setoption(tls_io, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT

                    /* create the SASL client IO using the TLS IO */
                    SASLCLIENTIO_CONFIG sasl_io_config;
                    sasl_io_config.underlying_io = tls_io;
                    sasl_io_config.sasl_mechanism = sasl_mechanism_handle;
                    if ((sasl_io = xio_create(saslclientio_get_interface_description(), &sasl_io_config)) == NULL)
                    {
                        LogError("Failed creating the SASL IO.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    /* create the connection, session and link */
                    else if ((connection = connection_create(sasl_io, eh_hostname, "e2etest_link", NULL, NULL)) == NULL)
                    {
                        LogError("Failed creating the connection.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    else if ((session = session_create(connection, NULL, NULL)) == NULL)
                    {
                        LogError("Failed creating the session.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    else if (session_set_incoming_window(session, 100) != 0)
                    {
                        /* set incoming window to 100 for the session */
                        LogError("Failed setting the session incoming window.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    else
                    {
                        char tempBuffer[256];
                        const char filter_name[] = "apache.org:selector-filter:string";
                        int filter_string_length = sprintf(tempBuffer, "amqp.annotation.x-opt-enqueuedtimeutc > %llu", ((unsigned long long)receiveTimeRangeStart - 330) * 1000);
                        if (filter_string_length < 0)
                        {
                            LogError("Failed creating filter set with enqueuedtimeutc filter.");
                            result = IOTHUB_TEST_CLIENT_ERROR;
                        }
                        else
                        {
                            /* create the filter set to be used for the source of the link */
                            filter_set filter_set = amqpvalue_create_map();
                            AMQP_VALUE filter_key = amqpvalue_create_symbol(filter_name);
                            AMQP_VALUE descriptor = amqpvalue_create_symbol(filter_name);
                            AMQP_VALUE filter_value = amqpvalue_create_string(tempBuffer);
                            AMQP_VALUE described_filter_value = amqpvalue_create_described(descriptor, filter_value);
                            amqpvalue_set_map_value(filter_set, filter_key, described_filter_value);
                            amqpvalue_destroy(filter_key);

                            if (filter_set == NULL)
                            {
                                LogError("Failed creating filter set with enqueuedtimeutc filter.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else
                            {
                                AMQP_VALUE target = NULL;
                                AMQP_VALUE source = NULL;

                                /* create the source of the link */
                                SOURCE_HANDLE source_handle = source_create();
                                AMQP_VALUE address_value = amqpvalue_create_string(receive_address);
                                source_set_address(source_handle, address_value);
                                source_set_filter(source_handle, filter_set);
                                amqpvalue_destroy(address_value);
                                source = amqpvalue_create_source(source_handle);
                                source_destroy(source_handle);

                                if (source == NULL)
                                {
                                    LogError("Failed creating source for link.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else
                                {
                                    target = messaging_create_target(receive_address);
                                    if (target == NULL)
                                    {
                                        LogError("Failed creating target for link.");
                                        result = IOTHUB_TEST_CLIENT_ERROR;
                                    }
                                    else if ((link = link_create(session, "receiver-link", role_receiver, source, target)) == NULL)
                                    {
                                        LogError("Failed creating link.");
                                        result = IOTHUB_TEST_CLIENT_ERROR;
                                    }
                                    else if (link_set_rcv_settle_mode(link, receiver_settle_mode_first) != 0)
                                    {
                                        LogError("Failed setting link receive settle mode.");
                                        result = IOTHUB_TEST_CLIENT_ERROR;
                                    }
                                    else
                                    {
                                        MESSAGE_RECEIVER_CONTEXT message_receiver_context;
                                        message_receiver_context.msgCallback = msgCallback;
                                        message_receiver_context.context = context;
                                        message_receiver_context.message_received = false;
                                        MESSAGE_RECEIVER_HANDLE message_receiver = NULL;

                                        /* create a message receiver */
                                        message_receiver = messagereceiver_create(link, NULL, NULL);
                                        if (message_receiver == NULL)
                                        {
                                            LogError("Failed creating message receiver.");
                                            result = IOTHUB_TEST_CLIENT_ERROR;
                                        }
                                        else if (messagereceiver_open(message_receiver, on_message_received, &message_receiver_context) != 0)
                                        {
                                            LogError("Failed opening message receiver.");
                                            result = IOTHUB_TEST_CLIENT_ERROR;
                                            messagereceiver_destroy(message_receiver);
                                        }
                                        else
                                        {
                                            time_t nowExecutionTime;
                                            time_t beginExecutionTime = time(NULL);
                                            double timespan;

                                            while ((nowExecutionTime = time(NULL)), timespan = difftime(nowExecutionTime, beginExecutionTime), timespan < maxDrainTimeInSeconds)
                                            {
                                                connection_dowork(connection);
                                                ThreadAPI_Sleep(10);

                                                if (message_receiver_context.message_received)
                                                {
                                                    break;
                                                }
                                            }

                                            if (!message_receiver_context.message_received)
                                            {
                                                LogError("No message was received, timed out.");
                                                result = IOTHUB_TEST_CLIENT_ERROR;
                                            }
                                            else
                                            {
                                                result = IOTHUB_TEST_CLIENT_OK;
                                            }
                                            messagereceiver_destroy(message_receiver);
                                        }

                                        amqpvalue_destroy(target);
                                    }
                                    amqpvalue_destroy(source);
                                }
                                amqpvalue_destroy(described_filter_value);
                                amqpvalue_destroy(filter_set);
                            }
                        }
                    }
                    link_destroy(link);
                    session_destroy(session);
                    connection_destroy(connection);
                    xio_destroy(sasl_io);
                    xio_destroy(tls_io);
                    saslmechanism_destroy(sasl_mechanism_handle);
                }

                free(receive_address);
            }

            free(eh_hostname);
        }
    }

    return result;
}

IOTHUB_TEST_CLIENT_RESULT IoTHubTest_ListenForRecentEvent(IOTHUB_TEST_HANDLE devhubHandle, pfIoTHubMessageCallback msgCallback, size_t partitionCount, void* context, double maxDrainTimeInSeconds)
{
    return IoTHubTest_ListenForEvent(devhubHandle, msgCallback, partitionCount, context, time(NULL), maxDrainTimeInSeconds);
}

IOTHUB_TEST_CLIENT_RESULT IoTHubTest_ListenForEventForMaxDrainTime(IOTHUB_TEST_HANDLE devhubHandle, pfIoTHubMessageCallback msgCallback, size_t partitionCount, void* context)
{
    return IoTHubTest_ListenForRecentEvent(devhubHandle, msgCallback, partitionCount, context, MAX_DRAIN_TIME);
}

static void on_message_send_complete(void* context, MESSAGE_SEND_RESULT send_result, AMQP_VALUE delivery_state)
{
    (void)delivery_state;
    MESSAGE_SEND_STATE* message_send_state = (MESSAGE_SEND_STATE*)context;
    if (send_result == MESSAGE_SEND_OK)
    {
        *message_send_state = MESSAGE_SEND_STATE_SENT_OK;
    }
    else
    {
        *message_send_state = MESSAGE_SEND_STATE_SEND_FAILED;
    }
}

IOTHUB_TEST_CLIENT_RESULT IoTHubTest_SendMessage(IOTHUB_TEST_HANDLE devhubHandle, const unsigned char* data, size_t len)
{
    IOTHUB_TEST_CLIENT_RESULT result;

    if ((devhubHandle == NULL) ||
        ((len == 0) && (data != NULL)) ||
        ((data != NULL) && (len == 0)))
    {
        LogError("Invalid arguments for IoTHubTest_SendMessage, devhubHandle = %p, len = %zu, data = %p.", devhubHandle, len, data);
        result = IOTHUB_TEST_CLIENT_ERROR;
    }
    else
    {
        IOTHUB_VALIDATION_INFO* devhubValInfo = (IOTHUB_VALIDATION_INFO*)devhubHandle;
        char* authcid = CreateSendAuthCid(devhubValInfo);
        if (authcid == NULL)
        {
            LogError("Could not create authcid for SASL plain.");
            result = IOTHUB_TEST_CLIENT_ERROR;
        }
        else
        {
            XIO_HANDLE sasl_io = NULL;
            CONNECTION_HANDLE connection = NULL;
            SESSION_HANDLE session = NULL;
            LINK_HANDLE link = NULL;
            MESSAGE_SENDER_HANDLE message_sender = NULL;
            SASL_MECHANISM_HANDLE sasl_mechanism_handle = NULL;
            XIO_HANDLE tls_io = NULL;

            char* target_address = CreateSendTargetAddress(devhubValInfo);
            if (target_address == NULL)
            {
                LogError("Could not create target_address string.");
                result = IOTHUB_TEST_CLIENT_ERROR;
            }
            else
            {
                size_t deviceDestLen = strlen(AMQP_ADDRESS_PATH_FMT) + strlen(devhubValInfo->deviceId) + 1;
                char* deviceDest = (char*)malloc(deviceDestLen + 1);
                if (deviceDest == NULL)
                {
                    LogError("Could not create device destination string.");
                    result = IOTHUB_TEST_CLIENT_ERROR;
                }
                else
                {
                    SASL_PLAIN_CONFIG sasl_plain_config;
                    sasl_plain_config.authcid = authcid;
                    sasl_plain_config.passwd = devhubValInfo->iotSharedSig;
                    sasl_plain_config.authzid = NULL;
                    const SASL_MECHANISM_INTERFACE_DESCRIPTION* sasl_mechanism_interface_description;

                    (void)sprintf_s(deviceDest, deviceDestLen + 1, AMQP_ADDRESS_PATH_FMT, devhubValInfo->deviceId);

                    if ((sasl_mechanism_interface_description = saslplain_get_interface()) == NULL)
                    {
                        LogError("Could not get SASL plain mechanism interface.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    else if ((sasl_mechanism_handle = saslmechanism_create(sasl_mechanism_interface_description, &sasl_plain_config)) == NULL)
                    {
                        LogError("Could not create SASL plain mechanism.");
                        result = IOTHUB_TEST_CLIENT_ERROR;
                    }
                    else
                    {
                        /* create the TLS IO */
                        TLSIO_CONFIG tls_io_config;
                        tls_io_config.hostname = devhubValInfo->hostName;
                        tls_io_config.port = 5671;
                        tls_io_config.underlying_io_interface = NULL;
                        tls_io_config.underlying_io_parameters = NULL;
                        const IO_INTERFACE_DESCRIPTION* tlsio_interface;

                        if ((tlsio_interface = platform_get_default_tlsio()) == NULL)
                        {
                            LogError("Could not get default TLS IO interface.");
                            result = IOTHUB_TEST_CLIENT_ERROR;
                        }
                        else if ((tls_io = xio_create(tlsio_interface, &tls_io_config)) == NULL)
                        {
                            LogError("Could not create TLS IO.");
                            result = IOTHUB_TEST_CLIENT_ERROR;
                        }
                        else
                        {
                            /* create the SASL client IO using the TLS IO */
                            SASLCLIENTIO_CONFIG sasl_io_config;
                            sasl_io_config.underlying_io = tls_io;
                            sasl_io_config.sasl_mechanism = sasl_mechanism_handle;
                            const IO_INTERFACE_DESCRIPTION* saslclientio_interface;

                            if ((saslclientio_interface = saslclientio_get_interface_description()) == NULL)
                            {
                                LogError("Could not create get SASL IO interface description.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else if ((sasl_io = xio_create(saslclientio_interface, &sasl_io_config)) == NULL)
                            {
                                LogError("Could not create SASL IO.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            /* create the connection, session and link */
                            else if ((connection = connection_create(sasl_io, devhubValInfo->hostName, "some", NULL, NULL)) == NULL)
                            {
                                LogError("Could not create connection.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else if ((session = session_create(connection, NULL, NULL)) == NULL)
                            {
                                LogError("Could not create session.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else if (session_set_incoming_window(session, 2147483647) != 0)
                            {
                                LogError("Could not set incoming window.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else if (session_set_outgoing_window(session, 65536) != 0)
                            {
                                LogError("Could not set outgoing window.");
                                result = IOTHUB_TEST_CLIENT_ERROR;
                            }
                            else
                            {
                                AMQP_VALUE source = NULL;
                                AMQP_VALUE target = NULL;
                                MESSAGE_HANDLE message = NULL;

                                if ((source = messaging_create_source("ingress")) == NULL)
                                {
                                    LogError("Could not create source for link.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else if ((target = messaging_create_target(target_address)) == NULL)
                                {
                                    LogError("Could not create target for link.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else if ((link = link_create(session, "sender-link", role_sender, source, target)) == NULL)
                                {
                                    LogError("Could not create link.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else if (link_set_snd_settle_mode(link, sender_settle_mode_unsettled) != 0)
                                {
                                    LogError("Could not set the sender settle mode.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else if (link_set_max_message_size(link, 65536) != 0)
                                {
                                    LogError("Could not set the message size.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else if ((message = message_create()) == NULL)
                                {
                                    LogError("Could not create a message.");
                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                }
                                else
                                {
                                    BINARY_DATA binary_data;
                                    binary_data.bytes = data;
                                    binary_data.length = len;
                                    if (message_add_body_amqp_data(message, binary_data) != 0)
                                    {
                                        LogError("Could not add the binary data to the message.");
                                        result = IOTHUB_TEST_CLIENT_ERROR;
                                    }
                                    else
                                    {
                                        PROPERTIES_HANDLE properties = properties_create();
                                        AMQP_VALUE to_amqp_value = amqpvalue_create_string(deviceDest);
                                        (void)properties_set_to(properties, to_amqp_value);
                                        amqpvalue_destroy(to_amqp_value);

                                        if (properties == NULL)
                                        {
                                            LogError("Could not create properties for message.");
                                            result = IOTHUB_TEST_CLIENT_ERROR;
                                        }
                                        else
                                        {
                                            if (message_set_properties(message, properties) != 0)
                                            {
                                                LogError("Could not set the properties on the message.");
                                                result = IOTHUB_TEST_CLIENT_ERROR;
                                            }
                                            else if ((message_sender = messagesender_create(link, NULL, NULL)) == NULL)
                                            {
                                                LogError("Could not create message sender.");
                                                result = IOTHUB_TEST_CLIENT_ERROR;
                                            }
                                            else if (messagesender_open(message_sender) != 0)
                                            {
                                                LogError("Could not open the message sender.");
                                                result = IOTHUB_TEST_CLIENT_ERROR;
                                            }
                                            else
                                            {
                                                MESSAGE_SEND_STATE message_send_state = MESSAGE_SEND_STATE_NOT_SENT;

                                                if (messagesender_send_async(message_sender, message, on_message_send_complete, &message_send_state, 0) != 0)
                                                {
                                                    LogError("Could not set outgoing window.");
                                                    result = IOTHUB_TEST_CLIENT_ERROR;
                                                }
                                                else
                                                {
                                                    size_t numberOfAttempts;
                                                    message_send_state = MESSAGE_SEND_STATE_SEND_IN_PROGRESS;

                                                    for (numberOfAttempts = 0; numberOfAttempts < 100; numberOfAttempts++)
                                                    {
                                                        connection_dowork(connection);

                                                        if (message_send_state != MESSAGE_SEND_STATE_SEND_IN_PROGRESS)
                                                        {
                                                            break;
                                                        }

                                                        ThreadAPI_Sleep(50);
                                                    }

                                                    if (message_send_state != MESSAGE_SEND_STATE_SENT_OK)
                                                    {
                                                        LogError("Failed sending (timed out).");
                                                        result = IOTHUB_TEST_CLIENT_ERROR;
                                                    }
                                                    else
                                                    {
                                                        result = IOTHUB_TEST_CLIENT_OK;
                                                    }
                                                }
                                            }

                                            properties_destroy(properties);
                                        }
                                    }
                                }

                                message_destroy(message);
                                amqpvalue_destroy(source);
                                amqpvalue_destroy(target);
                            }
                        }
                    }

                    free(deviceDest);
                }

                free(target_address);
            }

            free(authcid);

            messagesender_destroy(message_sender);
            link_destroy(link);
            session_destroy(session);
            connection_destroy(connection);
            xio_destroy(sasl_io);
            xio_destroy(tls_io);
            saslmechanism_destroy(sasl_mechanism_handle);
        }
    }

    return result;
}
