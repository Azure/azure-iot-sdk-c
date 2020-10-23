# IoT Hub C SDK Options

This document describes how to set the options available in the c sdk.

- [Setting Options Example](#set_option)
- [IoT Hub Client Options](#IotHub_options)
- [Transport Specific Options](#transport_options)
- [Device Provisioning Service (DPS) Client Options](#provisioning_option)
- [File Upload Options](#upload-options)

<a name="set_option"></a>

## Setting an Option

Setting an option in the c-sdk is dependant on which api set you are using:

```c
// Convience Layer
IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value)

// Single Thread API
IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
```

An example of using a set option:

```c
bool trace_on = true;
IoTHubClient_SetOption(sdk_handle, OPTION_LOG_TRACE, &trace_on);

HTTP_PROXY_OPTIONS http_proxy;
memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
http_proxy.host_address = PROXY_ADDRESS;
http_proxy.port = PROXY_PORT;
DPS_LL_SetOption(handle, OPTION_HTTP_PROXY, &http_proxy);
```

## Available Options for telemetry, device twins, and device methods
The options in this section are for IoT Hub connections that use telemetry, device twins, and device methods.  Most of these options are ignored when performing a file upload operation such as `IoTHubDeviceClient_LL_UploadToBlob`.  File upload options are considered [separately](#upload-options).


<a name="IotHub_options"></a>

## IoTHub_Client

### iothub_client_options.h

The options for configuring the IoT Hub device client are defined in mulitple header files.  The usage pattern, as defined above, is the same in all cases.

These options are declared in [iothub_client_options.h][iothub-client-options-h].


| Option Name                       | Option Define                   | Value Type         | Description
|-----------------------------------|---------------------------------|--------------------|-------------------------------
| `"logtrace"`           | OPTION_LOG_TRACE          | bool* value        | Turn on and off log tracing for the transport
| `"messageTimeout"`              | OPTION_MESSAGE_TIMEOUT           | tickcounter_ms_t*  | (DEPRECATED) Timeout used for message on the message queue
| `"product_info"`                | OPTION_PRODUCT_INFO              | const char*        | User defined Product identifier sent to the IoT Hub service
| `"retry_interval_sec"`          | OPTION_RETRY_INTERVAL_SEC        |  unsigned int*              | Amount of seconds between retries when using the interval retry policy.  (Not supported for HTTP transport.)
| `"retry_max_delay_secs"`        | OPTION_RETRY_MAX_DELAY_SECS      |  unsigned int*              | Maximum number of seconds a retry delay when using linear backoff, exponential backoff, or exponential backoff with jitter policy.  (Not supported for HTTP transport.)
| `"sas_token_lifetime"` | OPTION_SAS_TOKEN_LIFETIME | size_t* value      | Length of time in seconds used for lifetime of sas token.
| `"x509certificate"`    | OPTION_X509_CERT          | const char*        | Sets an RSA x509 certificate used for connection authentication
| `"x509privatekey"`     | OPTION_X509_PRIVATE_KEY   | const char*        | Sets the private key for the RSA x509 certificate

 These options are declared in [shared_util_options.h][shared-util-options-h].

| Option Name                       | Option Define                   | Value Type         | Description
|-----------------------------------|---------------------------------|--------------------|-------------------------------
| `"TrustedCerts"`                | OPTION_TRUSTED_CERT              | const char*        | Azure Server certificate used to validate TLS connection to IoT Hub
| `"x509EccCertificate"` | OPTION_X509_ECC_CERT      | const char*        | Sets the ECC x509 certificate used for connection authentication
| `"x509EccAliasKey"`    | OPTION_X509_ECC_KEY       | const char*        | Sets the private key for the ECC x509 certificate
| `"proxy_data"`         | OPTION_HTTP_PROXY         | [HTTP_PROXY_OPTIONS*][shared-util-options-h]| Http proxy data object used for proxy connection to IoT Hub
| `"tls_version"`         | OPTION_TLS_VERSION         | int*            | TLS version to use for openssl, 10 for version 1.0, 11 for version 1.1, 12 for version 1.2


<a name="transport_options"></a>

## Transport specific options

Some options are only supported by a given transport.  These are always declared in [iothub_client_options.h][iothub-client-options-h].

### MQTT Specific Options

| Option Name               | Option Define                 | Value Type         | Description
|---------------------------|-------------------------------|--------------------|-------------------------------
| `"keepalive"`             | OPTION_KEEP_ALIVE             | int*               | Length of time to send `Keep Alives` to service for D2C Messages
| `"auto_url_encode_decode"`| OPTION_AUTO_URL_ENCODE_DECODE | bool*              | Turn on and off automatic URL Encoding and Decoding
| `"model_id"`              | OPTION_MODEL_ID               | const char*        | IoT Plug and Play model ID the device implements

### AMQP Specific Options

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"cbs_request_timeout"`      | OPTION_CBS_REQUEST_TIMEOUT      | `size_t`* value   | Amount of seconds to wait for a cbs request to complete
| `"sas_token_refresh_time"`   | OPTION_SAS_TOKEN_REFRESH_TIME   | `size_t`* value   | Frequency in seconds that the SAS token is refreshed
| `"event_send_timeout_secs"`  | OPTION_EVENT_SEND_TIMEOUT_SECS  | `size_t`* value   | Amount of seconds to wait for telemetry message to complete
| `"c2d_keep_alive_freq_secs"` | OPTION_C2D_KEEP_ALIVE_FREQ_SECS | `size_t`* value   | Informs service of maximum period the client waits for keep-alive message

### HTTP Specific Options

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"Batching"`                 | OPTION_BATCHING                 | `bool`* value     | Turn on and off message batching
| `"MinimumPollingTime"`       | OPTION_MIN_POLLING_TIME         | `unsigned int`* value     | Minimum time in seconds allowed between 2 consecutive GET issues to the service
| `"timeout"`                  | OPTION_HTTP_TIMEOUT             | `long`* value     | When using curl the amount of time before the request times out, defaults to 242 seconds.

<a name="provisioning_option></a>

## Device Provisioning Service (DPS) Client Options

These options are supporting fort the  DPS client.
declared in [prov_device_ll_client.h][provisioning-device-client-options-h].


<a name="upload-options"></a>

## File Upload Options

When uploading files to Azure, most of the options above are silently ignored.  This is even though `IoTHubDeviceClient_LL_UploadToBlob` and related APIs use the same IoT Hub handle as used for telemetry, device methods, and device twin.  The reason the options are different is because the underlying transport is implemented differently.

The following options are supported when performing file uploads.  They are declared in [iothub_client_options.h][iothub-client-options-h].

| Option Name                  | Option Define                   | Value Type        | Description
|------------------------------|---------------------------------|-------------------|-------------------------------
| `"blob_upload_timeout_secs"` | OPTION_BLOB_UPLOAD_TIMEOUT_SECS | size_t*           | Timeout in seconds of initial connection establishment to IoT Hub.  NOTE: This does not specify the end-to-end time of the upload, which is currently not configurable.
| `"CURLOPT_VERBOSE"`          | OPTION_CURL_VERBOSE             | `bool`* value     | Turn on and off verbosity at curl level.  (For stacks using curl only.)
| `"x509certificate"`          | OPTION_X509_CERT                | const char*       | Sets an RSA x509 certificate used for connection authentication
| `"x509privatekey"`           | OPTION_X509_PRIVATE_KEY         | const char*       | Sets the private key for the RSA x509 certificate
| `"TrustedCerts"`             | OPTION_TRUSTED_CERT             | const char*       | Azure Server certificate used to validate TLS connection to IoT Hub and Azure Storage
| `"proxy_data"`               | OPTION_HTTP_PROXY               | [HTTP_PROXY_OPTIONS*][http-proxy-object]| Http proxy data object used for proxy connection to IoT Hub and Azure Storage

## Batching and IoT Hub Client SDK

Batching is the ability of a protocol to send multiple messages in one payload, rather than one at a time.  This can result in less overhead, especially when sending multiple, small messages.  This SDK supports various levels of batching when using IoTHubClient_LL_SendEventAsync.

- AMQP uses batching always.

- HTTP can optionally enable batching, using the "Batching" option referenced above.

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


[iothub-client-options-h]: https://github.com/Azure/azure-iot-sdk-c/blob/master/iothub_client/inc/iothub_client_options.h
[shared-util-options-h]: https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/shared_util_options.h
[provisioning-device-client-options-h]: https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/inc/azure_prov_client/prov_device_ll_client.h
