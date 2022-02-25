# Azure IoT Device Client C SDK

## Connection and Messaging Reliability

### Overview

In this document you will find information about the reliability aspects of the [Azure IoT C SDK](https://github.com/Azure/azure-iot-sdk-c) design, including details of:

- Connection authentication/refresh;

- How connection failures are detected;

- The reconnection logic;

- Available options for fine-tuning reconnection (retry policies, timeouts);

- Notifications of IoT Hub connection status (status callbacks);

- What happens to previously queued and new messages;

- Specific behaviors of the supported transport-protocols (AMQP, MQTT, HTTP).

### Connection Authentication

This is a brief note to clarify how authentication is done in the IoTHub Device/Module clients.

Authentication of a client in the SDK can be done using either

- [SAS tokens](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-security#security-tokens), or 

- [x509 certificates](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-security#supported-x509-certificates), or

- [Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/).

This section does not describe the details of Device Provisioning Service (DPS), please use the link above for details.

When using SAS tokens, authentication can be done by:

- Providing [your own SAS token](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-security#authentication), or

- Giving the device keys to the SDK (using the device connection string - see ["Alternate Device Credentials"](https://docs.microsoft.com/azure/iot-hub/iot-hub-device-sdk-c-iothubclient#alternate-device-credentials) in our documentation) and letting it create the SAS tokens for you (this is the most usual authentication method).

As mentioned in the articles above, SAS tokens have an expiration time.

The Azure IoT SDK connection then generates and sends new SAS tokens periodically to the IoT Hub to keep the connection authenticated.

The internal behaviour is different depending on the transport protocol used:

|**Transport**|**Behaviour**|
|-|-|
|MQTT|SAS tokens are valid for 1 hour, and a new one is sent every 48 minutes. Every time a new SAS token needs to be sent, the client will disconnect from the Azure IoT Hub and reconnect.|
|AMQP|SAS tokens are valid for 1 hour, and a new one is sent every 48 minutes. Client is not disconnected when a new SAS token is sent.|
|HTTP|There is no persistent connection; a new SAS token (valid for 1 hour) is created and sent with each request to the Azure IoT Hub.|


Both the SAS token lifetime and refresh rate are configurable on AMQP transport (see [AMQP transport section](https://github.com/Azure/azure-iot-sdk-c/blob/2019-03-18/doc/Iothub_sdk_options.md#amqp-transport) in SDK options documentation).


### Connection Establishment and Retry Logic

The design of the Azure IoT C SDK is composed of layers, each of them assigned specific responsibilities:

| **Layer**                 | **C module**                                                                                                        | **Purpose**                                                                                                                                                                                                                                                                                              |
|---------------------------|---------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Azure IoT C SDK           | [iothub\_client](https://github.com/Azure/azure-iot-sdk-c/blob/main/iothub_client/inc/iothub_client.h)            | Multi-threaded layer over iothub\_ll\_client (automatically performs invocation of the IoTHubClient\_LL\_DoWork function and internal callback handling)                                                                                                                                                 |
| Azure IoT C Low Level SDK | [iothub\_client\_ll](https://github.com/Azure/azure-iot-sdk-c/blob/main/iothub_client/inc/iothub_client_ll.h)     | Main surface of the Azure IoT device client API (single-threaded)                                                                                                                                                                                                                                        |
| Protocol Transport        | [iothubtransport\*](https://github.com/Azure/azure-iot-sdk-c/tree/main/iothub_client/inc)                         | Provides an interface between the specific protocol API (e.g., [uamqp](https://github.com/Azure/azure-uamqp-c), [umqtt](https://github.com/Azure/azure-umqtt-c)) and the upper client SDK. It is responsible for part of the business logic, the message queuing and timeout control, options handling. |
| Protocol API              | [uamqp](https://github.com/Azure/azure-uamqp-c), [umqtt](https://github.com/Azure/azure-umqtt-c) or native HTTP API | Implements the specific application protocol (either AMQP, MQTT or HTTP, respectivelly)                                                                                                                                                                                                                  |
| TLS                       | [tlsio\_\*](https://github.com/Azure/azure-c-shared-utility/tree/master/adapters)                                   | Provides a wrapper over the specific TLS API (Schannel, openssl, wolfssl, mbedtls), using the [xio](https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/xio.h) interface that the device client SDK uses                                                              |
| Socket                    | [socketio\_\*](https://github.com/Azure/azure-c-shared-utility/tree/master/adapters)                                | Provides a wrapper over the specific socket API ([win32, berkeley, mbed](https://github.com/Azure/azure-c-shared-utility/tree/master/adapters)), using the [xio](https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/xio.h) interface that the device client SDK uses |

When an Azure IoT device client instance is created, this is the typical\* sequence within the SDK:

1. User creates a new device client instance using, e.g., IoTHubClient\_CreateFromConnectionString;

2. The device client instance creates a transport protocol instance based on the selection from the user (AMQP\_Protocol, [MQTT\_Protocol](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothubtransportmqtt.h#L13), [AMQP\_Protocol\_over\_WebSocketsTls](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothubtransportamqp_websockets.h#L15), [MQTT\_WebSocket\_Protocol](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothubtransportmqtt_websockets.h#L15) or [HTTP\_Protocol](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothubtransporthttp.h#L14));

3. The transport protocol then creates:

    - A tlsio\_\* instance based on the system where it is running (by default Schannel on Windows and openssl on Linux), and

    - An instance of the application protocol specific API (uamqp, [umqtt](https://github.com/Azure/azure-umqtt-c), httpapi), passing as argument the tlsio\_\* instance created above.

4. The protocol API instance then uses the tlsio\_\* to establish the secure connection with the Azure IoT Hub

    - The socket creation and connection happens within the tlsio\_\* layer.

    \* The sequence shown is different if using specific features like device multiplexing.

Each of these layers provide status and error events to the above through function returns and callbacks. Connection issues are detected in three different ways accross the SDK:

1. Through persistent device-to-cloud message I/O failures;

> Which can be, for example:

    - uAMQP or uMQTT reporting failures when sending:

    - Telemetry messages;

    - Device Twin reported property updates;

    - Keep-alive messages they frequently send to the Azure IoT Hub;

- Transport protocol detecting timeouts waiting for:

  - Telemetry messages to complete sending;

  - [CBS authentication token](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-security#security-tokens) refresh to be completed.

1. Through failures reported by the socket APIs;

    - On send() and/or receive() failures;

    - On signals raised by the socket API (different operating systems report failures faster than others, resulting in corresponding speed of detection by the device client SDK).

2. Through graceful disconnection notifications from the Azure IoT Hub.

> Which can happen when a hub is preparing for a system update, for example (upon reconnection the device client automatically gets routed to the next available Hub).

Some aspects of the detection of connection issues are specific to the transport protocol used, as shown in the table:

| **Protocol** | **Connection Issue Detection**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AMQP         | Besides regular detection through callbacks from uAMQP, the AMQP protocol transport will mark a connection to the Azure IoT hub as faulty if [5 (five)](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/src/iothubtransport_amqp_common.c#L47) or more consecutive failures occur on any of these: **A)** Attempting to subscribe for Commands, Device Methods or Twin Desired Properties, **B)** sending Telemetry messages (either by timeouts or active failures returned by uAMQP api), **C)** responding to Device Method invokations, **D)** refreshing CBS authentication tokens. |
| MQTT         | Besides regular detection through callbacks from uMQTT, the MQTT protocol transport will attempt to publish messages up to [two times](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/src/iothubtransport_mqtt_common.c#L46) (waiting [60 seconds](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/src/iothubtransport_mqtt_common.c#L45) between attempts) before raising a failure.                                                                                                                                                                            |
| HTTP         | HTTP connections to the Azure IoT Hub are not persistent. Each outgoing message to the hub results in a new connection and is closed as soon as the I/O is completed. Incoming messages from the Hub are received by the device client through polling mechanisms, where the the HTTP connection follows the same lifecycle above. If connection failures occur, the protocol transport simply keeps retrying the operation until it succeeds.                                                                                                                                                                  |

### The Connection Retry Logic

Once a connection issue is detected, the transport protocol will initiate its connection retry logic. The process is as follows:

1. Connection is marked as faulty by the protocol transport (through the ways described above);

2. Transport prepares for re-connection;

> a. Pending outgoing messages are properly handled;

- No messages are ever lost; they are either:

  - Re-sent after a new connection is successfully reestablished, or

  - Returned to the user as failed through the completion callbacks;

> b. Connection components are destroyed;

- That includes destroying the uAMQP or uMQTT components (sessions, links, connection for AMQP; topic subscriptions for MQTT) and tlsio\_\* instance (and its internal socket connection)

> c. Connection Status Callback is invoked (if subscribed);

- More details are available on the ["Connection Status Callback"](file:///C:\repos\s1\azure-iot-sdk-c\doc\connection-and-messaging-reliability.md#Connection-Status-Callback) sub-section below.

1. Retry Policy is checked to determine if a reconnection attempt shall be attempted;

> More details are explained in the ["Connection Retry Policies"](file:///C:\repos\s1\azure-iot-sdk-c\doc\connection-and-messaging-reliability.md#Connection-Retry-Policies) sub-section below.

1. If the Retry Policy allows, a re-connection is attempted;

> If the Retry Policy requires to wait before an attempt can be made, the protocol transport delays the re-connection, starting again from **step 3** afterwards.

1. If the re-connection attempt fails, start again from **step 2.**

2. Upon successful re-connection, all these additional steps are taken by the SDK:

    - Previous subscriptions made by the user for cloud-to-device messages (Commands, Device Methods, Device Twin Desired Properties) are automatically reestablished, requiring no additional action (the SDK continues to use the same callback functions previously set);

    - Pending outgoing messages start to be sent again to the Azure IoT Hub;

    - Connection Status Callback is invoked (if subscribed).

### Behavior of the Device Client SDK while Re-Connecting

The Azure IoT Device Client C SDK implements asynchronous operations through the \*\_DoWork() model it uses.

All API functions that result in I/O (like [*IoTHubClient\_LL\_SendEventAsync*](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L354) or [*IoTHubClient\_LL\_SetMessageCallback*](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L385)) are actually queued or stored, taking effect only when [*IoTHubClient\_LL\_DoWork*](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L461) is invoked\* AND the connection with the Azure IoT Hub is established.

\* IoTHubClient\_LL\_DoWork is invoked internally automatically if using the iothub\_client multi-threaded API.

For example, while the device client is in re-connection mode,

- New Telemetry messages passed to the SDK will be queued (but still subject to the message-send timeout even during reconnection);

- New subscriptions for cloud-to-device messages (e.g., Commands) will be stored, but result in an actual request for subscription to the Azure IoT Hub only the device client re-connects.

For clarity it is worth mentioning that while the Azure IoT Device Client is re-connecting, it has no means to receive any messages from the Azure IoT Hub. During that time any attempts to send Commands or invoke Device Methods to the given device client (using for example the [Azure IoT Service Client C SDK](https://github.com/Azure/azure-iot-sdk-c/tree/2018-05-04/iothub_service_client)) will result in failure returned by the Azure IoT Hub to the source of those requests.

### Timeout Controls over Outgoing Messages

Besides checking for error returns and responding to callbacks from lower its layers, the Azure IoT Device Client C SDK also implements extra logic to detect failures by tracking timeouts. They apply to different functionalities within the SDK, each with a specific course of action in case

- Connections complete in time (including TLS negotiation);

- Messages (e.g., Telemetry, Twin reported properties) are sent within the expected time range;

- Authentication refreshes are done with the expected frequency.

Some of the time-outs can be fine-tuned by the user (through IoTHubClient\_LL\_SetOption). Some are dependent on the application protocol selected (AMQP, MQTT or HTTP).

Details about the specific timeout control values that can be set by the user can be found on the ["Current Configuration Options"](file:///C:\repos\s1\azure-iot-sdk-c\doc\connection-and-messaging-reliability.md#Current-Configuration-Options) sub-section bellow.

### Known Issue: Duplicated Timeout Control for Sending Telemetry Messages

Currently the Azure IoT Device Client C SDK implements a queue for pending outgoing Telemetry messages that is owned by the iothub\_client\_ll layer.

Any new Telemetry messages passed to the SDK by the user (through ) are copied and immediatelly stored in that queue.

That same queue is shared with the protocol transport, which treats it as a "waiting to send" list. The protocol transport also has its own queue for Telemetry messages, but this one is only for messages that are already being sent (i.e., have been processed and passed down to the protocol API module).

The protocol transport at some point (namely, right when IoTHubClient\_LL\_DoWork is invoked) removes messages in the "waiting to send" list (in the order they were added), converts them to the format understood by the protocol API layer, then adds the pair to its "in progress" list.

Once the lower layer calls back with the send completion notification the protocol transport removes the specific message from its "in progress" list and bubbles it (along with the send result) to the user (through the callback provided).

There are two timeout controls in this system. An original one in the iothub\_client\_ll layer - which controls the "waiting to send" queue - and a modern one in the protocol transport layer - that applies to the "in progress" list. However, since IoTHubClient\_LL\_DoWork causes the Telemetry messages to be immediately\* processed, sent and moved to the "in progress" list, the first timeout control is virtually non-applicable.

Both can be fine-tuned by users through IoTHubClient\_LL\_SetOption, and because of that removing the original control could cause a break for existing customers. For that reason it has been kept as is, but it will be re-designed when we move to the next major version of the product.

For now, new customers should set both options if using the iothub\_client\_ll module, or just the protocol transport one if using the iothub\_client module.

\* Depends on how frequently IoTHubClient\_LL\_DoWork is invoked. On the iothub\_client (threaded) layer, it is invoked every one-hundred milliseconds.

### Connection Retry Policies

Some times the connection issues, although transient, can last longer than expected, or network availability can bounce on and off for a while after it first returns.

Attempting to reconnect to the Azure IoT Hub immediatelly in a loop is not the most efficient way for the device client SDK to operate, since some of the initial attempts might fail because of the reasons above.

As shown above on ["The Connection Retry Logic"](file:///C:\repos\s1\azure-iot-sdk-c\doc\connection-and-messaging-reliability.md#The-Connection-Retry-Logic) (step 3), the protocol transport checks if the current retry policy allows it to attempt to re-connect to the Azure IoT Hub.

The Retry Policy feature exposes a way to control how immediatelly and frequently the Azure IoT Device Client C SDK will attempt to re-connect to the Azure IoT Hub in case a connection issue occurs.

The logic is as follows:

- Within the transport protocol there is the Retry Control, a the dedicated module for handling Retry Policy matters;

- The transport control checks if the Retry Control allows it to reconnect at a given moment;

- The Retry Control will calculate the current wait time based on the Retry Policy currently set on the SDK (see more on the table below), and indicate;

  - That a re-connection can be immediatelly attempted; OR

> In such case the protocol transport will attempt reconnecting.
>
> If if succeeds, it will instruct the Retry Control to reset its counters, and the next wait times it calculates will start fresh from the initial base time.
>
> If the re-connection attempt fails the protocol transport will continue checking with the Retry Control if it can try again. Depending on the Retry Policy currently set this next wait time calculated by the Retry Control can be longer (and similarly along with the next wait times calculated), aiming at easing out the frequency with which the device client attempts to reconnect. That gives a chance for the network (at the Operating System level) to get back to its normal availability.

- That the transport protocol must wait for a while to attempt re-connecting; OR

> The transport protocol will continue to check with the Retry Control if it is time to attempt re-connecting (until it is finally allowed or denied).

- That no re-connections should be attempted anymore.

> The Retry Policies are composed by a algorithm to calculate the wait times in between reconnection attempts, as well as a maximum time for the total ammount of consecutive tries (see argument "retryTimeoutLimitInSeconds" on the functions below).
>
> If this timeout is reached, the Retry Control will instruct the transport protocol to not re-connect anymore. In this scenario a Connection Status notification is raised and the user application is made aware of that. Some customers use this aspect of the feature to perform manual intervention in case re-connections have been failing consecutivelly for long periods (e.g., 24 hours or more), possibly indicating more serious network issues.
>
> One important detail is that the protocol transport never ceases checking with the Retry Control if it can attempt to re-connect again, even when it is told not to try anymore. In that case, if during execution time the user changes the current Retry Policy (see below for details) the Retry Control gets reset, giving the opportunity for the device client to start attemping to re-connect again.

Currently the default Retry Policy in the Azure IoT Device Client C SDK is IOTHUB\_CLIENT\_RETRY\_EXPONENTIAL\_BACKOFF\_WITH\_JITTER (with no timeout), but it can be set by using the following SDK function:

In [iothub\_client module](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client.h#L188):

IOTHUB\_CLIENT\_RESULT IoTHubClient\_SetRetryPolicy( IOTHUB\_CLIENT\_HANDLE iotHubClientHandle, IOTHUB\_CLIENT\_RETRY\_POLICY retryPolicy, size\_t retryTimeoutLimitInSeconds);

Or if using the [iothub\_client\_ll module](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L419):

IOTHUB\_CLIENT\_RESULT IoTHubClient\_LL\_SetRetryPolicy( IOTHUB\_CLIENT\_LL\_HANDLE iotHubClientHandle, IOTHUB\_CLIENT\_RETRY\_POLICY retryPolicy, size\_t retryTimeoutLimitInSeconds);

If retryTimeoutLimitInSeconds is set as 0 (zero) the timeout for retry policies is disabled.

The [current retry policies](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L53) available for the argument retryPolicy are:

|Policy|Description|Example|
|-|-|-|
|IOTHUB_CLIENT_RETRY_NONE|No re-connections are ever attempted.|Usually this option is used along with Connection Status callbacks by users that want to implement their own connection retry logic (at the application layer).|
|IOTHUB_CLIENT_RETRY_NONE|No re-connections are ever attempted.</br>Usually this option is used along with Connection Status callbacks by users that want to implement their own connection retry logic (at the application layer).|Device client detects a connection issue, but it never attempts to reconnect.|
|IOTHUB_CLIENT_RETRY_IMMEDIATE|Re-connections shall be tried immediatelly, with no wait time in between attempts|Device client detects a connection issue.</br></br>The re-connection attempts happen immediatelly in a loop with no wait time until one succeeds|
|IOTHUB_CLIENT_RETRY_INTERVAL|First attempt should be done immediatelly.</br></br>Until the re-connection succeeds, each subsequent attempt is subject to a fixed-interval wait time (5 seconds by default).</br></br>|Device client detects a connection issue.</br></br>The first re-connection attempt happens immediatelly, then again every 5 seconds until it succeeds|
|IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF|First attempt should be done immediatelly.</br></br>Until the re-connection succeeds, each subsequent attempt is subject to a wait time that grows linearly.</br></br>Default behavior: starts from 5 seconds and grows by increments of 5 seconds each time.</br></br>|Device client detects a connection issue.</br></br>The first re-connection attempt happens immediatelly, then again every 5 seconds until it succeeds|
|IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF|First attempt should be done immediatelly.</br></br>Until the re-connection succeeds, each subsequent attempt is subject to a wait time that grows exponentially.</br></br>Default behavior: starts from 1 second and doubles each time.</br></br>|Device client detects a connection issue.</br></br>The first re-connection attempt happens immediatelly, then again in 1 second, then again 2 seconds, 4 seconds, 8 seconds, 16, 32, 64, ... until it succeeds.|
|IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER|First attempt should be done immediatelly.</br></br>Until the re-connection succeeds, each subsequent attempt is subject to a wait time that grows exponentially but with a random jitter deduction.</br></br>Default behavior: starts from 1 second and doubles each time minus a random jitter of zero to one-hundred percent.</br></br>|Device client detects a connection issue.</br></br>The first re-connection attempt happens immediatelly, then again in 1 second, then again 1 second (-100% jitter), 2 seconds (0% jitter), 3 seconds (-50% jitter), 6 (0% jitter), 10 (-67% jitter), 19 (-10% jitter), ... until it succeeds.|
|IOTHUB_CLIENT_RETRY_RANDOM|First attempt should be done immediatelly.</br></br>Until the re-connection succeeds, each subsequent attempt is subject to a random wait time.</br></br>Default behavior: the random wait time range is from 0 to 5 seconds.</br></br>|Device client detects a connection issue.</br></br>The first re-connection attempt happens immediatelly, then again in 5 seconds (random multiplier of 100%), then again 2 seconds ( (random multiplier of 40%), 4 seconds (random multiplier of 80%), 0 seconds (random multiplier of 0%), 3 (60%), ... until it succeeds.|

### Connection Status Callback

The Azure IoT Device Client C SDK provides a callback option to notify the upper application layer if it is connected to the Azure IoT Hub or not, followed by a standardized reason.

To access it the user can invoke one of the functions below, passing a callback function.

On [iothub\_client](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client.h#L171) module:

IOTHUB\_CLIENT\_RESULT IoTHubClient\_SetConnectionStatusCallback(IOTHUB\_CLIENT\_HANDLE iotHubClientHandle, IOTHUB\_CLIENT\_CONNECTION\_STATUS\_CALLBACK connectionStatusCallback, void\* userContextCallback);

On [iothub\_client\_ll](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L402) module:

IOTHUB\_CLIENT\_RESULT IoTHubClient\_LL\_SetConnectionStatusCallback(IOTHUB\_CLIENT\_LL\_HANDLE iotHubClientHandle, IOTHUB\_CLIENT\_CONNECTION\_STATUS\_CALLBACK connectionStatusCallback, void\* userContextCallback);

This callback will be invoked in these specific situations:

- When the device client gets connected or re-connected to the Azure IoT Hub;

- When the device client gets disconnected due to any issues;

> These include network availability and IoT hub connectivity issues, authentication failures.

- When the device client ceases attempting to re-connect (if the Retry Policy no longer allows to).

Please take a look [here](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L144) for more info on the possible status and reasons.

### Current Configuration Options

Most of the options exposed by the public API of the Azure IoT Device Client C SDK are listed on the header file [\`iothub\_client\_options.h](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_options.h).

They can be set by one of the \_SetOption functions depending on the module used:

On [iothub\_client](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client.h#L269):

IOTHUB\_CLIENT\_RESULT IoTHubClient\_SetOption( IOTHUB\_CLIENT\_HANDLE iotHubClientHandle, const char\* optionName, const void\* value);

On [iothub\_client\_ll](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_ll.h#L506):

IOTHUB\_CLIENT\_RESULT IoTHubClient\_LL\_SetOption( IOTHUB\_CLIENT\_LL\_HANDLE iotHubClientHandle, const char\* optionName, const void\* value);

Here is a list of the specific options that apply to connection and messaging reliability:

|Option|Value Type|Applicable To|Description|
|-|-|-|-|
|OPTION_MESSAGE_TIMEOUT|const tickcounter_ms_t*|Timeout for iothub client messages waiting to be sent to the IoTHub|See [description above](#Known-Issue:-Duplicated-Timeout-Control-for-Sending-Telemetry-Messages) for details.</br></br>The default value is zero (disabled).|
|"event_send_timeout_secs"|size_t*|AMQP and AMQP over WebSockets transports|Maximum amount of time, in seconds, the AMQP protocol transport will wait for a Telemetry message to complete sending.</br></br>If reached, the callback function passed to IoTHubDeviceClient_LL_SendEventAsync or IoTHubDeviceClient_SendEventAsync is invoked with result IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT.</br></br>The default value 5 minutes.|
|OPTION_SERVICE_SIDE_KEEP_ALIVE_FREQ_SECS|size_t*|AMQP and AMQP over WebSockets transports|[See code comments](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_options.h#L47).|
|OPTION_REMOTE_IDLE_TIMEOUT_RATIO|double*|AMQP and AMQP over WebSockets transports|[See code comments](https://github.com/Azure/azure-iot-sdk-c/blob/2018-05-04/iothub_client/inc/iothub_client_options.h#L59).|
|OPTION_KEEP_ALIVE|int*|MQTT and MQTT over WebSockets protocol transports|Frequency in seconds that the transport protocol will be sending MQTT pings to the Azure IoT Hub.</br></br>The lower this number, more responsive the device client (when using MQTT) will be to connection issues. However, slightly more data traffic it will generate.</br></br>The default value is 4 minutes.|
|OPTION_CONNECTION_TIMEOUT|int*|MQTT and MQTT over WebSockets protocol transports|While connecting, it is the maximum number of seconds the device client (when using MQTT) will wait for the connection to complete (CONNACK).</br></br>The default value is 30 seconds.|

### Special Timeout Controls Not Exposed to the User

Although not currently configurable, they are important for further understanding the directives that impact the connection, re-connection and messaging within the Azure IoT Device Client C SDK.

|Control|Description|
|-|-|
|Number of cumulative failures the AMQP protocol transport will wait for to mark a connection as faulty|Currently that number is five.|
|Number of cumulative send failures the MQTT protocol transport will wait for to mark a connection as faulty|Currently that number is two.|
|Maximum time the AMQP transport protocols will wait for the AMQP negotiation to complete (including authentication) when a device client is connecting to the Azure IoT Hub connection|The default value is 60 seconds.</br></br>If the whole AMQP negotiation does not complete within that time, the connection is deemed faulty and re-connection kicks in.</br></br>That can be triggered by super-slow connections. Relaxing this timeout hasn't showed practical improvements overall.|
