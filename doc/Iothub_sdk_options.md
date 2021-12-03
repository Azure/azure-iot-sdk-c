# IoT Hub C SDK Options

This document describes how you can set options for the Azure IoT Hub and Device Provisioning Service (DPS) client connections.

- [IoT Hub C SDK Options](#iot-hub-c-sdk-options)
  - [Example Code for Setting an Option](#example-code-for-setting-an-option)
  - [When to Set Options](#when-to-set-options)
  - [Common Transport Options](#common-transport-options)
  - [IoT Hub Device and Module Client Options](#iot-hub-device-and-module-client-options)
  - [MQTT, AMQP, and HTTP Specific Protocol Options](#mqtt-amqp-and-http-specific-protocol-options)
    - [MQTT Specific Options](#mqtt-specific-options)
    - [AMQP Specific Options](#amqp-specific-options)
    - [HTTP Specific Options](#http-specific-options)
  - [Device Provisioning Service (DPS) Client Options](#device-provisioning-service-dps-client-options)
  - [File Upload Options](#file-upload-options)
  - [Batching and IoT Hub Client SDK](#batching-and-iot-hub-client-sdk)
  - [Advanced Compilation Options](#advanced-compilation-options)

## Example Code for Setting an Option

You will use a different API to set options, depending on whether you are using a device or module identity and whether you are using the convenience or lower layer APIs.  The pattern is generally the same.


```c
// Convenience layer for device client
IOTHUB_CLIENT_RESULT IoTHubDeviceClient_SetOption(IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value);

// Lower layer for device client
IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SetOption(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value);

// Convenience layer for module client
IOTHUB_CLIENT_RESULT IoTHubModuleClient_SetOption(IOTHUB_MODULE_CLIENT_HANDLE iotHubModuleClientHandle, const char* optionName, const void* value);

// Lower layer for module client
IOTHUB_CLIENT_RESULT IoTHubModuleClient_LL_SetOption(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle, const char* optionName, const void* value);
```

An example of setting an option:

```c
// IoT Hub Device Client
bool trace_on = true;
IoTHubDeviceClient_SetOption(sdk_handle, OPTION_LOG_TRACE, &trace_on);

// Device Provisioning Service (DPS) Client
HTTP_PROXY_OPTIONS http_proxy;
memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
http_proxy.host_address = PROXY_ADDRESS;
http_proxy.port = PROXY_PORT;
Prov_Device_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);
```

## When to Set Options

You should set the options you need right after creating your IoT Hub device or module handle.  Setting most options after the connection has been initiated may be silently ignored or applied much later.  Many of these are used at connection initiation time itself.


## Common Transport Options
You can use the options below for the IoT Hub Device Client, the IoT Hub Module Client, and for the Device Provisioning Client.  These options are declared in [shared_util_options.h][shared-util-options-h].

**NOTE: Uploading files to Azure (e.g. IoTHubDeviceClient_UploadToBlobAsync) does not support this complete set of options.  See [here](#upload-options) for more.**

| Option Name                       | Option Define                   | Value Type         | Description
|-----------------------------------|---------------------------------|--------------------|-------------------------------
| `"TrustedCerts"`                  | OPTION_TRUSTED_CERT             | const char*        | Azure Server certificate used to validate TLS connection to IoT Hub.  This is usually not required on operating systems that have built in certificates to trust, such as Windows and some Linux distributions.  A typical use case is on an embedded system which does not trust any certificates or when connecting to a gateway whose certificates that are not otherwise trusted.  See [here][gateway-sample] for a gateway sample.
| `"x509certificate"`               | SU_OPTION_X509_CERT             | const char*        | Sets an RSA x509 certificate used for connection authentication.  (Also available from [iothub_client_options.h][iothub-client-options-h] as `OPTION_X509_CERT`.)
| `"x509privatekey"`                | SU_OPTION_X509_PRIVATE_KEY      | const char*        | Sets the private key for the RSA x509 certificate.  (Also available from [iothub_client_options.h][iothub-client-options-h] as `OPTION_X509_PRIVATE_KEY`.)
| `"x509EccCertificate"`            | OPTION_X509_ECC_CERT            | const char*        | Sets the ECC x509 certificate used for connection authentication
| `"x509EccAliasKey"`               | OPTION_X509_ECC_KEY             | const char*        | Sets the private key for the ECC x509 certificate
| `"proxy_data"`                    | OPTION_HTTP_PROXY               | [HTTP_PROXY_OPTIONS*][shared-util-options-h]| Http proxy data object used for proxy connection to IoT Hub
| `"tls_version"`                   | OPTION_TLS_VERSION              | int*               | TLS version to use for openssl, 10 for version 1.0, 11 for version 1.1, 12 for version 1.2.  (**DEPRECATED**: TLS 1.0 and 1.1 are not secure and should not be used.  This option is included only for backward compatibility.)


## IoT Hub Device and Module Client Options
You can use the options below for IoT Hub connections.  These options are declared in [iothub_client_options.h][iothub-client-options-h].  

You may also use [common transport options](#general_options) options.

**NOTE: Uploading files to Azure (e.g. IoTHubDeviceClient_UploadToBlobAsync) does not support this complete set of options.  See [here](#upload-options) for more.**


| Option Name                       | Option Define                   | Value Type         | Description
|-----------------------------------|---------------------------------|--------------------|-------------------------------
| `"logtrace"`                      | OPTION_LOG_TRACE                | bool*              | Turn on and off log tracing for the transport
| `"messageTimeout"`                | OPTION_MESSAGE_TIMEOUT          | tickcounter_ms_t*  | (DEPRECATED) Timeout used for message on the message queue
| `"product_info"`                  | OPTION_PRODUCT_INFO             | const char*        | User defined Product identifier sent to the IoT Hub service
| `"retry_interval_sec"`            | OPTION_RETRY_INTERVAL_SEC       | unsigned int*      | Number of seconds between retries when using the interval retry policy.  (Not supported for HTTP transport.)
| `"retry_max_delay_secs"`          | OPTION_RETRY_MAX_DELAY_SECS     | unsigned int*      | Maximum number of seconds a retry delay when using linear backoff, exponential backoff, or exponential backoff with jitter policy.  (Not supported for HTTP transport.)
| `"sas_token_lifetime"`            | OPTION_SAS_TOKEN_LIFETIME       | size_t*            | Length of time in seconds used for lifetime of SAS token.
| `"do_work_freq_ms"`               | OPTION_DO_WORK_FREQUENCY_IN_MS  | [tickcounter_ms_t *][tick-counter-header] | Specifies how frequently the worker thread spun by the convenience layer will wake up, in milliseconds.  The default is 1 millisecond.  The maximum allowable value is 100.  (Convenience layer APIs only)


## MQTT, AMQP, and HTTP Specific Protocol Options

Some options are only supported by a given protocol (e.g. MQTT, AMQP, HTTP).  These are declared in [iothub_client_options.h][iothub-client-options-h].

### MQTT Specific Options

| Option Name               | Option Define                 | Value Type         | Description
|---------------------------|-------------------------------|--------------------|-------------------------------
| `"auto_url_encode_decode"`| OPTION_AUTO_URL_ENCODE_DECODE | bool*              | Turn on and off automatic URL Encoding and Decoding.  **You are strongly encouraged to set this to true.**  If you do not do so and send a property with a character that needs URL encoding to the server, it will result in hard to diagnose problems.  The SDK cannot auto-enable this feature because it needs to maintain backwards compatibility with applications already doing their own URL encoding.
| `"keepalive"`             | OPTION_KEEP_ALIVE             | int*               | Length of time to send `Keep Alives` to service for D2C Messages
| `"model_id"`              | OPTION_MODEL_ID               | const char*        | [IoT Plug and Play][iot-pnp] model ID the device or module implements

### AMQP Specific Options

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"cbs_request_timeout"`      | OPTION_CBS_REQUEST_TIMEOUT      | size_t*           | Number of seconds to wait for a CBS request to complete
| `"sas_token_refresh_time"`   | OPTION_SAS_TOKEN_REFRESH_TIME   | size_t*           | Frequency in seconds that the SAS token is refreshed
| `"event_send_timeout_secs"`  | OPTION_EVENT_SEND_TIMEOUT_SECS  | size_t*           | Number of seconds to wait for telemetry message to complete
| `"c2d_keep_alive_freq_secs"` | OPTION_C2D_KEEP_ALIVE_FREQ_SECS | size_t*           | Informs service of maximum period the client waits for keep-alive message

### HTTP Specific Options

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"Batching"`                 | OPTION_BATCHING                 | bool*             | Turn on and off message batching
| `"MinimumPollingTime"`       | OPTION_MIN_POLLING_TIME         | unsigned int*     | Minimum time in seconds allowed between 2 consecutive GET issues to the service
| `"timeout"`                  | OPTION_HTTP_TIMEOUT             | long*             | When using curl the amount of time before the request times out, defaults to 242 seconds.

## Device Provisioning Service (DPS) Client Options

You can use the options below to configure the DPS client.  These are defined in [prov_device_ll_client.h][provisioning-device-ll-client-options-h] except for `PROV_OPTION_DO_WORK_FREQUENCY_IN_MS` which is defined in [prov_device_client.h][provisioning-device-client-options-h].

You may also use [common transport options](#general_options).

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"logtrace"`                 | PROV_OPTION_LOG_TRACE           | bool*             | Turn on and off log tracing for the transport
| `"registration_id"`          | PROV_REGISTRATION_ID            | const char*       | The registration ID of the device.
| `"provisioning_timeout"`     | PROV_OPTION_TIMEOUT             | long*             | Maximum time to allow DPS to complete, in seconds.
| `"do_work_freq_ms"`          | PROV_OPTION_DO_WORK_FREQUENCY_IN_MS | uint16_t * | Specifies how frequently the worker thread spun by the convenience layer will wake up, in milliseconds.  The default is 1 millisecond.  (Convenience layer APIs only)

## File Upload Options

When you upload  files to Azure with APIs like `IoTHubDeviceClient_LL_UploadToBlob`, most of the options described above are silently ignored.  This is even though these APIs use the same IoT Hub handle as used for telemetry, device methods, and device twin.  The reason the options are different is because the underlying transport is implemented differently for uploads.

The following options are supported when performing file uploads.  They are declared in [iothub_client_options.h][iothub-client-options-h] and in [shared_util_options.h][shared-util-options-h].

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"blob_upload_timeout_secs"` | OPTION_BLOB_UPLOAD_TIMEOUT_SECS | size_t*           | Timeout in seconds of initial connection establishment to IoT Hub.  NOTE: This does not specify the end-to-end time of the upload, which is currently not configurable.
| `"CURLOPT_VERBOSE"`          | OPTION_CURL_VERBOSE             | bool*             | Turn on and off verbosity at curl level.  (Only available when using curl as underlying HTTP client.)
| `"x509certificate"`          | OPTION_X509_CERT                | const char*       | Sets an RSA x509 certificate used for connection authentication
| `"x509privatekey"`           | OPTION_X509_PRIVATE_KEY         | const char*       | Sets the private key for the RSA x509 certificate
| `"TrustedCerts"`             | OPTION_TRUSTED_CERT             | const char*       | Azure Server certificate used to validate TLS connection to IoT Hub and Azure Storage
| `"proxy_data"`               | OPTION_HTTP_PROXY               | [HTTP_PROXY_OPTIONS*][shared-util-options-h] | Http proxy data object used for proxy connection to IoT Hub and Azure Storage
| `"network_interface_upload_to_blob"`| OPTION_NETWORK_INTERFACE_UPLOAD_TO_BLOB | const char* | Set the interface name to use as outgoing network interface for upload to blob.  NOTE: Not all HTTP clients support this option. It is currently only supported when using cURL.
| `"blob_upload_tls_renegotiation"`| OPTION_BLOB_UPLOAD_TLS_RENEGOTIATION | bool* | *[HTTP Compact](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/httpapi_compact_requirements.md) only; not supported when using other HTTP stacks such as cURL and WinHTTP.*   Tells HTTP stack to enable TLS renegotiation when using client certificates.  Non-HTTP Compact stacks will use their defaults for this.

## Batching and IoT Hub Client SDK

Batching is the ability of a protocol to send multiple messages in one payload, rather than one at a time.  This can result in less overhead, especially when sending multiple, small messages.  This SDK supports various levels of batching when using IoTHubClient_LL_SendEventAsync.

- AMQP uses batching always.

- HTTP can optionally enable batching, using the "Batching" option.

- MQTT does not have a batching option.

None of the protocols has a windowing or Nagling concept. They do NOT wait a certain amount of time to attempt to queue up multiple messages to put into a single batch.  Instead, they just batch whatever is on the to-send queue.  For customers using the lower-layer protocols (LL), they can force batching by performing multiple `IoTHubDeviceClient_LL_SendEventAsync` calls before `IoTHubDeviceClient_LL_DoWork`.

```c
IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle;
// Queues msg1 to be sent but does not perform any network I/O
IoTHubDeviceClient_LL_SendEventAsync(iotHubClientHandle, msg1, ...);
// Queues msg2 to be sent but does not perform any network I/O
IoTHubDeviceClient_LL_SendEventAsync(iotHubClientHandle, msg2, ...);
// Performs network I/O.  If batching is enabled, the SDK will batch msg1 and msg2
IoTHubDeviceClient_LL_DoWork(iotHubClientHandle);
```

## Advanced Compilation Options

We recommend leaving the following settings at their defaults. Tuning them may allow optimizations for specific devices or scenarios but could also negatively impact RAM or EEPROM usage.
The options are presented only as compilation flags and must be appended to the CMake `compileOption_C` setting:

| Option Name                  | Option Define                                           | Description
|------------------------------|---------------------------------------------------------|-------------------------------------------------------------
| `"XIO Receive Buffer"`       | `-DcompileOption_C="-DXIO_RECEIVE_BUFFER_SIZE=<value>"` | Configure the internal XIO receive buffer.


[iothub-client-options-h]: https://github.com/Azure/azure-iot-sdk-c/blob/main/iothub_client/inc/iothub_client_options.h
[shared-util-options-h]: https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/shared_util_options.h
[provisioning-device-ll-client-options-h]: https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/inc/azure_prov_client/prov_device_ll_client.h
[provisioning-device-client-options-h]: https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/inc/azure_prov_client/prov_device_client.h
[iot-pnp]: https://aka.ms/iotpnp
[gateway-sample]: https://github.com/Azure/azure-iot-sdk-c/tree/main/iothub_client/samples/iotedge_downstream_device_sample
[tick-counter-header]: https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/tickcounter.h
