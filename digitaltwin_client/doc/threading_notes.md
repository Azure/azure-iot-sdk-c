# Threading model and optimizing for resource constrained devices

## Digital Twin C SDK and the \_LL\_ (lower layer) API

### The lower layer
There are two distinct but closely related sets of API's for Digital Twin.
Applications that want to run on a single thread should use the \_LL\_ (aka lower level) API.  Sometimes the hardware/OS can only support one thread at a time or else spinning additional threads is expensive.  Applications on more capable hardware may also use the \_LL\_ layer for greater control over scheduling.

Calls to the Digital Twin API's ending in Async do not cause the Digital Twin operation to execute immediately.  Instead, the work - both for sending data across the network and receiving data - is queued.  The application must manually schedule the work by calling `DigitalTwin_DeviceClient_LL_DoWork`.  

This is typically done in some sort of simple loop, e.g.

```c
    while (appIsRunning) {
        SendTelemetryIfThereIsAnyToSend(digitalTwinLLHandle);
        DigitalTwin_DeviceClient_LL_DoWork(digitalTwinLLHandle);
        Sleep(1000);
    }
```

### The convenience layer

Applications may also use the non \_LL\_ API set, which is also known as the "convenience layer."  

Just like the \_LL\_ layer, Digital Twin API's ending in Async queue work to be performed later and do not block on the service accepting or rejecting the request.  The difference is that the convenience layer itself automatically takes care of sending the data.  In other words, there is no `DoWork`.  The convenience layer does this for you automatically.  It also performs locking, allowing a given `DIGITALTWIN_DEVICE_CLIENT_HANDLE` to be safely used by  different threads.

## How to specify between the \_LL\_ and convenience layers

* Applications using the \_LL\_ layer should #include [digitaltwin_device_client_ll.h](../inc/digitaltwin_device_client_ll.h) and use its API's and `DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE`.
* Applications using the convenience layer should #include [digitaltwin_device_client.h](../inc/digitaltwin_device_client.h) and use its API's and `DIGITALTWIN_DEVICE_CLIENT_HANDLE`.

## Avoid long-running callbacks

Regardless of whether you are using the \_LL\_ or convenience layer, application callbacks that you implement - such as processing a property update - must NOT block for extended periods of time.  

The Digital Twin SDK uses a single dispatcher thread to handle all callbacks to user code.  This same thread handles all network I/O.  This is true whether the \_LL\_ or convenience layer is used.  The only difference is that in the \_LL\_ layer, your thread is the dispatcher.

An application callback that Digital Twin calls that takes a long time to run is problematic.  The Digital Twin Device SDK will not be able to call other pending callbacks as it is blocked on the long-running one.  Worse, if the call back code takes long enough (minutes) there is the risk that the Digital Twin SDK for an \_LL\_ handle will not be able to fire its periodic network keep-alive and that the entire connection will be dropped.


## CRITICAL(!) notes for \_LL\_ application development

As the name implies, the convenience layer makes programming a Digital Twin more convenient (albeit it at a resource cost).  When using the \_LL\_ layer, special care must be taken.  

**NOT FOLLOWING THESE STEPS WILL POTENTIALLY RESULT IN VERY HARD TO DIAGNOSE BUGS**

* You **MUST NOT** use a `DIGITALTWIN_DEVICE_CLIENT_LL_HANDLE`  on more than one thread (unless you provide *all* your own locking).  Part of what makes the \_LL\_ layer use fewer resources is that it does not use locks.  It assumes a single threaded environment where locks are not needed.
* Even if you are not actively sending data while using \_LL\_ layer, you still need to periodically invoke `DigitalTwin_DeviceClient_LL_DoWork` to make the client accept data from the network.  This periodic DoWork call is also required as otherwise the underlying network transport and/or service may timeout the service due to inactivity.
