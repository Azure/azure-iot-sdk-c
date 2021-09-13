# Threading model and optimizing for resource constrained devices

## IoTHub Device Client and the \_LL\_ (lower layer) API

### The lower layer

There are two distinct but closely related sets of APIs for the IoTHub client.
Applications that want to run on a single thread should use the \_LL\_ (aka lower level) API.  Sometimes the hardware/OS can only support one thread at a time or else spinning additional threads is expensive.  Applications on more capable hardware may also use the \_LL\_ layer for greater control over scheduling.

Calls to the IoThub API's ending in Async do not cause the operation to execute immediately.  Instead, the work - both for sending data across the network and receiving data - is queued.  The application must manually schedule the work by calling `IoTHubDeviceClient_LL_DoWork` (when using a device client).

This is typically done in some sort of simple loop, e.g.

```c
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubLLHandle;

    while (appIsRunning) {
        SendTelemetryIfThereIsAnyToSend(iotHubLLHandle);
        IoTHubDeviceClient_LL_DoWork(iotHubLLHandle);
        Sleep(1000);
    }
```

**CRITICAL(!) notes for \_LL\_ application development**  
*NOT FOLLOWING THESE STEPS WILL POTENTIALLY RESULT IN VERY HARD TO DIAGNOSE BUGS*

* You **MUST NOT** use an \_LL\_ handle on more than one thread (unless you provide *all* your own locking).  Part of what makes the \_LL\_ layer use fewer resources is that it does not use locks.  It assumes a single threaded environment where locks are not needed.
* Even if you are not actively sending data while using \_LL\_ layer, you still need to periodically invoke the appropriate `DoWork` to make the client accept data from the network.  This periodic `DoWork` call is also required as otherwise the underlying network transport and/or service may timeout the service due to inactivity.

### The convenience layer

Applications may also use the non \_LL\_ API set, which is also known as the "convenience layer."  

Just like the \_LL\_ layer, IoTHub API's ending in Async queue work to be performed later and do not block waiting for the service accepting or rejecting the request.  The difference is that the convenience layer itself automatically takes care of sending the data.  In other words, there is no `DoWork`.  The convenience layer does this for you automatically by spinning a worker thread to implicitly `DoWork` for your application.  The convenience layer also performs locking, allowing a given `IOTHUB_DEVICE_CLIENT_HANDLE` to be safely used by  different threads.

## How to specify between the \_LL\_ and convenience layers

* Applications using the \_LL\_ layer should `#include iothub_device_client_ll.h` and use its API's and `IOTHUB_DEVICE_CLIENT_LL_HANDLE`.
* Applications using the convenience layer should `#include iothub_device_client.h` and use its API's and `IOTHUB_DEVICE_CLIENT_HANDLE`.

The \_LL\_ | convenience layer paradigm is used in many places throughout the SDK.  The module client follows this pattern with `IOTHUB_MODULE_CLIENT_LL_HANDLE` and `IOTHUB_MODULE_CLIENT_HANDLE`.  The provisioning client also follows this pattern with `PROV_DEVICE_LL_HANDLE` and `PROV_DEVICE_HANDLE`.

## Avoid long-running callbacks

Regardless of whether you are using the \_LL\_ or convenience layer, application callbacks that you implement - such as processing a twin update - must NOT block for extended periods of time.  

The IoTHub SDK uses a single dispatcher thread to handle all callbacks to user code.  This same thread handles all network I/O.  This is true whether the \_LL\_ or convenience layer is used.  The only difference is that in the \_LL\_ layer, your thread is the dispatcher when it calls into the appropriate DoWork() call.

An application callback that takes a long time to run is problematic.  The SDK will not be able to call other pending callbacks as it is blocked on the long-running one.  If the call back code takes long enough (minutes) there is the risk that the SDK will not be able to fire its periodic network keep-alive and that the entire connection will be dropped.
