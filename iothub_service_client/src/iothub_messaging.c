// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/sastoken.h"

#include "azure_uamqp_c/connection.h"
#include "azure_uamqp_c/message_receiver.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/sasl_mechanism.h"
#include "azure_uamqp_c/saslclientio.h"
#include "azure_uamqp_c/sasl_plain.h"
#include "azure_uamqp_c/cbs.h"
#include <signal.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#include "parson.h"

#include "iothub_messaging_ll.h"
#include "iothub_messaging.h"

typedef struct IOTHUB_MESSAGING_CLIENT_INSTANCE_TAG
{
    IOTHUB_MESSAGING_HANDLE IoTHubMessagingHandle;
    THREAD_HANDLE ThreadHandle;
    LOCK_HANDLE LockHandle;
    sig_atomic_t StopThread;
} IOTHUB_MESSAGING_CLIENT_INSTANCE;

static int ScheduleWork_Thread(void* threadArgument)
{
    IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)threadArgument;

    while (1)
    {
        if (Lock(iotHubMessagingClientInstance->LockHandle) == LOCK_OK)
        {
            if (iotHubMessagingClientInstance->StopThread)
            {
                (void)Unlock(iotHubMessagingClientInstance->LockHandle);
                break; /*gets out of the thread*/
            }
            else
            {
                IoTHubMessaging_LL_DoWork(iotHubMessagingClientInstance->IoTHubMessagingHandle);
                (void)Unlock(iotHubMessagingClientInstance->LockHandle);
            }
        }
        else
        {
            LogError("Lock failed, shall retry");
        }
        (void)ThreadAPI_Sleep(1);
    }

    ThreadAPI_Exit(0);
    return 0;
}

static IOTHUB_MESSAGING_RESULT StartWorkerThreadIfNeeded(IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance)
{
    IOTHUB_MESSAGING_RESULT result;
    if (iotHubMessagingClientInstance->ThreadHandle == NULL)
    {
        iotHubMessagingClientInstance->StopThread = 0;
        if (ThreadAPI_Create(&iotHubMessagingClientInstance->ThreadHandle, ScheduleWork_Thread, iotHubMessagingClientInstance) != THREADAPI_OK)
        {
            LogError("ThreadAPI_Create failed");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGING_OK;
        }
    }
    else
    {
        result = IOTHUB_MESSAGING_OK;
    }
    return result;
}

IOTHUB_MESSAGING_CLIENT_HANDLE IoTHubMessaging_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    IOTHUB_MESSAGING_CLIENT_INSTANCE* result;

    if (serviceClientHandle == NULL)
    {
        LogError("serviceClientHandle input parameter cannot be NULL");
        result = NULL;
    }
    else
    {
        if ((result = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)malloc(sizeof(IOTHUB_MESSAGING_CLIENT_INSTANCE))) == NULL)
        {
            LogError("malloc failed for IoTHubMessagingClient");
            result = NULL;
        }
        else
        {
            result->LockHandle = Lock_Init();
            if (result->LockHandle == NULL)
            {
                LogError("Lock_Init failed");
                free(result);
                result = NULL;
            }
            else
            {
                result->IoTHubMessagingHandle = IoTHubMessaging_LL_Create(serviceClientHandle);
                if (result->IoTHubMessagingHandle == NULL)
                {
                    LogError("IoTHubMessaging_LL_Create failed");
                    Lock_Deinit(result->LockHandle);
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->StopThread = 0;
                    result->ThreadHandle = NULL;
                }
            }
        }
    }
    return (IOTHUB_MESSAGING_CLIENT_HANDLE)result;
}

