// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/platform.h"


#include <signal.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#include "parson.h"

#include "iothub_message_monitoring_ll.h"
#include "iothub_message_monitoring.h"

typedef struct IOTHUB_MESSAGE_MONITORING_TAG
{
    IOTHUB_MESSAGE_MONITORING_LL_HANDLE iotHubMsgMonLLHandle;
    THREAD_HANDLE threadHandle;
    LOCK_HANDLE lockHandle;
    bool stopThread;
} IOTHUB_MESSAGE_MONITORING;

static int ScheduleWork_Thread(void* threadArgument)
{
    IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)threadArgument;

    while (1)
    {
        if (Lock(msgMon->lockHandle) == LOCK_OK)
        {
            if (msgMon->stopThread)
            {
                (void)Unlock(msgMon->lockHandle);
                break; /*gets out of the thread*/
            }
            else
            {
                IoTHubMessageMonitoring_LL_DoWork(msgMon->iotHubMsgMonLLHandle);
                (void)Unlock(msgMon->lockHandle);
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

static int StartWorkerThreadIfNeeded(IOTHUB_MESSAGE_MONITORING* msgMon)
{
    int result;

    if (msgMon->threadHandle == NULL)
    {
        msgMon->stopThread = false;

        if (ThreadAPI_Create(&msgMon->threadHandle, ScheduleWork_Thread, msgMon) != THREADAPI_OK)
        {
            LogError("ThreadAPI_Create failed");

            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_Open(IOTHUB_MESSAGE_MONITORING_HANDLE messageMonitoringHandle, const char* consumerGroup, size_t partitionNumber, IOTHUB_MESSAGE_FILTER* filter, IOTHUB_MESSAGE_MONITORING_STATE_CHANGED_CALLBACK stateChangedCallback, void* userContext)
{
    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=NULL)");
        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)messageMonitoringHandle;

        if (Lock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Failed to lock");
            result = IOTHUB_MESSAGE_MONITORING_ERROR;
        }
        else
        {
            result = IoTHubMessageMonitoring_LL_Open(messageMonitoringHandle->iotHubMsgMonLLHandle, consumerGroup, partitionNumber, filter, stateChangedCallback, userContext);

            if (result != IOTHUB_MESSAGE_MONITORING_OK)
            {
                LogError("Failed opening low level message monitoring");
            }
            else if (StartWorkerThreadIfNeeded(msgMon) != 0)
            {
                LogError("Failed starting monitoring thread");
                result = IOTHUB_MESSAGE_MONITORING_ERROR;
            }

            if (Unlock(msgMon->lockHandle) != LOCK_OK)
            {
                LogError("Failed to unlock");
                result = IOTHUB_MESSAGE_MONITORING_ERROR;
            }
        }
    }

    return result;
}

void IoTHubMessageMonitoring_Close(IOTHUB_MESSAGE_MONITORING_HANDLE messageMonitoringHandle)
{
    if (messageMonitoringHandle == NULL)
    {
        LogError("NULL messageMonitoringHandle");
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)messageMonitoringHandle;

        if (Lock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Could not acquire lock");

            msgMon->stopThread = true; /*setting it even when Lock fails*/
        }
        else
        {
            msgMon->stopThread = true;

            if (Unlock(msgMon->lockHandle) != LOCK_OK)
            {
                LogError("Failed to unlock");
            }
        }

        if (msgMon->threadHandle != NULL)
        {
            int res;
            if (ThreadAPI_Join(msgMon->threadHandle, &res) != THREADAPI_OK)
            {
                LogError("ThreadAPI_Join failed");
            }

            msgMon->threadHandle = NULL;
        }

        IoTHubMessageMonitoring_LL_Close(messageMonitoringHandle->iotHubMsgMonLLHandle);
    }
}

void IoTHubMessageMonitoring_Destroy(IOTHUB_MESSAGE_MONITORING_HANDLE messageMonitoringHandle)
{
    if (messageMonitoringHandle == NULL)
    {
        LogError("messageMonitoringHandle input parameter is NULL");
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)messageMonitoringHandle;

        if (msgMon->threadHandle != NULL)
        {
            IoTHubMessageMonitoring_Close(messageMonitoringHandle);
        }

        if (msgMon->lockHandle != NULL && Lock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Failed to lock");
        }

        if (msgMon->iotHubMsgMonLLHandle != NULL)
        {
            IoTHubMessageMonitoring_LL_Destroy(msgMon->iotHubMsgMonLLHandle);
        }

        if (msgMon->lockHandle != NULL && Unlock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Failed to unlock");
        }

        if (msgMon->lockHandle != NULL)
        {
            Lock_Deinit(msgMon->lockHandle);
        }

        free(msgMon);
    }
}

IOTHUB_MESSAGE_MONITORING_HANDLE IoTHubMessageMonitoring_Create(IOTHUB_SERVICE_CLIENT_AUTH_HANDLE serviceClientHandle)
{
    IOTHUB_MESSAGE_MONITORING* result;

    if (serviceClientHandle == NULL)
    {
        LogError("Invalid argument (serviceClientHandle=NULL)");
        result = NULL;
    }
    else
    {
        if ((result = (IOTHUB_MESSAGE_MONITORING*)malloc(sizeof(IOTHUB_MESSAGE_MONITORING))) == NULL)
        {
            LogError("Failed allocating IOTHUB_MESSAGE_MONITORING struct");
        }
        else
        {
            memset(result, 0, sizeof(IOTHUB_MESSAGE_MONITORING));

            if ((result->lockHandle = Lock_Init()) == NULL)
            {
                LogError("Lock_Init failed");
                IoTHubMessageMonitoring_Destroy(result);
                result = NULL;
            }
            else if ((result->iotHubMsgMonLLHandle = IoTHubMessageMonitoring_LL_Create(serviceClientHandle)) == NULL)
            {
                LogError("IoTHubMessageMonitoring_LL_Create failed");
                IoTHubMessageMonitoring_Destroy(result);
                result = NULL;
            }
        }
    }

    return (IOTHUB_MESSAGE_MONITORING_HANDLE)result;
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_SetMessageCallback(IOTHUB_MESSAGE_MONITORING_HANDLE messageMonitoringHandle, IOTHUB_MESSAGE_RECEIVED_CALLBACK messageReceivedCallback, void* userContextCallback)
{
    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=NULL)");
        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)messageMonitoringHandle;

        if (Lock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Failed to lock");
            result = IOTHUB_MESSAGE_MONITORING_ERROR;
        }
        else
        {
            result = IoTHubMessageMonitoring_LL_SetMessageCallback(messageMonitoringHandle->iotHubMsgMonLLHandle, messageReceivedCallback, userContextCallback);

            if (Unlock(msgMon->lockHandle) != LOCK_OK)
            {
                LogError("Failed to unlock");
                result = IOTHUB_MESSAGE_MONITORING_ERROR;
            }
        }
    }

    return result;
}

IOTHUB_MESSAGE_MONITORING_RESULT IoTHubMessageMonitoring_SetOption(IOTHUB_MESSAGE_MONITORING_HANDLE messageMonitoringHandle, const char* name, const void* value)
{
    IOTHUB_MESSAGE_MONITORING_RESULT result;

    if (messageMonitoringHandle == NULL)
    {
        LogError("Invalid argument (messageMonitoringHandle=NULL)");

        result = IOTHUB_MESSAGE_MONITORING_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_MONITORING* msgMon = (IOTHUB_MESSAGE_MONITORING*)messageMonitoringHandle;

        if (Lock(msgMon->lockHandle) != LOCK_OK)
        {
            LogError("Failed to lock");
            result = IOTHUB_MESSAGE_MONITORING_ERROR;
        }
        else
        {
            result = IoTHubMessageMonitoring_LL_SetOption(msgMon->iotHubMsgMonLLHandle, name, value);

            if (Unlock(msgMon->lockHandle) != LOCK_OK)
            {
                LogError("Failed to unlock");
                result = IOTHUB_MESSAGE_MONITORING_ERROR;
            }
        }
    }

    return result;
}