void IoTHubMessaging_Destroy(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle)
{
    if (messagingClientHandle == NULL)
    {
        LogError("messagingClientHandle input parameter is NULL");
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* messagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        IoTHubMessaging_LL_Destroy(messagingClientInstance->IoTHubMessagingHandle);

        Lock_Deinit(messagingClientInstance->LockHandle);

        free(messagingClientInstance);
    }
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_Open(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle, IOTHUB_OPEN_COMPLETE_CALLBACK openCompleteCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    if (messagingClientHandle == NULL)
    {
        LogError("NULL messagingClientHandle");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        if (Lock(iotHubMessagingClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            result = IoTHubMessaging_LL_Open(messagingClientHandle->IoTHubMessagingHandle, openCompleteCallback, userContextCallback);

            (void)Unlock(iotHubMessagingClientInstance->LockHandle);
        }
    }

    return result;
}

void IoTHubMessaging_Close(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle)
{
    if (messagingClientHandle == NULL)
    {
        LogError("NULL messagingClientHandle");
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        if (Lock(iotHubMessagingClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");
            iotHubMessagingClientInstance->StopThread = 1; /*setting it even when Lock fails*/
        }
        else
        {
            iotHubMessagingClientInstance->StopThread = 1;

            (void)Unlock(iotHubMessagingClientInstance->LockHandle);
        }

        if (iotHubMessagingClientInstance->ThreadHandle != NULL)
        {
            int res;
            if (ThreadAPI_Join(iotHubMessagingClientInstance->ThreadHandle, &res) != THREADAPI_OK)
            {
                LogError("ThreadAPI_Join failed");
            }
        }

        IoTHubMessaging_LL_Close(messagingClientHandle->IoTHubMessagingHandle);
    }
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_SetFeedbackMessageCallback(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle, IOTHUB_FEEDBACK_MESSAGE_RECEIVED_CALLBACK feedbackMessageReceivedCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    if (messagingClientHandle == NULL)
    {
        LogError("NULL messagingClientHandle");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        if (Lock(iotHubMessagingClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            result = IoTHubMessaging_LL_SetFeedbackMessageCallback(messagingClientHandle->IoTHubMessagingHandle, feedbackMessageReceivedCallback, userContextCallback);

            (void)Unlock(iotHubMessagingClientInstance->LockHandle);
        }
    }

    return result;
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_SendAsync(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle, const char* deviceId, IOTHUB_MESSAGE_HANDLE message, IOTHUB_SEND_COMPLETE_CALLBACK sendCompleteCallback, void* userContextCallback)
{
    IOTHUB_MESSAGING_RESULT result;

    if (messagingClientHandle == NULL)
    {
        LogError("NULL iothubClientHandle");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        if (Lock(iotHubMessagingClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");
            result = IOTHUB_MESSAGING_INVALID_ARG;
        }
        else
        {
            if ((result = StartWorkerThreadIfNeeded(iotHubMessagingClientInstance)) != IOTHUB_MESSAGING_OK)
            {
                LogError("Could not start worker thread");
                result = IOTHUB_MESSAGING_ERROR;
            }
            else
            {
                result = IoTHubMessaging_LL_Send(iotHubMessagingClientInstance->IoTHubMessagingHandle, deviceId, message, sendCompleteCallback, userContextCallback);
            }

            (void)Unlock(iotHubMessagingClientInstance->LockHandle);
        }
    }

    return result;
}

IOTHUB_MESSAGING_RESULT IoTHubMessaging_SetTrustedCert(IOTHUB_MESSAGING_CLIENT_HANDLE messagingClientHandle, const char* trusted_cert)
{
    IOTHUB_MESSAGING_RESULT result;

    if (messagingClientHandle == NULL)
    {
        LogError("NULL iothubClientHandle");
        result = IOTHUB_MESSAGING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGING_CLIENT_INSTANCE* iotHubMessagingClientInstance = (IOTHUB_MESSAGING_CLIENT_INSTANCE*)messagingClientHandle;

        if (Lock(iotHubMessagingClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");
            result = IOTHUB_MESSAGING_ERROR;
        }
        else
        {
            result = IoTHubMessaging_LL_SetTrustedCert(iotHubMessagingClientInstance->IoTHubMessagingHandle, trusted_cert);
            (void)Unlock(iotHubMessagingClientInstance->LockHandle);
        }
    }
    return result;
}
